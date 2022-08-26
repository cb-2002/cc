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

#define error(...) (printf(__VA_ARGS__),exit(1))

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

static char *read_file(char *name) {
	FILE *f = fopen(name, "r");
	if(!f)
		error("couldn't find file: %s\n", name);
#if 0
	fseek(f, 0, SEEK_END);
	unsigned len = ftell(f);
	fseek(f, 0, SEEK_SET);
#else
	unsigned len = 0;
	while(fgetc(f) != EOF)
		++len;
	rewind(f);
#endif
	char *buf = malloc(len);
	fread(buf, 1, len, f);
	assert(buf[len - 1] == '\n');
	buf[len - 1] = '\0';
	return buf;
}

int main(int argc, char **argv) {
	assert(argc == 2);
	Token *tk = tokenize(read_file(argv[1]));
	ParseState ps = parse(tk);
	gen(&ps);
	return 0;
}
