Token *tk;

// node structure

enum NodeKind {
#define x(a, b) a,
#include "parse.inc"
#undef x
};
typedef enum NodeKind NodeKind;

typedef struct Node Node;
struct Node {
	NodeKind kind;
	Token *tk;
	Node *children[];
};

#define FIRST(nd) ((nd)->children[0])
#define SECOND(nd) ((nd)->children[1])
#define THIRD(nd) ((nd)->children[2])
#define FORTH(nd) ((nd)->children[3])
#define FITH(nd) ((nd)->children[4])

#define LEFT FIRST
#define RIGHT SECOND

#define IF_COND FIRST
#define IF_BODY SECOND
#define IF_ELSE THIRD

#define FOR_INIT FIRST
#define FOR_COND SECOND
#define FOR_INC THIRD
#define FOR_BODY FORTH

#define WHILE_COND FOR_COND
#define WHILE_BODY FOR_BODY

#define DO_BODY FIRST
#define DO_COND SECOND

#define FN_SPEC FIRST
#define FN_TYPE SECOND
#define FN_VAR THIRD
#define FN_PARAMS FORTH
#define FN_BODY FITH

#define DECL_TYPE FIRST
#define DECL_VAR SECOND
#define DECL_NEXT THIRD

static unsigned node_size(NodeKind kind) {
	switch (kind) {
#define x(a, b) case a: return sizeof(Node) + sizeof(Node *) * b;
#include "parse.inc"
#undef x
	}
}

static Node *node_new(NodeKind kind) {
	unsigned size = node_size(kind);
	Node *nd = malloc(size);
	nd->kind = kind;
	nd->tk = tk;
	memset(nd->children, 0, size - sizeof(Node));
	return nd;
}

static Node *node_cpy(Node *nd) {
	unsigned size = node_size(nd->kind);
	Node *tmp = malloc(size);
	memcpy(tmp, nd, size);
	return tmp;
}

static Node *node_wrap(NodeKind kind, Node *left) {
	Node *nd = node_new(kind);
	LEFT(nd) = left;
	return nd;
}

static unsigned node_depth(Node *nd) {
	return nd ? 1 + node_depth(SECOND(nd)) : 0;
}

// ast structure

typedef struct Scope Scope;
typedef struct Ast Ast;
struct Ast {
	Node **vars, **fns;
};

// parsing functions

static bool token_consume(TokenKind k) {
	return tk->kind == k ? ++tk, true :
		false;
}

static void token_expect(TokenKind k) {
	if(!token_consume(k))
		error_token(tk, "expected '%c'\n", (char)k);
}

static bool is_specifier(void) {
	return tk->kind == TK_TYPE;
}

// huristic for difference between definition and declaration
// TODO does this work with structs?
static bool is_fn(void) {
	for (Token *tmp = tk; tmp->kind != '\0'; ++tmp)
		if (tmp->kind == ';')
			return false;
		else if (tmp->kind == ')' && tmp[1].kind == '{')
			return true;
	return false;
}

// recursive function declarations

typedef Node *ParseFn(void);

static ParseFn(parse_expr); 
static ParseFn(parse_assign);
static ParseFn(parse_stmt); 

static Node *parse_binary_left(NodeKind get_node_kind[TK_COUNT], ParseFn *parse_child) {
	Node *nd = parse_child();
	NodeKind kind;
	while(kind = get_node_kind[tk->kind]) {
		nd = node_wrap(kind, nd);
		++tk;
		SECOND(nd) = parse_child();
	}
	return nd;
}

static Node *parse_binary_right(NodeKind get_node_kind[TK_COUNT], ParseFn *parse_child) {
	Node *nd = parse_child();
	NodeKind kind = get_node_kind[tk->kind];
	if(kind) {
		nd = node_wrap(kind, nd);
		++tk;
		SECOND(nd) = parse_binary_left(get_node_kind, parse_child);
	}
	return nd;
}

static Node *parse_var(void) {
	Node *nd = node_new(ND_VAR);
	token_expect(TK_ID);
	return nd;
}

static Node *parse_atom(void) {
	Node *nd;
	switch(tk->kind) {
		case TK_INT:
			nd = node_new(ND_INT);
			break;
		case '(':
			++tk;
			nd = parse_expr();
			token_expect(')');
			return nd;
		case TK_ID:
			return parse_var();
		default:
			error_token(tk, "syntax\n");
	}
	++tk;
	return nd;
}

static Node *parse_arg(void) {
	Node *nd = node_new(ND_ARG);
	FIRST(nd) = parse_assign();
	return nd;
}

static Node *parse_args(void) {
	char head[node_size(ND_ARG)];
	Node *nd = (Node *)head;
	do
		nd = SECOND(nd) = parse_arg();
	while (token_consume(','));
	token_expect(')');
	return SECOND((Node *)head);
}

static Node *parse_postfix(void) {
	NodeKind get_node_kind[TK_COUNT] = {
		['('] = ND_CALL,
		[TK_INC] = ND_POST_INC,
		[TK_DEC] = ND_POST_DEC,
	};
	Node *nd = parse_atom();
	NodeKind kind = get_node_kind[tk->kind];
	switch(kind) {
		case 0:
			return nd;
		case ND_CALL:
			nd = node_wrap(kind, nd);
			++tk;
			if(!token_consume(')'))
				SECOND(nd) = parse_args();
			return nd;
		default:
			nd = node_wrap(kind, nd);
			++tk;
			return nd;
	}
}

static Node *parse_prefix(void) {
	NodeKind get_node_kind[TK_COUNT]  = {
		['!'] = ND_LNOT,
		['&'] = ND_ADDR,
		['*'] = ND_DEREF,
		['-'] = ND_NEG,
		['~'] = ND_NOT,
		[TK_DEC] = ND_PRE_DEC,
		[TK_INC] = ND_PRE_INC,
	};
	token_consume('+'); // ignoring unary + as it does nothing
	NodeKind kind = get_node_kind[tk->kind];
	if(kind) {
		Node *nd = node_new(kind);
		++tk;
		FIRST(nd) = parse_prefix();
		return nd;
	} 
	return parse_postfix(); 
}

static Node *parse_mul(void) {
	NodeKind table[TK_COUNT] = {
		['*'] = ND_MUL,
		['/'] = ND_DIV,
		['%'] = ND_MOD,
	};
	return parse_binary_left(table, &parse_prefix);
}

static Node *parse_add(void) {
	NodeKind table[TK_COUNT] = {
		['+'] = ND_ADD,
		['-'] = ND_SUB,
	};
	return parse_binary_left(table, &parse_mul);
}

static Node *parse_shift(void) {
	NodeKind table[TK_COUNT] = {
		[TK_SHR] = ND_SHR,
		[TK_SHL] = ND_SHL,
	};
	return parse_binary_left(table, &parse_add);
}

static Node *parse_relational(void) {
	NodeKind table[TK_COUNT] = {
		['<'] = ND_LT,
		['>'] = ND_GT,
		[TK_GE] = ND_GE,
		[TK_LE] = ND_LT,
	};
	return parse_binary_left(table, &parse_shift);
}

static Node *parse_equality(void) {
	NodeKind table[TK_COUNT] = {
		[TK_EQ] = ND_EQ,
		[TK_NE] = ND_NE,
	};
	return parse_binary_left(table, &parse_relational);
}

static Node *parse_and(void) {
	NodeKind table[TK_COUNT] = {
		['&'] = ND_AND,
	};
	return parse_binary_left(table, &parse_equality);
}

static Node *parse_xor(void) {
	NodeKind table[TK_COUNT] = {
		['^'] = ND_XOR,
	};
	return parse_binary_left(table, &parse_and);
}

static Node *parse_or(void) {
	NodeKind table[TK_COUNT] = {
		['|'] = ND_OR,
	};
	return parse_binary_left(table, &parse_xor);
}

static Node *parse_land(void) {
	NodeKind table[TK_COUNT] = {
		[TK_LAND] = ND_LAND,
	};
	return parse_binary_left(table, &parse_or);
}

static Node *parse_lor(void) {
	NodeKind table[TK_COUNT] = {
		[TK_LOR] = ND_LOR,
	};
	return parse_binary_left(table, &parse_land);
}

static Node *parse_assign(void) {
	NodeKind table[TK_COUNT] = {
		['='] = ND_ASSIGN,
		[TK_ADD_A] = ND_ADD_A,
		[TK_AND_A] = ND_AND_A,
		[TK_MOD_A] = ND_MOD_A,
		[TK_MUL_A] = ND_MUL_A,
		[TK_OR_A] =  ND_OR_A,
		[TK_SHL_A] = ND_SHL_A,
		[TK_SHR_A] = ND_SHR_A,
		[TK_SUB_A] = ND_SUB_A,
		[TK_XOR_A] = ND_XOR_A,
	};
	// TODO do this in a separate pass
	Node *nd = parse_binary_right(table, &parse_lor);
	switch(nd->kind) {
		case ND_ADD_A:
		case ND_AND_A:
		case ND_MOD_A:
		case ND_MUL_A:
		case ND_OR_A:
		case ND_SHL_A:
		case ND_SHR_A:
		case ND_SUB_A:
		case ND_XOR_A:
			--nd->kind;
			RIGHT(nd) = node_cpy(nd);
			nd->kind = ND_ASSIGN;
	}
	return nd;
}

static Node *parse_ternary(void) {
	Node *nd = parse_assign();
	if(tk->kind == '?') {
		nd = node_wrap(ND_IF, nd);
		++tk;
		IF_BODY(nd) = parse_expr();
		token_expect(':');
		IF_ELSE(nd) = parse_ternary();
	}
	return nd;
}

static Node *parse_comma(void) {
	NodeKind table[TK_COUNT] = {
		[','] = ND_COMMA,
	};
	return parse_binary_right(table, &parse_ternary);
}

static Node *parse_expr(void) {
	NodeKind get_node_kind[TK_COUNT] = {
		[TK_BREAK] = ND_BREAK,
		[TK_CONTINUE] = ND_CONTINUE,
	};
	Node *nd;
	NodeKind kind = get_node_kind[tk->kind];
	switch(kind) {
		case 0:
			return parse_comma();
		default:
			nd = node_new(kind);
			++tk;
			return nd;
	}
}

static Node *parse_expr_stmt(void) {
	Node *nd = parse_expr();
	token_expect(';');
	return nd;
}

static Node *parse_block(void) {
	char head[node_size(ND_BLOCK)];
	Node *block = node_new(ND_BLOCK), *nd = (Node *)head;
	token_expect('{');
	while(!token_consume('}'))
		nd = SECOND(nd) = parse_stmt();
	FIRST(block) = SECOND((Node *)head);
	return block;
}

static Node *parse_specifier(void) {
	// TODO types
	// for now everything is an int
	Node *nd = node_new(ND_SPECIFIER);
	token_expect(TK_TYPE);
	return nd;
}

static Node *parse_type(void) {
	return token_consume('*') ? node_new(ND_PTR)
		: NULL;
}

static Node *parse_param(void) {
	token_expect(TK_TYPE);
	Node *nd = node_new(ND_PARAM);
	token_expect(TK_ID);
	return nd;
}

static Node *parse_params(void) {
	char head[node_size(ND_PARAM)];
	Node *nd = (Node *)head;
	token_expect('(');
	if (token_consume(')'))
		return NULL;
	do
		nd = FIRST(nd) = parse_param();
	while (token_consume(','));
	token_expect(')');
	return FIRST((Node *)head);
}

static Node *parse_fn(void) {
	Node *nd = node_new(ND_FN);
	FN_SPEC(nd) = parse_specifier();
	FN_TYPE(nd) = parse_type();
	FN_VAR(nd) = parse_var();
	FN_PARAMS(nd) = parse_params();
	if (!token_consume(';'))
		FN_BODY(nd) = parse_block();
	return nd;
}

static Node *parse_init_declarator(void) {
	Node *nd = parse_var();
	if (tk->kind == '=') {
		nd = node_wrap(ND_ASSIGN, nd);
		++tk;
		SECOND(nd) = parse_assign();
	}
	return nd;
}

static Node *parse_declarator(void) {
	Node *nd = node_new(ND_DECLARATOR);
	DECL_TYPE(nd) = parse_type();
	DECL_VAR(nd) = parse_init_declarator();
	return DECL_VAR(nd)->kind == ND_FN ? DECL_VAR(nd)
		: nd;
}

static Node *parse_declarators(void) {
	char head[node_size(ND_DECLARATOR)];
	Node *nd = (Node *)head;
	do
		nd = DECL_NEXT(nd) = parse_declarator();
	while (token_consume(','));
	if (nd->kind != ND_FN)
		token_expect(';');
	return DECL_NEXT((Node *)head);
}

static Node *parse_declaration(void) {
	Node *nd = node_new(ND_DECLARATION);
	FIRST(nd) = parse_specifier();
	SECOND(nd) = parse_declarators();
	return SECOND(nd)->kind == ND_FN ? SECOND(nd)
		: nd;
}

static Node *parse_unit(void) {
	return (is_fn() ? parse_fn : parse_declaration)();
}

static Node *parse_stmt(void) {
	Node *nd;
	switch(tk->kind) {
		case ';':
			++tk;
			return NULL;
		case '{':
			nd = parse_block();
			break;
		case TK_IF:
			nd = node_new(ND_IF);
			++tk;
			token_expect('(');
			IF_COND(nd) = parse_expr();
			token_expect(')');
			IF_BODY(nd) = parse_stmt();
			if(token_consume(TK_ELSE))
				IF_ELSE(nd) = parse_stmt();
			break;
		case TK_FOR:
			nd = node_new(ND_FOR);
			++tk;
			token_expect('(');
			if(!token_consume(';'))
				FOR_INIT(nd) = (is_specifier() ? parse_declaration : parse_expr_stmt)();
			if(!token_consume(';'))
				FOR_COND(nd) = parse_expr_stmt();
			if(!token_consume(')')) {
				FOR_INC(nd) = parse_expr();
				token_expect(')');
			}
			FOR_BODY(nd) = parse_stmt();
			break;
		case TK_WHILE:
			nd = node_new(ND_WHILE);
			++tk;
			token_expect('(');
			WHILE_COND(nd) = parse_expr();
			token_expect(')');
			WHILE_BODY(nd) = parse_stmt();
			break;
		case TK_DO:
			nd = node_new(ND_DO);
			++tk;
			DO_BODY(nd) = parse_stmt();
			token_expect(TK_WHILE);
			token_expect('(');
			DO_COND(nd) = parse_expr();
			token_expect(')');
			token_expect(';');
			break;
		case TK_GOTO:
			nd = node_new(ND_GOTO);
			++tk;
			FIRST(nd) = node_new(ND_LABEL);
			token_expect(TK_ID);
			token_expect(';');
			break;
		case TK_RET:
			nd = node_new(ND_RET);
			++tk;
			FIRST(nd) = parse_expr_stmt();
			break;
		default:
			if (tk[1].kind == ':') {
				nd = node_new(ND_LABEL);
				++tk;
				token_expect(':');
				FIRST(nd) = parse_stmt();
			} else if(is_specifier())
				nd = parse_declaration();
			else
				nd = parse_expr_stmt();
			break;
	}
	return node_wrap(ND_STMT, nd);
}

static Ast *parse(Token *tokens) {
	tk = tokens;
	Ast *ast = malloc(sizeof(Ast));
	ast->vars = vec_new(1, sizeof(Node *));
	ast->fns = vec_new(1, sizeof(Node *));
	while(tk->kind != '\0') {
		Node *nd = parse_unit();
		if (nd->kind == ND_FN)
			vec_push_back(ast->fns, nd);
		else
			vec_push_back(ast->vars, nd);
	}
	return ast;
}
