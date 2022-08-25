#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#include "string.c"
#include "list.c"

__declspec(noreturn)
static void error_at(char *src, char *loc, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("%s\n%*s%c\n", src, (int)(loc - src), "", '^');
	exit(1);
}

#include "lex.c"
#include "parse.c"
#include "gen.c"

int main(int argc, char **argv) {
	assert(argc == 2);
	Token *tk = tokenize(argv[1]);
	ParseState ps = parse(tk);
	gen(&ps);
	return 0;
}
