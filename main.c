#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.c"
#include "vector.c"

#define error(...) (printf(__VA_ARGS__),exit(1))

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
