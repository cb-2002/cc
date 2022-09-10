// types

enum TypeKind {
       TY_UNKNOWN,

       TY_PTR,
       TY_INT,
};
typedef enum TypeKind TypeKind;

typedef struct Type Type;
struct Type {
       TypeKind kind;
       Type *base;
};

static Type *type_new(TypeKind kind, Type *base) {
       Type *ty = malloc(sizeof(Type));
       ty->kind = kind;
       ty->base = base;
       return ty;
}

static TypeKind type_spec(Node *nd) {
       static TypeKind table[] = {
               [ND_INT] = TY_INT,
               [ND_PTR] = TY_PTR,
       };
       return table[nd->kind];
}

static Type *type_node(Node *spec, Node *ptr) {
       Type *ty = type_new(type_spec(spec), NULL);
       for (; ptr; ptr = FIRST(ptr))
               ty = type_new(TY_PTR, ty);
       return ty;
}

// var

typedef struct Var Var;
struct Var {
	Token *tk;
	int offset;
	Type *ty;
};

static Var *var_new(Var **vars, Token *tk, int offset, Type *ty) {
	Var v = {
		.tk = tk,
		.offset = offset,
		.ty = ty,
	};
	vec_push_back(*vars, v);
	return vec_back(*vars);
}

//

typedef struct Scope Scope;
typedef struct GenState GenState;
struct GenState {
	Scope *scope;
};

// scope

struct Scope {
	Scope *parent;
	Var *vars;
	int offset;
};

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

static Var *var_find(GenState *gs, Token *tk) {
	for (Scope *s = gs->scope; s; s = s->parent)
		for (unsigned i = 0; i < vec_len(s->vars); ++i)
				if (token_cmp(s->vars[i].tk, tk))
					return s->vars + i;
	return NULL;
}

// analysis functions

static void define_var(GenState *gs, unsigned offset, Node *spec, Node *ptr, Node *var) {
	Token *tk = var->tk;
	Type *ty = type_node(spec, ptr);
	var_new(&gs->scope->vars, tk, offset, ty);
}

static void define_local(GenState *gs, Node *spec, Node *ptr, Node *var) {
	define_var(gs, ++gs->scope->offset, spec, ptr, var);
}

static void define_global(GenState *gs, Node *spec, Node *ptr, Node *var) {
	define_var(gs, 0, spec, ptr, var);
}

static void define_params(GenState *gs, Node *nd) {
	for (int offset = -1; nd; nd = PARAM_NEXT(nd))
		define_var(gs, --offset, PARAM_SPEC(nd), PARAM_PTR(nd), PARAM_VAR(nd));
}
