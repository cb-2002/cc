typedef enum TokenKind TokenKind;
enum TokenKind {
	TK_INT=256,

	TK_ADD_A,
	TK_AND_A,
	TK_BREAK,
	TK_COMMENT,
	TK_CONTINUE,
	TK_DEFINE,
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

typedef struct TokenState TokenState;
struct TokenState {
	char *line, *start, *pos;
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

static TokenKind scan(TokenState *);
static Token *token_new(TokenState *ts) {
	TokenKind kind = scan(ts);
	if (!kind)
		return NULL;
	Token *tk = malloc(sizeof(Token));
	tk->kind = kind;
	tk->line = ts->line;
	tk->pos = ts->start;
	tk->len = ++ts->pos - ts->start;
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
	while (line[len] != '\n' && line[len] != '\0')
		++len;
	error("%.*s\n%*s^\n", len, line, (int)(pos - line), "");
}

#define error_token(tk, fmt, ...) \
	error_at(tk->line, tk->pos, fmt, __VA_ARGS__)

static bool is_int(char c) {
	return '0' <= c && c <= '9';
}

static bool is_alpha(char c) {
	return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
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

static TokenKind scan_directive(TokenState *ts) {
	error_at(ts->line, ts->pos, "nye");
}

static void scan_whitespace(TokenState *ts) {
	++ts->pos;
	switch (ts->pos[0]) {
		case ' ':
		case '\t':
			scan_whitespace(ts);
	}
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

static TokenKind scan_newline(TokenState *ts) {
	ts->line = ts->pos + 1;
	scan_whitespace(ts);
	return (ts->pos[0] == '#' ? scan_directive : scan)(ts);
}

static TokenKind scan(TokenState *ts) {
	TokenKind k;
	ts->start = ts->pos;
	switch (ts->pos[0]) {
		// whitespace
		case '\n':
			return scan_newline(ts);
		case ' ':
			scan_whitespace(ts); 
			return scan(ts);
		// single character operators
		case '(':
		case ')':
		case '\0':
		case ',':
		case ':':
		case ';':
		case '?':
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
			return scan_keyword(ts);
		default:
			error_at(ts->line, ts->pos, "unknown token: '%c'\n", ts->pos[0]);
	}
}

static Token *tokenize(char *src) {
	TokenState *ts = &(TokenState){
		.line = src,
		.start =src,
		.pos = src,
	};
	Token head, *tk = &head;
	while (tk = tk->next = token_new(ts));
	return head.next;
}
