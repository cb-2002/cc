enum TokenKind {
	TK_INT=256,

	TK_ADD_A,
	TK_AND_A,
	TK_BREAK,
	TK_COMMENT,
	TK_CONTINUE,
	TK_DEC,
	TK_DIV_A,
	TK_DO,
	TK_ELSE,
	TK_EQ,
	TK_FOR,
	TK_GE,
	TK_GOTO,
	TK_ID,
	TK_IF,
	TK_INC,
	TK_INCLUDE,
	TK_LAND,
	TK_LE,
	TK_LOR,
	TK_MOD_A,
	TK_MUL_A,
	TK_NE,
	TK_OR_A,
	TK_RET,
	TK_SHL,
	TK_SHL_A,
	TK_SHR,
	TK_SHR_A,
	TK_SUB_A,
	TK_TYPE,
	TK_WHILE,
	TK_WHITESPACE,
	TK_XOR_A,

	TK_COUNT,
};
typedef unsigned short TokenKind;
static_assert(USHRT_MAX > TK_COUNT, "");

typedef struct Token Token;
typedef struct TokenState TokenState;
struct TokenState {
	char *file, *start, *pos;
	Token *tk;
	TokenState *next;
};

static void token_state_init(TokenState *ts, char *file, Token *tk) {
	ts->file = file;
	ts->pos = read_file(file);
	ts->tk = tk;
	ts->next = NULL;
}

static void token_state_push(TokenState *ts, char *file) {
	TokenState *tmp = malloc(sizeof(TokenState));
	memcpy(tmp, ts, sizeof(TokenState));
	token_state_init(ts, file, ts->tk);
	ts->next = tmp;
}

static void token_state_pop(TokenState *ts) {
	memcpy(ts, ts->next, sizeof(TokenState));
	free(ts->next);
}

struct Token {
	char *file, *pos;
	TokenKind kind;
};

static TokenKind scan(TokenState *);
static bool token_new(TokenState *ts) {
	TokenKind kind = scan(ts);
	Token tk = {
		.kind = kind,
		.file = ts->file,
		.pos = ts->start,
	};
	++ts->pos;
	vec_push_back(ts->tk, tk);
	return kind != '\0';
}

static unsigned token_len(Token *tk) {
	TokenState ts = {
		.file = tk->file,
		.pos = tk->pos,
	};
	scan(&ts);
	return 1 + ts.pos - tk->pos;
}

static void print_token(Token *tk) {
	printf("%.*s", token_len(tk), tk->pos);
}

static bool token_cmp(Token *tk, Token *TK) {
	unsigned len = token_len(tk);
	return len == token_len(TK) && 0 == memcmp(tk->pos, TK->pos, len);
}

static char *line_start(char *pos) {
	while (pos[-1] && pos[-1] != '\n')
		--pos;
	return pos;
}

static char *line_end(char *pos) {
	while (*pos && *pos != '\n')
		++pos;
	return pos;
}

static unsigned line_offset(char *start, char *pos) {
	unsigned col = 0;
	for (char *s = start; s < pos; ++s) {
		col += *s == '\t' ? 8 - col % 8
			: 1;
	}
	return col;
}

__declspec(noreturn)
static void error_at(char *file, char *pos, char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	char *start = line_start(pos);
	unsigned len = line_end(pos) - start, offset = line_offset(start, pos);
	error("in %s\n%.*s\n%*s^\n", file, len, start, offset, "");
}

#define error_token(tk, fmt, ...) \
	error_at(tk->file, tk->pos, fmt, ##__VA_ARGS__)

static bool is_int(char c) {
	return '0' <= c && c <= '9';
}

static bool is_alpha(char c) {
	return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

static bool is_whitespace(char c) {
	switch (c) {
		case ' ':
		case '\t':
			return true;
		default:
			return false;
	}
}

typedef struct TokenMap TokenMap;
struct TokenMap {
	char *key;
	TokenKind val;
};

static TokenKind scan_word(TokenState *ts, TokenMap map[], unsigned num) {
	while (is_alpha(ts->pos[1]))
		++ts->pos;
	unsigned len = ts->pos - ts->start + 1;
	for (int i = 0; i < num; ++i) 
		if (strlen(map[i].key) == len && 0 == memcmp(map[i].key, ts->start, len))
			return map[i].val;
	return 0;
}

static TokenKind scan_keyword(TokenState *ts) {
	static TokenMap map[] = {
		{ "break", TK_BREAK, },
		{ "continue", TK_CONTINUE, },
		{ "do", TK_DO, },
		{ "else", TK_ELSE, },
		{ "else", TK_FOR, },
		{ "for", TK_FOR, },
		{ "goto", TK_GOTO, },
		{ "if", TK_IF, },
		{ "int", TK_TYPE, },
		{ "return", TK_RET, },
		{ "while", TK_WHILE, },
	};
	TokenKind k = scan_word(ts, map, sizeof(map) / sizeof(map[0]));
	return k ? k : TK_ID;
}

static char *scan_filename(TokenState *ts, char end) {
	ts->start = ++ts->pos;
	unsigned len = 0;
	while (ts->pos[++len] != end)
		if (ts ->pos[len] == '\n')
			error_at(ts->file, ts->pos, "expected '%c'\n", end);
	ts->pos += len + 1;
	char *src = malloc(sizeof(char) * (len + 1));
	memcpy(src, ts->start, len);
	src[len] = '\0';
	return src;
}

static void scan_whitespace(TokenState *ts) {
	while (is_whitespace((++ts->pos)[0]));
}

static TokenKind scan_include(TokenState *ts) {
	char *file;
	switch (ts->pos[0]) {
		case '"':
			file = scan_filename(ts, '"');
			break;
		case '<':
			file = scan_filename(ts, '>');
			break;
		default:
			error_at(ts->file, ts->pos, "expected '\"' or '<'");
	}
	token_state_push(ts, file);
	return scan(ts);
}

static TokenKind scan_directive(TokenState *ts) {
	ts->start = ++ts->pos;
	static TokenMap map[] = {
		{ "include", TK_INCLUDE, },
	};
	TokenKind k = scan_word(ts, map, sizeof(map) / sizeof(map[0]));
	scan_whitespace(ts);
	switch (k) {
		case TK_INCLUDE:
			return scan_include(ts);
		default:
			error_at(ts->file, ts->pos, "unknown directive\n");
	}
}

static void scan_comment(TokenState *ts) {
	ts->pos += 2;
	while(!(ts->pos[0] == '*' && ts->pos[1] == '/')) {
		if (!*ts->pos)
			error_at(ts->file, ts->pos, "expected '*/'\n");
		if (ts->pos[0] == '/' && ts->pos[1] == '*')
			scan_comment(ts);
		else
			++ts->pos;
	}
	ts->pos += 2;
}

static TokenKind scan(TokenState *ts) {
	ts->start = ts->pos;
	switch (ts->pos[0]) {
		case '\0':
			if (!ts->next)
				return '\0';
			token_state_pop(ts);
			return scan(ts);
		// whitespace
		case '\n':
			scan_whitespace(ts); 
			if (ts->pos[0] != '#')
				return scan(ts);
			++ts->pos;
			return scan_directive(ts);
		case ' ':
		case '\t':
			scan_whitespace(ts); 
			return scan(ts);
		// single character operators
		case '(':
		case ')':
		case ',':
		case ':':
		case ';':
		case '?':
		case '{':
		case '}':
		case '~':
		case '#':
			return ts->pos[-1] ? ts->pos[0]
				: scan_directive(ts);
		// double character operators
		case '%':
			switch (ts->pos[1]) {
				case '=':
					++ts->pos;
					return TK_MOD_A;
				default:
					return ts->pos[0];
			}
		case '/':
			switch (ts->pos[1]) {
				case '*':
					scan_comment(ts);
					return scan(ts);
				case '/':
					while (ts->pos[0] != '\n')
						++ts->pos;
					return scan(ts);
				case '=':
					++ts->pos;
					return TK_DIV_A;
				default:
					return ts->pos[0];
			}
		case '^':
			switch (ts->pos[1]) {
				case '=':
					++ts->pos;
					return TK_XOR_A;
				default:
					return ts->pos[0];
			}
		case '*':
			switch (ts->pos[1]) {
				case '=':
					++ts->pos;
					return TK_MUL_A;
				default:
					return ts->pos[0];
			}
		case '+':
			switch (ts->pos[1]) {
				case '+':
					++ts->pos;
					return TK_INC;
				case '=':
					++ts->pos;
					return TK_ADD_A;
				default:
					return ts->pos[0];
			}
		case '-':
			switch (ts->pos[1]) {
				case '-':
					++ts->pos;
					return TK_DEC;
				case '=':
					++ts->pos;
					return TK_SUB_A;
				default:
					return ts->pos[0];
			}
		case '&':
			switch (ts->pos[1]) {
				case '&':
					++ts->pos;
					return TK_LAND;
				case '=':
					++ts->pos;
					return TK_AND_A;
				default:
					return ts->pos[0];
			}
		case '|':
			switch (ts->pos[1]) {
				case '=':
					++ts->pos;
					return TK_OR_A;
				case '|':
					++ts->pos;
					return TK_LOR;
				default:
					return ts->pos[0];
			}
		case '<':
			switch (ts->pos[1]) {
				case '<':
					++ts->pos;
					if (ts->pos[1] == '=') {
						++ts->pos;
						return TK_SHL_A;
					}
					return TK_SHL;
				default:
					return ts->pos[0];
			}
		case '>':
			switch (ts->pos[1]) {
				case '>':
					++ts->pos;
					if (ts->pos[1] == '=') {
						++ts->pos;
						return TK_SHR_A; 
					}
					return TK_SHR;
				default:
					return ts->pos[0];
			}
		case '=':
			switch (ts->pos[1]) {
				case '=':
					++ts->pos;
					return TK_EQ;
				default:
					return ts->pos[0];
			}
		case '!':
			switch (ts->pos[1]) {
				case '=':
					++ts->pos;
					return TK_NE;
				default:
					return ts->pos[0];
			}
		// numbres
		case '0' ... '9':
			while (is_int(ts->pos[1]))
				++ts->pos;
			return TK_INT;
		case 'a' ... 'z':
		case 'A' ... 'Z':
			return scan_keyword(ts);
		default:
			error_at(ts->file, ts->pos, "unknown token: '%c'\n", ts->pos[0]);
	}
}

static Token *tokenize(char *file) {
	TokenState *ts = &(TokenState){0};
	token_state_init(ts, file, vec_new(1, sizeof(Token)));
	while (token_new(ts));
	return ts->tk;
}
