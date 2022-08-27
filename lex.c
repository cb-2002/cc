typedef enum TokenKind TokenKind;
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

typedef struct Token Token;
struct Token {
	char *line, *pos;
	unsigned len;
	TokenKind kind;
	Token *next;
};

static void print_token(Token *tk) {
	printf("%.*s", tk->len, tk->pos);
}

static Token *token_new(char *line, char *pos, unsigned len, TokenKind kind) {
	Token *tk = malloc(sizeof(Token));
	tk->kind = kind;
	tk->line = line;
	tk->pos = pos;
	tk->len = len;
	tk->next = NULL;
	return tk;
}

__declspec(noreturn)
static void error_at(char *line, char *pos, char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	unsigned len = 1;
	while (line[len] != '\n' || line[len] != '\0')
		++len;
	error("%.*s\n%*s^\n", len, line, (int)(pos - line), "");
}

#define error_token(tk, fmt, ...) \
	error_at(tk->line, tk->pos, fmt, __VA_ARGS__)

typedef struct TokenState TokenState;
struct TokenState {
	char *line, *pos;
};

static bool is_int(char c) {
	return '0' <= c && c <= '9';
}

static bool is_alpha(char c) {
	return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

static TokenKind lex_keyword(TokenState *ts) {
	unsigned len = 0;
	TokenKind k = TK_ID;
	while (is_alpha(ts->pos[++len]));
	static struct {
		char *key;
		TokenKind value;
	} map[] = {
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
	for (int i = 0; i < sizeof(map)/sizeof(map[0]); ++i)
		if (strlen(map[i].key) == len && 0 == memcmp(map[i].key, ts->pos, len)) {
			k = map[i].value;
			break;
		}
	ts->pos += len - 1;
	return k;
}

static void scan_comment(TokenState *ts) {
	ts->pos += 2;
	while(!(ts->pos[0] == '*' && ts->pos[1] == '/')) {
		if (!*ts->pos)
			error_at(ts->line, ts->pos, "expected '*/'\n");
		if (ts->pos[0] == '/' && ts->pos[1] == '*')
			scan_comment(ts);
		else
			++ts->pos;
	}
	ts->pos += 2;
}

static TokenKind get_kind(TokenState *ts) {
	switch (ts->pos[0]) {
		// whitespace
		case '\n':
			ts->line = ts->pos + 1;
		case ' ':
			return TK_WHITESPACE;
		// single character operators
		case '(':
		case ')':
		case ',':
		case ':':
		case ';':
		case '?':
		case '\0':
		case '{':
		case '}':
		case '~':
			return ts->pos[0];
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
					return TK_COMMENT;
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
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			while (is_int(ts->pos[1]))
				++ts->pos;
			return TK_INT;
		case 'a': case 'A':
		case 'b': case 'B':
		case 'c': case 'C':
		case 'd': case 'D':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'i': case 'I':
		case 'j': case 'J':
		case 'k': case 'K':
		case 'l': case 'L':
		case 'm': case 'M':
		case 'n': case 'N':
		case 'o': case 'O':
		case 'p': case 'P':
		case 'q': case 'Q':
		case 'r': case 'R':
		case 's': case 'S':
		case 't': case 'T':
		case 'u': case 'U':
		case 'v': case 'V':
		case 'w': case 'W':
		case 'x': case 'X':
		case 'y': case 'Y':
		case 'z': case 'Z':
			return lex_keyword(ts);
		default:
			error_at(ts->line, ts->pos, "unknown token: '%c'\n", ts->pos[0]);
	}
}

// TODO merge this with get_kind, as there's shared code
static Token *get_token(TokenState *ts) {
	char *start = ts->pos;
	TokenKind kind = get_kind(ts);
	++ts->pos;
	switch (kind) {
		case '\0':
			return NULL;
		case TK_WHITESPACE:
		case TK_COMMENT:
			return get_token(ts);
		default:
			return token_new(ts->line, start, ts->pos - start, kind);
	}
}

static Token *tokenize(char *src) {
	TokenState *ts = &(TokenState){
		.line = src,
		.pos = src,
	};
	Token head;
	Token *tk = &head;
	while(tk = tk->next = get_token(ts));
	return head.next;
}
