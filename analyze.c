typedef struct Var Var;
struct Var {
	Token *tk;
	int offset;
};

typedef struct Scope Scope;
struct Scope {
	Scope *parent;
	Var *vars;
	int offset;
};

typedef struct GenState GenState;
struct GenState {
	Scope *scope;
	bool decl;
};

static Var *var_find(GenState *gs, Token *tk) {
	for (Scope *s = gs->scope; s; s = s->parent)
		for (unsigned i = 0; i < vec_len(s->vars); ++i)
				if (token_cmp(s->vars[i].tk, tk))
					return s->vars + i;
	return NULL;
}

static Scope *scope_new() {
	Scope *s = malloc(sizeof(Scope));
	s->parent = NULL;
	s->vars = vec_new(1, sizeof(Scope));
	s->offset = 0;
	return s;
}

static void scope_enter(GenState *gs) {
	Scope *s = malloc(sizeof(Scope));
	s->parent = gs->scope;
	s->vars = vec_new(1, sizeof(Var));
	s->offset = gs->scope->offset;
	gs->scope = s;
};

static void scope_exit(GenState *gs) {
	gs->scope = gs->scope->parent;
}

static Var *define_sym(GenState *gs, Token *tk, int offset) {
	Var v = {
		.tk = tk,
		.offset = offset
	};
	vec_push_back(gs->scope->vars, v);
	return vec_back(gs->scope->vars);
}

static Var *define_var(GenState *gs, Token *tk) {
	return define_sym(gs, tk, ++gs->scope->offset);
}

static void define_params(GenState *gs, Node *nd) {
	for (int offset = -1; nd; nd = PARAM_NEXT(nd))
		define_sym(gs, PARAM_VAR(nd)->tk, --offset);
}

