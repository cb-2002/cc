enum NodeKind {
	ND_UNKNOWN,
	ND_ADDR,
	
	ND_ADD,
	ND_ADD_A,
	ND_AND,
	ND_AND_A,
	ND_ARG,
	ND_ASSIGN,
	ND_BLOCK,
	ND_BREAK,
	ND_CALL,
	ND_COMMA,
	ND_CONTINUE,
	ND_DECLARATION,
	ND_DEREF,
	ND_DIV,
	ND_DO,
	ND_EQ,
	ND_FN,
	ND_FOR,
	ND_GE,
	ND_GOTO,
	ND_GT,
	ND_IF,
	ND_INT,
	ND_LABEL,
	ND_LAND,
	ND_LE,
	ND_LNOT,
	ND_LOR,
	ND_LT,
	ND_MOD,
	ND_MOD_A,
	ND_MUL,
	ND_MUL_A,
	ND_NE,
	ND_NEG,
	ND_NOT,
	ND_OR,
	ND_OR_A,
	ND_PARAM,
	ND_POST_DEC,
	ND_POST_INC,
	ND_PRE_DEC,
	ND_PRE_INC,
	ND_RET,
	ND_SHL,
	ND_SHL_A,
	ND_SHR,
	ND_SHR_A,
	ND_STMT,
	ND_SUB,
	ND_SUB_A,
	ND_TERNARY,
	ND_VAR,
	ND_WHILE,
	ND_XOR,
	ND_XOR_A,
};
typedef enum NodeKind NodeKind;

typedef struct Var Var;
struct Var {
	int offset;
	Var *next;
	Token *tk;
};

typedef struct Scope Scope;
struct Scope {
	Scope *parent;
	Var *vars;
	unsigned offset;
};

static Scope *scope_new(unsigned offset) {
	Scope *scope = malloc(sizeof(scope));
	scope->parent = NULL;
	scope->vars = NULL;
	scope->offset = offset;
	return scope;
}

typedef struct ParseState ParseState;
struct ParseState {
	Token *tk;
	Scope *scope;
	List vars, fns;
};

static Var *var_new(ParseState *ps) {
	Var *v = malloc(sizeof(Var));
	v->tk = ps->tk;
	return v;
}

static Var *var_find(ParseState *ps, Token *tk) {
	for(Scope *scope = ps->scope; scope; scope = scope->parent)
		for(Var *v = scope->vars; v; v = v->next)
			if(token_cmp(v->tk, tk))
				return v;
	return NULL;
}

static void scope_enter(ParseState *ps) {
	Scope *scope = scope_new(ps->scope->offset);
	scope->parent = ps->scope;
	ps->scope = scope;
}

static void scope_exit(ParseState *ps) {
	ps->scope = ps->scope->parent;
}

typedef struct Node Node;
struct Node {
	NodeKind kind;
	Token *tk;
	Node *children[4];
	void *data;
};

#define FIRST(nd) nd->children[0]
#define SECOND(nd) nd->children[1]
#define THIRD(nd) nd->children[2]
#define FORTH(nd) nd->children[3]

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

#define FN_PARAMS FIRST
#define FN_BODY SECOND

static Node *node_new(ParseState *ps, NodeKind kind) {
	Node *nd = calloc(1,sizeof(Node));
	nd->kind = kind;
	nd->tk = ps->tk;
	return nd;
}

static Node *node_cpy(Node *nd) {
	Node *tmp = malloc(sizeof(Node));
	*tmp = *nd;
	return tmp;
}

static Node *node_wrap(ParseState *ps, NodeKind kind, Node *left) {
	Node *nd = node_new(ps, kind);
	LEFT(nd) = left;
	return nd;
}

static unsigned node_depth(Node *nd) {
	return nd ? 1 + node_depth(SECOND(nd)) : 0;
}

static unsigned get_stack_size(Node *nd) {
	assert(nd->kind == ND_BLOCK);
	return ((Scope *)nd->data)->offset;
}

// parsing functions

static void token_next(ParseState *ps) {
	++ps->tk;
}

static bool token_equals(ParseState *ps, TokenKind k) {
	return ps->tk->kind == k;
}

static bool token_consume(ParseState *ps, TokenKind k) {
	if(!token_equals(ps, k))
		return false;
	token_next(ps);
	return true;
}

static void token_expect(ParseState *ps, TokenKind k) {
	if(!token_consume(ps, k))
		error_token(ps->tk, "expected '%c'\n", (char)k);
}

static bool is_specifier(ParseState *ps) {
	return token_equals(ps, TK_TYPE);
}

// recursive function declarations

typedef Node *ParseFn(ParseState *);

ParseFn(parse_expr); 
ParseFn(parse_assign);
ParseFn(parse_stmt); 

static Node *parse_binary_left(
	ParseState *ps,
	NodeKind get_node_kind[TK_COUNT],
	ParseFn *parse_child
) {
	Node *nd = parse_child(ps);
	NodeKind kind;
	while(kind = get_node_kind[ps->tk->kind]) {
		nd = node_wrap(ps, kind, nd);
		token_next(ps);
		SECOND(nd) = parse_child(ps);
	}
	return nd;
}

static Node *parse_binary_right(
	ParseState *ps,
	NodeKind get_node_kind[TK_COUNT],
	ParseFn *parse_child
) {
	Node *nd = parse_child(ps);
	NodeKind kind = get_node_kind[ps->tk->kind];
	if(kind) {
		nd = node_wrap(ps, kind, nd);
		token_next(ps);
		SECOND(nd) = parse_binary_left(ps, get_node_kind, parse_child);
	}
	return nd;
}

static Node *parse_atom(ParseState *ps) {
	Node *nd;
	switch(ps->tk->kind) {
		case TK_INT:
			nd = node_new(ps, ND_INT);
			break;
		case '(':
			token_next(ps);
			nd = parse_expr(ps);
			token_expect(ps, ')');
			return nd;
		case TK_ID:
			nd = node_new(ps, ND_VAR);
			Var *v = var_find(ps, nd->tk);
			if(!v)
				error_token(ps->tk, "unknown var\n");
			nd->data = v;
			break;
		default:
			error_token(ps->tk, "syntax\n");
	}
	token_next(ps);
	return nd;
}

static Node *parse_args(ParseState *ps) {
	Node *nd = node_new(ps, ND_ARG);
	FIRST(nd) = parse_assign(ps);
	if(token_consume(ps, ','))
		SECOND(nd) = parse_args(ps);
	else
		token_expect(ps, ')');
	return nd;
}

static Node *parse_postfix(ParseState *ps) {
	NodeKind get_node_kind[TK_COUNT] = {
		['('] = ND_CALL,
		[TK_INC] = ND_POST_INC,
		[TK_DEC] = ND_POST_DEC,
	};
	Node *nd = parse_atom(ps);
	NodeKind kind = get_node_kind[ps->tk->kind];
	switch(kind) {
		case 0:
			return nd;
		case ND_CALL:
			nd = node_wrap(ps, kind, nd);
			token_next(ps);
			if(!token_consume(ps, ')'))
				SECOND(nd) = parse_args(ps);
			return nd;
		default:
			nd = node_wrap(ps, kind, nd);
			token_next(ps);
			return nd;
	}
}

static Node *parse_prefix(ParseState *ps) {
	NodeKind get_node_kind[TK_COUNT]  = {
		['!'] = ND_LNOT,
		['&'] = ND_ADDR,
		['*'] = ND_DEREF,
		['-'] = ND_NEG,
		['~'] = ND_NOT,
		[TK_DEC] = ND_PRE_DEC,
		[TK_INC] = ND_PRE_INC,
	};
	token_consume(ps, '+'); // ignoring unary + as it does nothing
	NodeKind kind = get_node_kind[ps->tk->kind];
	if(kind) {
		Node *nd = node_new(ps, kind);
		token_next(ps);
		FIRST(nd) = parse_prefix(ps);
		return nd;
	} 
	return parse_postfix(ps); 
}

static Node *parse_mul(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		['*'] = ND_MUL,
		['/'] = ND_DIV,
		['%'] = ND_MOD,
	};
	return parse_binary_left(ps, table, &parse_prefix);
}

static Node *parse_add(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		['+'] = ND_ADD,
		['-'] = ND_SUB,
	};
	return parse_binary_left(ps, table, &parse_mul);
}

static Node *parse_shift(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		[TK_SHR] = ND_SHR,
		[TK_SHL] = ND_SHL,
	};
	return parse_binary_left(ps, table, &parse_add);
}

static Node *parse_relational(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		['<'] = ND_LT,
		['>'] = ND_GT,
		[TK_GE] = ND_GE,
		[TK_LE] = ND_LT,
	};
	return parse_binary_left(ps, table, &parse_shift);
}

static Node *parse_equality(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		[TK_EQ] = ND_EQ,
		[TK_NE] = ND_NE,
	};
	return parse_binary_left(ps, table, &parse_relational);
}

static Node *parse_and(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		['&'] = ND_AND,
	};
	return parse_binary_left(ps, table, &parse_equality);
}

static Node *parse_xor(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		['^'] = ND_XOR,
	};
	return parse_binary_left(ps, table, &parse_and);
}

static Node *parse_or(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		['|'] = ND_OR,
	};
	return parse_binary_left(ps, table, &parse_xor);
}

static Node *parse_land(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		[TK_LAND] = ND_LAND,
	};
	return parse_binary_left(ps, table, &parse_or);
}

static Node *parse_lor(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		[TK_LOR] = ND_LOR,
	};
	return parse_binary_left(ps, table, &parse_land);
}

static Node *parse_assign(ParseState *ps) {
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
	Node *nd = parse_binary_right(ps, table, &parse_lor);
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

static Node *parse_ternary(ParseState *ps) {
	Node *nd = parse_assign(ps);
	if(token_equals(ps, '?')) {
		nd = node_wrap(ps, ND_IF, nd);
		token_next(ps);
		IF_BODY(nd) = parse_expr(ps);
		token_expect(ps, ':');
		IF_ELSE(nd) = parse_ternary(ps);
	}
	return nd;
}

static Node *parse_comma(ParseState *ps) {
	NodeKind table[TK_COUNT] = {
		[','] = ND_COMMA,
	};
	return parse_binary_right(ps, table, &parse_ternary);
}

static Node *parse_expr(ParseState *ps) {
	// TODO use get_node_kind table here
	Node *nd;
	switch(ps->tk->kind) {
		case TK_BREAK:
			nd = node_new(ps, ND_BREAK);
			break;
		case TK_CONTINUE:
			nd = node_new(ps, ND_CONTINUE);
			break;
		default:
			return parse_comma(ps);
	}
	token_next(ps);
	return nd;
}

static Node *parse_expr_stmt(ParseState *ps) {
	Node *nd = parse_expr(ps);
	token_expect(ps, ';');
	return nd;
}

static Node *parse_block(ParseState *ps) {
	// recursion here instead of using stack
	Node *block = node_new(ps, ND_BLOCK), *head = &(Node){0}, *nd = head;
	token_expect(ps, '{');
	block->data = ps->scope;
	while(!token_consume(ps, '}'))
		nd = SECOND(nd) = parse_stmt(ps);
	FIRST(block) = SECOND(head);
	return block;
}

static Node *parse_params(ParseState *ps) {
	token_expect(ps, TK_TYPE);
	Node *nd = node_new(ps, ND_PARAM);
	nd->data = var_new(ps);
	token_expect(ps, TK_ID);
	if(token_consume(ps, ','))
		FIRST(nd) = parse_params(ps);
	else
		token_expect(ps, ')');
	return nd;
}

static void define_var(Scope *scope, Var *v, int offset) {
	v->offset = scope->parent ? offset : 0; // 0 used for globals
	v->next = scope->vars;
	scope->vars = v;
}

static void define_params(Scope *scope, Node *nd, int offset) {
	if(!nd)
		return;
	define_var(scope, nd->data, --offset);
	define_params(scope, FIRST(nd), offset);
}

static Node *parse_fn(ParseState *ps, Node *nd) {
	assert(nd->kind == ND_FN);
	scope_enter(ps);
	define_params(ps->scope, FN_PARAMS(nd), -1);
	FN_BODY(nd) = parse_block(ps);
	scope_exit(ps);
	return nd;
}

static Node *parse_declarator_list(ParseState *ps) {
	Node *nd = node_new(ps, ND_VAR);
	nd->data = var_new(ps);
	define_var(ps->scope, nd->data, ++ps->scope->offset);
	token_expect(ps, TK_ID);
	if(token_equals(ps, '=')) {
		nd = node_wrap(ps, ND_ASSIGN, nd);
		token_next(ps);
		SECOND(nd) = parse_assign(ps);
	} else if(token_consume(ps, '(')) {
		nd->kind = ND_FN;
		if(!token_consume(ps, ')'))
			FN_PARAMS(nd) = parse_params(ps);
		if(token_equals(ps, '{'))
			return parse_fn(ps, nd);
	}
	nd = node_wrap(ps, ND_DECLARATION, nd);
	if(token_consume(ps, ','))
		SECOND(nd) = parse_declarator_list(ps);
	else
		token_expect(ps, ';');
	return nd;
}

static Node *parse_declaration(ParseState *ps) {
	// TODO types
	// for now everything is an int
	token_expect(ps, TK_TYPE);
	return parse_declarator_list(ps);
}

// TODO move wrapping to parse_block
static Node *parse_stmt(ParseState *ps) {
	Node *stmt = node_new(ps, ND_STMT), *nd;
	switch(ps->tk->kind) {
		case ';':
			token_next(ps);
			return stmt;
		case '{':
			scope_enter(ps);
			nd = parse_block(ps);
			scope_exit(ps);
			break;
		case TK_IF:
			nd = node_new(ps, ND_IF);
			token_next(ps);
			token_expect(ps, '(');
			IF_COND(nd) = parse_expr(ps);
			token_expect(ps, ')');
			IF_BODY(nd) = parse_stmt(ps);
			if(token_consume(ps, TK_ELSE))
				IF_ELSE(nd) = parse_stmt(ps);
			break;
		case TK_FOR:
			nd = node_new(ps, ND_FOR);
			token_next(ps);
			token_expect(ps, '(');
			if(!token_consume(ps, ';'))
				FOR_INIT(nd) = (is_specifier(ps) ? parse_declaration : parse_expr_stmt)(ps);
			if(!token_consume(ps, ';'))
				FOR_COND(nd) = parse_expr_stmt(ps);
			if(!token_consume(ps, ')')) {
				FOR_INC(nd) = parse_expr(ps);
				token_expect(ps, ')');
			}
			FOR_BODY(nd) = parse_stmt(ps);
			break;
		case TK_WHILE:
			nd = node_new(ps, ND_WHILE);
			token_next(ps);
			token_expect(ps, '(');
			WHILE_COND(nd) = parse_expr(ps);
			token_expect(ps, ')');
			WHILE_BODY(nd) = parse_stmt(ps);
			break;
		case TK_DO:
			nd = node_new(ps, ND_DO);
			token_next(ps);
			DO_BODY(nd) = parse_stmt(ps);
			token_expect(ps, TK_WHILE);
			token_expect(ps, '(');
			DO_COND(nd) = parse_expr(ps);
			token_expect(ps, ')');
			break;
		case TK_GOTO:
			nd = node_new(ps, ND_GOTO);
			token_next(ps);
			FIRST(nd) = node_new(ps, ND_LABEL);
			token_expect(ps, TK_ID);
			token_expect(ps, ';');
			break;
		case TK_RET:
			nd = node_new(ps, ND_RET);
			token_next(ps);
			FIRST(nd) = parse_expr_stmt(ps);
			break;
		default:
			if (ps->tk[1].kind == ':') {
				nd = node_new(ps, ND_LABEL);
				token_next(ps);
				token_expect(ps, ':');
				FIRST(nd) = parse_stmt(ps);
			} else if(is_specifier(ps))
				nd = parse_declaration(ps);
			else
				nd = parse_expr_stmt(ps);
			break;
	}
	FIRST(stmt) = nd;
	return stmt;
}

static ParseState parse(Token *tokens) {
	ParseState *ps = &(ParseState){
		.tk = tokens,
		.scope = scope_new(0),
		.fns = list_new(),
		.vars = list_new(),
	};
	Node *nd;
	while(ps->tk->kind != '\0') {
		nd = parse_declaration(ps);
		list_push_back(nd->kind == ND_FN ? &ps->fns : &ps->vars, nd);
	}
	assert(!ps->scope->parent);
	return *ps;
}
