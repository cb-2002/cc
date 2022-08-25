typedef enum TokenKind TokenKind;
enum TokenKind {
	TK_INT=256,

	TK_ADD_A,
	TK_AND_A,
	TK_BREAK,
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

static TokenKind lex_keyword(char *start, unsigned len) {
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
	for(int i = 0; i < sizeof(map)/sizeof(map[0]); ++i)
		if(strlen(map[i].key) == len && 0 == memcmp(map[i].key, start, len))
			return map[i].value;
	return 0;
}

static TokenKind get_kind(TokenState *ts) {
	TokenKind k;
	char *c = ts->loc;
	switch(*c) {
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
			break;
		// double character operators
		case '%':
			switch(*++c) {
				case '=':
					k = TK_MOD_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '/':
			switch(*++c) {
				case '=':
					k = TK_DIV_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '^':
			switch(*++c) {
				case '=':
					k = TK_XOR_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '*':
			switch(*++c) {
				case '=':
					k = TK_MUL_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '+':
			switch(*++c) {
				case '+':
					k = TK_INC;
					break;
				case '=':
					k = TK_ADD_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '-':
			switch(*++c) {
				case '-':
					k = TK_DEC;
					break;
				case '=':
					k = TK_SUB_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '&':
			switch(*++c) {
				case '&':
					k = TK_LAND;
					break;
				case '=':
					k = TK_AND_A;
					break;
				default:
					--c;
					break;
			}
			break;
		case '|':
			switch(*++c) {
				case '=':
					k = TK_OR_A;
					break;
				case '|':
					k = TK_LOR;
					break;
				default:
					--c;
					break;
			}
			break;
		case '<':
			switch(*++c) {
				case '<':
					if(c[1] == '=') {
						++c;
						k = TK_SHL_A;
					} else
					k = TK_SHL;
					break;
				default:
					--c;
					break;
			}
			break;
		case '>':
			switch(*++c) {
				case '>':
					if(c[1] == '=') {
						++c;
						k = TK_SHR_A; 
					}
					else
						k = TK_SHR;
					break;
				default:
					--c;
					break;
			}
			break;
		case '=':
			switch(*++c) {
				case '=':
					k = TK_EQ;
					break;
				default:
					--c;
					break;
			}
			break;
		case '!':
			switch(*++c) {
				case '=':
					k = TK_NE;
					break;
				default:
					--c;
					break;
			}
			break;
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
			while(is_int(*++ts->loc));
			return TK_INT;
		default:
			while(is_alpha(*c))
				++c;
			if(c == ts->loc)
				error_at(ts->src, ts->loc, "unknown token\n");
			if(!(k = lex_keyword(ts->loc, c - ts->loc))) {
				k = TK_ID;
			}
			ts->loc = c;
			return k;
	}
	if(c == ts->loc)
		k = *c;
	ts->loc = ++c;
	return k;
}

// TODO merge this with get_kind, as there's shared code
static Token *get_token(TokenState *ts) {
	while(*ts->loc == ' ')
		++ts->loc;
	char *start = ts->loc;
	TokenKind kind = get_kind(ts);
	return token_new(ts->src, (String){
		.src = start,
		.size = ts->loc - start,
	}, kind);
}

static Token *tokenize(char *src) {
	TokenState *ts = &(TokenState){
		.src = src,
		.loc = src,
	};
	Token head;
	Token *tk = &head;
	while(*ts->loc != '\0')
		tk = tk->next = get_token(ts);
	return head.next;
}
