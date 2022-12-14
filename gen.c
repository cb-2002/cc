#include "analyze.c"

// globals

static unsigned count = 0, loop_label = 0;

// generation functions

static void gen_node(GenState *gs, Node *nd);
static void gen_addr(GenState *gs, Node *nd) {
	Var *v = var_find(gs, nd->tk);
	if (!v)
		error_node(nd, "undefined variable\n");
	printf("; %d", v->offset * 8);
	print_node(nd);
	printf("\n");
	switch(nd->kind) {
		case ND_VAR:
			if(v->offset) {
				printf("mov rax, rbp\n");
				printf("sub rax, %d\n", 8 * v->offset);
			} else {
				printf("mov rax, ");
				print_node(nd);
				printf("\n");
			}
			return;
		case ND_DEREF:
			gen_node(gs, FIRST(nd));
			return;
		default:
			error_node(nd, "expected lvar\n");
	}
}

static void gen_binary_args(GenState *gs, Node *nd) {
	gen_node(gs, RIGHT(nd));
	printf("push rax\n");
	gen_node(gs, LEFT(nd));
	printf("pop rcx\n");
}

static void gen_binary(GenState *gs, Node *nd, char *s) {
	gen_binary_args(gs, nd);
	printf("%s rax, rcx\n", s);
}

static void gen_div(GenState *gs, Node *nd) {
	gen_binary_args(gs, nd);
	printf("cqo\n");
	printf("idiv rcx\n");
}

static void gen_cond(GenState *gs, Node *nd, char *s) {
	gen_binary(gs, nd, "cmp");
	printf("%s al\n", s);
}

static void gen_postfix(GenState *gs, Node *nd, char *s) {
	gen_addr(gs, FIRST(nd));
	printf("mov rcx, [rax]\n");
	printf("%s DWORD [rax]\n", s);
	printf("mov rax, rcx\n");
}

static void gen_prefix(GenState *gs, Node *nd, char *s) {
	gen_addr(gs, FIRST(nd));
	printf("%s DWORD [rax]\n", s);
	printf("mov rax, [rax]\n");
}

static void gen_logical(GenState *gs, Node *nd, char *s) {
	unsigned x = count++;
	gen_node(gs, FIRST(nd));
	printf("cmp rax, 0\n");
	printf("%s .false%d\n", s, x);
	gen_node(gs, RIGHT(nd));
	printf("cmp rax, 0\n");
	printf(".false%d:\n", x);
	printf("setne al\n");
}

static void gen_shift(GenState *gs, Node *nd, char *s) {
	gen_binary_args(gs, nd);
	printf("%s rax, cl\n", s);
}

static void gen_declarator_local(GenState *gs, Node *spec, Node *ptr, Node *var) {
	define_local(gs, spec, ptr, var->kind == ND_ASSIGN ? FIRST(var) : var);
	gen_node(gs, var);
}

// TODO different node kind for assigned declarator
static void gen_declarator_global(GenState *gs, Node *spec, Node *ptr, Node *var) {
	define_global(gs, spec, ptr, var->kind == ND_ASSIGN ? FIRST(var) : var);
	switch (var->kind) {
		case ND_VAR:
			print_node(var);
			printf(" dw 0\n");
			break;
		case ND_ASSIGN:
			Node *val = SECOND(var);
			var = FIRST(var);
			print_node(var);
			printf(" dw ");
			print_node(val);
			printf("\n");
			break;
		// function prototype
		case ND_FN:
			printf("extern ");
			print_token(FN_VAR(var)->tk);
			printf("\n");
			break;
		default:
			quit();
	}
}

typedef void GenDeclarator(GenState *gs, Node *spec, Node *ptr, Node *var);
static void gen_declarators(GenState *gs, Node *spec, Node *nd, GenDeclarator *gen_declarator) {
	do {
		gen_declarator(gs, spec, DECL_PTR(nd), DECL_VAR(nd));
	} while (nd = DECL_NEXT(nd));
}

static void gen_declaration(GenState *gs, Node *nd, GenDeclarator *gen_declarator) {
	gen_declarators(gs, FIRST(nd), SECOND(nd), gen_declarator);
}

static void gen_node(GenState *gs, Node *nd) {
	if(!nd)
		return;
	printf("; ");
	print_node(nd);
	printf("\n");
	switch(nd->kind) {
		case ND_INT:
			printf("mov rax, ");
			print_node(nd);
			printf("\n");
			return;
		case ND_ADD:
			gen_binary(gs, nd, "add");
			return;
		case ND_SUB:
			gen_binary(gs, nd, "sub");
			return;
		case ND_MUL:
			gen_binary(gs, nd, "imul");
			return;
		case ND_DIV:
			gen_div(gs, nd);
			return;
		case ND_MOD:
			gen_div(gs, nd);
			printf("mov rax, rdx\n");
			return;
		case ND_AND:
			gen_binary(gs, nd, "and");
			return;
		case ND_XOR:
			gen_binary(gs, nd, "xor");
			return;
		case ND_OR:
			gen_binary(gs, nd, "or");
			return;
		case ND_NEG:
			gen_node(gs, FIRST(nd));
			printf("neg rax\n");
			return;
		case ND_NOT:
			gen_node(gs, FIRST(nd));
			printf("not rax\n");
			return;
		case ND_LNOT:
			gen_node(gs, FIRST(nd));
			printf("cmp rax, 0\n");
			printf("mov rax, 0\n");
			printf("sete al\n");
			return;
		case ND_LT:
			gen_cond(gs, nd, "setl");
			return;
		case ND_GT:
			gen_cond(gs, nd, "setg");
			return;
		case ND_SHL:
			gen_shift(gs, nd, "shl");
			return;
		case ND_SHR:
			gen_shift(gs, nd, "shr");
			return;
		case ND_EQ:
			gen_cond(gs, nd, "sete");
			return;
		case ND_NE:
			gen_cond(gs, nd, "setne");
			return;
		case ND_LAND:
			gen_logical(gs, nd, "je");
			return;
		case ND_LOR:
			gen_logical(gs, nd, "jne");
			return;
		case ND_IF:
		case ND_TERNARY:
		{
			unsigned x = count++;
			gen_node(gs, IF_COND(nd));
			printf("cmp rax, 0\n");
			printf("je .false%d\n", x);
			gen_node(gs, IF_BODY(nd));
			printf("jmp .true%d\n", x);
			printf(".false%d:\n", x);
			gen_node(gs, IF_ELSE(nd));
			printf(".true%d:\n", x);
			return;
		}
		case ND_STMT:
			if (FIRST(nd)->kind == ND_BLOCK) {
				scope_enter(gs);
				gen_node(gs, FIRST(nd));
				scope_exit(gs);
			} else
				gen_node(gs, FIRST(nd));
			gen_node(gs, SECOND(nd));
			return;
		case ND_COMMA:
			gen_node(gs, FIRST(nd));
			gen_node(gs, SECOND(nd));
			return;
		case ND_ASSIGN:
			gen_addr(gs, FIRST(nd));
			printf("push rax\n");
			gen_node(gs, SECOND(nd));
			printf("pop rcx\n");
			printf("mov [rcx], rax\n");
			return;
		case ND_VAR:
			gen_addr(gs, nd);
			printf("mov rax, [rax]\n");
			return;
		case ND_ADDR:
			gen_addr(gs, FIRST(nd));
			return;
		case ND_DEREF:
			gen_node(gs, FIRST(nd));
			printf("mov rax, [rax]\n");
			return;
		case ND_BLOCK:
		{
			// TODO set stack size for arbitrary number of local variables
			unsigned x = 16; // reserve space for 16 local variables
			printf("sub rsp, %d\n", 8 * x);
			gen_node(gs, FIRST(nd));
			printf("add rsp, %d\n", 8 * x);
			return;
		}
		case ND_RET:
			gen_node(gs, FIRST(nd));
			printf("mov rsp, rbp\n");
			printf("pop rbp\n");
			printf("ret\n");
			return;
		case ND_FOR:
		case ND_WHILE:
		{
			unsigned x = loop_label = count++;
			gen_node(gs, FOR_INIT(nd));
			printf(".start%d:\n", x);
			if(FOR_COND(nd)) {
				gen_node(gs, FOR_COND(nd));
				printf("cmp rax, 0\n");
				printf("je .end%d\n", x);
			}
			gen_node(gs, FOR_BODY(nd));
			printf(".cont%d:\n", x);
			gen_node(gs, FOR_INC(nd));
			printf("jmp .start%d\n", x);
			printf(".end%d:\n", x);
			loop_label = x;
			return;
		}
		case ND_DO:
		{
			unsigned x = loop_label = count++;
			printf(".start%d:\n", x);
			gen_node(gs, DO_BODY(nd));
			gen_node(gs, DO_COND(nd));
			printf("cmp rax, 0\n");
			printf("jne .start%d\n", x);
			loop_label = x;
			return;
		}
		case ND_BREAK:
			printf("jmp .end%d\n", loop_label);
			return;
		case ND_CONTINUE:
			printf("jmp .cont%d\n", loop_label);
			return;
		case ND_LABEL:
			printf(".");
			print_node(nd);
			printf(":\n");
			gen_node(gs, FIRST(nd));
			return;
		case ND_GOTO:
			printf("jmp .");
			print_token(FIRST(nd)->tk);
			printf("\n");
			return;
		case ND_FN:
			scope_enter(gs);
			define_params(gs, FN_PARAMS(nd));
			print_token(FN_VAR(nd)->tk);
			printf(":\n");
			printf("push rbp\n");
			printf("mov rbp, rsp\n");
			gen_node(gs, FN_BODY(nd));
			printf("pop rbp\n");
			printf("ret\n");
			scope_exit(gs);
			return;
		case ND_CALL:
			gen_node(gs, SECOND(nd));
			printf("call ");
			print_token(FIRST(nd)->tk);
			printf("\n");
			printf("add rsp, %d\n", 8 * node_depth(SECOND(nd)));
			return;
		case ND_ARG:
			if(SECOND(nd))
				gen_node(gs, SECOND(nd));
			gen_node(gs, FIRST(nd));
			printf("push rax\n");
			return;
		case ND_POST_INC:
			gen_postfix(gs, nd, "inc");
			return;
		case ND_POST_DEC:
			gen_postfix(gs, nd, "dec");
			return;
		case ND_PRE_INC:
			gen_prefix(gs, nd, "inc");
			return;
		case ND_PRE_DEC:
			gen_prefix(gs, nd, "dec");
			return;
		case ND_DECLARATION:
			gen_declaration(gs, nd, gen_declarator_local);
			return;
		default:
			error_node(nd, "unknown node type\n");
	}
}

static void gen(Ast *ast) {
	GenState *gs = &(GenState){
		.scope = scope_new(),
	};
	printf("bits 64\n");
	printf("default rel\n");
	printf("global main\n");
	printf("section .data\n");
	for (unsigned i = 0; i < vec_len(ast->vars); ++i)
		gen_declaration(gs, ast->vars[i], gen_declarator_global);
	printf("section .text\n");
	for (int i = 0; i < vec_len(ast->fns); i++)
		gen_node(gs, ast->fns[i]);
}
