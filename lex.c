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
	char *src;
	String str;
	TokenKind kind;
	Token *next;
};

static Token *token_new(char *src, String str, TokenKind kind) {
	Token *tk = malloc(sizeof(Token));
	tk->kind = kind;
	tk->src = src;
	tk->str = str;
	tk->next = NULL;
	return tk;
}

#define error_token(tk, fmt, ...) \
	error_at(tk->src, tk->str.src, fmt, __VA_ARGS__)

typedef struct TokenState TokenState;
struct TokenState {
	char *src;
	char *loc;
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
	while (is_alpha(ts->loc[++len]));
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
		if (strlen(map[i].key) == len && 0 == memcmp(map[i].key, ts->loc, len)) {
			k = map[i].value;
			break;
		}
	ts->loc += len - 1;
	return k;
}

static void scan_comment(TokenState *ts) {
	ts->loc += 2;
	while(!(ts->loc[0] == '*' && ts->loc[1] == '/')) {
		if (!*ts->loc)
			error_at(ts->src, ts->loc, "expected '*/'\n");
		if (ts->loc[0] == '/' && ts->loc[1] == '*')
			scan_comment(ts);
		else
			++ts->loc;
	}
	ts->loc += 2;
}

static TokenKind get_kind(TokenState *ts) {
	switch (ts->loc[0]) {
		// whitespace
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
			return ts->loc[0];
		// double character operators
		case '%':
			switch (ts->loc[1]) {
				case '=':
					++ts->loc;
					return TK_MOD_A;
				default:
					return ts->loc[0];
			}
		case '/':
			switch (ts->loc[1]) {
				case '*':
					scan_comment(ts);
					return TK_COMMENT;
				case '=':
					++ts->loc;
					return TK_DIV_A;
				default:
					return ts->loc[0];
			}
		case '^':
			switch (ts->loc[1]) {
				case '=':
					++ts->loc;
					return TK_XOR_A;
				default:
					return ts->loc[0];
			}
		case '*':
			switch (ts->loc[1]) {
				case '=':
					++ts->loc;
					return TK_MUL_A;
				default:
					return ts->loc[0];
			}
		case '+':
			switch (ts->loc[1]) {
				case '+':
					++ts->loc;
					return TK_INC;
				case '=':
					++ts->loc;
					return TK_ADD_A;
				default:
					return ts->loc[0];
			}
		case '-':
			switch (ts->loc[1]) {
				case '-':
					++ts->loc;
					return TK_DEC;
				case '=':
					++ts->loc;
					return TK_SUB_A;
				default:
					return ts->loc[0];
			}
		case '&':
			switch (ts->loc[1]) {
				case '&':
					++ts->loc;
					return TK_LAND;
				case '=':
					++ts->loc;
					return TK_AND_A;
				default:
					return ts->loc[0];
			}
		case '|':
			switch (ts->loc[1]) {
				case '=':
					++ts->loc;
					return TK_OR_A;
				case '|':
					++ts->loc;
					return TK_LOR;
				default:
					return ts->loc[0];
			}
		case '<':
			switch (ts->loc[1]) {
				case '<':
					++ts->loc;
					if (ts->loc[1] == '=') {
						++ts->loc;
						return TK_SHL_A;
					}
					return TK_SHL;
				default:
					return ts->loc[0];
			}
		case '>':
			switch (ts->loc[1]) {
				case '>':
					++ts->loc;
					if (ts->loc[1] == '=') {
						++ts->loc;
						return TK_SHR_A; 
					}
					return TK_SHR;
				default:
					return ts->loc[0];
			}
		case '=':
			switch (ts->loc[1]) {
				case '=':
					++ts->loc;
					return TK_EQ;
				default:
					return ts->loc[0];
			}
		case '!':
			switch (ts->loc[1]) {
				case '=':
					++ts->loc;
					return TK_NE;
				default:
					return ts->loc[0];
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
			while (is_int(ts->loc[1]))
				++ts->loc;
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
			error_at(ts->src, ts->loc, "unknown token: '%c'\n", ts->loc[0]);
	}
}

// TODO merge this with get_kind, as there's shared code
static Token *get_token(TokenState *ts) {
	char *start = ts->loc;
	TokenKind kind = get_kind(ts);
	++ts->loc;
	switch (kind) {
		case '\0':
			return NULL;
		case TK_WHITESPACE:
		case TK_COMMENT:
			return get_token(ts);
		default:
			return token_new(ts->src, (String){
				.src = start,
				.size = ts->loc - start,
			}, kind);
	}
}

static Token *tokenize(char *src) {
	TokenState *ts = &(TokenState){
		.src = src,
		.loc = src,
	};
	Token head;
	Token *tk = &head;
	while(tk = tk->next = get_token(ts));
	return head.next;
}
