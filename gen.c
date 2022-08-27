static void gen_node(Node *);

static void gen_addr(Node *nd) {
	printf("; %d", ((Var *)nd->data)->offset * 8);
	print_token(nd->tk);
	printf("\n");
	switch(nd->kind) {
		case ND_VAR:
			int offset = ((Var *)nd->data)->offset;
			if(offset) {
				printf("mov rax, rbp\n");
				printf("sub rax, %d\n", 8 * offset);
			} else {
				printf("mov rax, ");
				print_token(nd->tk);
				printf("\n");
			}
			return;
		case ND_DEREF:
			gen_node(FIRST(nd));
			return;
		default:
			error_token(nd->tk, "expected lvar\n");
	}
}

static void gen_node(Node *nd) {
	static unsigned count = 0, loop_label = 0;
	if(!nd)
		return;
	printf("; ");
	print_token(nd->tk);
	printf("\n");
	switch(nd->kind) {
		case ND_INT:
			printf("mov rax, ");
			print_token(nd->tk);
			printf("\n");
			return;
		case ND_ADD:
			gen_node(SECOND(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("add rax, rcx\n");
			return;
		case ND_SUB:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("sub rax, rcx\n");
			return;
		case ND_MUL:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("imul rax, rcx\n");
			return;
		case ND_DIV:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("cqo\n");
			printf("idiv rcx\n");
			printf("\n");
			return;
		case ND_MOD:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("cqo\n");
			printf("idiv rcx\n");
			printf("mov rax, rdx\n");
			printf("\n");
			return;
		case ND_AND:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("and rax, rcx\n");
			return;
		case ND_XOR:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("xor rax, rcx\n");
			return;
		case ND_OR:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("or rax, rcx\n");
			return;
		case ND_NEG:
			gen_node(FIRST(nd));
			printf("neg rax\n");
			return;
		case ND_NOT:
			gen_node(FIRST(nd));
			printf("not rax\n");
			return;
		case ND_LNOT:
			gen_node(FIRST(nd));
			printf("cmp rax, 0\n");
			printf("mov rax, 0\n");
			printf("sete al\n");
			return;
		case ND_LT:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("cmp rax, rcx\n");
			printf("setl al\n");
			return;
		case ND_GT:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("cmp rax, rcx\n");
			printf("setg al\n");
			return;
		case ND_SHL:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("shl rax, cl\n");
			return;
		case ND_SHR:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("shr rax, cl\n");
			return;
		case ND_EQ:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("cmp rax, rcx\n");
			printf("sete al\n");
			return;
		case ND_NE:
			gen_node(RIGHT(nd));
			printf("push rax\n");
			gen_node(FIRST(nd));
			printf("pop rcx\n");
			printf("cmp rax, rcx\n");
			printf("setne al\n");
			return;
		case ND_LAND:
		{
			unsigned x = count++;
			gen_node(FIRST(nd));
			printf("cmp rax, 0\n");
			printf("je .false%d\n", x);
			gen_node(RIGHT(nd));
			printf("cmp rax, 0\n");
			printf(".false%d:\n", x);
			printf("setne al\n");
			return;
		}
		case ND_LOR:
		{
			unsigned x = count++;
			gen_node(FIRST(nd));
			printf("cmp rax, 0\n");
			printf("jne .false%d\n", x);
			gen_node(SECOND(nd));
			printf("cmp rax, 0\n");
			printf(".false%d:\n", x);
			printf("setne al\n");
			return;
		}
		case ND_IF:
		case ND_TERNARY:
		{
			unsigned x = count++;
			gen_node(IF_COND(nd));
			printf("cmp rax, 0\n");
			printf("je .false%d\n", x);
			gen_node(IF_BODY(nd));
			printf("jmp .true%d\n", x);
			printf(".false%d:\n", x);
			gen_node(IF_ELSE(nd));
			printf(".true%d:\n", x);
			return;
		}
		case ND_COMMA:
		case ND_STMT:
		case ND_DECLARATION:
			gen_node(FIRST(nd));
			gen_node(SECOND(nd));
			return;
		case ND_ASSIGN:
			gen_addr(FIRST(nd));
			printf("push rax\n");
			gen_node(SECOND(nd));
			printf("pop rcx\n");
			printf("mov [rcx], rax\n");
			return;
		case ND_VAR:
			gen_addr(nd);
			printf("mov rax, [rax]\n");
			return;
		case ND_ADDR:
			gen_addr(FIRST(nd));
			return;
		case ND_DEREF:
			gen_node(FIRST(nd));
			printf("mov rax, [rax]\n");
			return;
		case ND_BLOCK:
		{
			unsigned x = get_stack_size(nd);
			printf("sub rsp, %d\n", 8 * x);
			gen_node(FIRST(nd));
			printf("add rsp, %d\n", 8 * x);
			return;
		}
		case ND_RET:
			gen_node(FIRST(nd));
			printf("mov rsp, rbp\n");
			printf("pop rbp\n");
			printf("ret\n");
			return;
		case ND_FOR:
		case ND_WHILE:
		{
			unsigned x = loop_label = count++;
			gen_node(FOR_INIT(nd));
			printf(".start%d:\n", x);
			if(FOR_COND(nd)) {
				gen_node(FOR_COND(nd));
				printf("cmp rax, 0\n");
				printf("je .end%d\n", x);
			}
			gen_node(FOR_BODY(nd));
			printf(".cont%d:\n", x);
			gen_node(FOR_INC(nd));
			printf("jmp .start%d\n", x);
			printf(".end%d:\n", x);
			loop_label = x;
			return;
		}
		case ND_DO:
		{
			unsigned x = loop_label = count++;
			printf(".start%d:\n", x);
			gen_node(DO_BODY(nd));
			gen_node(DO_COND(nd));
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
			print_token(nd->tk);
			printf(":\n");
			gen_node(FIRST(nd));
			return;
		case ND_GOTO:
			printf("jmp .");
			print_token(FIRST(nd)->tk);
			printf("\n");
			return;
		case ND_FN:
			print_token(nd->tk);
			printf(":\n");
			printf("push rbp\n");
			printf("mov rbp, rsp\n");
			gen_node(FN_BODY(nd));
			printf("pop rbp\n");
			printf("ret\n");
			return;
		case ND_CALL:
			// TODO alias for FIRST AND SECOND?
			gen_node(SECOND(nd));
			printf("call ");
			print_token(FIRST(nd)->tk);
			printf("\n");
			printf("add rsp, %d\n", 8 * node_depth(SECOND(nd)));
			return;
		case ND_ARG:
			if(SECOND(nd))
				gen_node(SECOND(nd));
			gen_node(FIRST(nd));
			printf("push rax\n");
			return;
		case ND_POST_INC:
			gen_addr(FIRST(nd));
			printf("mov rcx, [rax]\n");
			printf("inc DWORD [rax]\n");
			printf("mov rax, rcx\n");
			return;
		case ND_POST_DEC:
			gen_addr(FIRST(nd));
			printf("mov rcx, [rax]\n");
			printf("dec DWORD [rax]\n");
			printf("mov rax, rcx\n");
			return;
		case ND_PRE_INC:
			gen_addr(FIRST(nd));
			printf("inc DWORD [rax]\n");
			printf("mov rax, [rax]\n");
			return;
		case ND_PRE_DEC:
			gen_addr(FIRST(nd));
			printf("dec DWORD [rax]\n");
			printf("mov rax, [rax]\n");
			return;
		default:
			error_token(nd->tk, "unknown node type\n");
	}
}

static void gen(ParseState *ps) {
	printf("bits 64\n");
	printf("default rel\n");
	printf("global main\n");
	printf("section .data\n");
	LIST_FOR_EACH(it, ps->vars) {
		for(Node *decl = it->data; decl; decl = SECOND(decl)) {
			assert(decl->kind = ND_DECLARATION);
			Node *nd = FIRST(decl);
			switch(nd->kind) {
				case ND_VAR:
					print_token(nd->tk);
					printf(" dw 0\n");
					break;
				case ND_ASSIGN:
					Node *var = FIRST(nd), *val = SECOND(nd);
					print_token(var->tk);
					printf(" dw ");
					assert(val->kind == ND_INT);
					print_token(val->tk);
					printf("\n");
					break;
				default:
					error_token(nd->tk, "unknown node type\n");
			}
		}
	}
	printf("section .text\n");
	LIST_FOR_EACH(it, ps->fns)
		gen_node(list_get(it));
}
