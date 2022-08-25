typedef struct String String;
struct String {
	unsigned size;
	char *src;
};

void print_string(String str) {
	printf("%.*s", str.size, str.src);
}

String c_string(char *s) {
	return (String){
		.size = sizeof(s) / sizeof(char) - 1,
		.src = s,
	};
}

bool string_cmp(String s0, String s1) {
	for(unsigned i = 0; i < s0.size || i < s1.size; ++i)
		if(s0.src[i] != s1.src[i])
			return false;
	return true;
}
