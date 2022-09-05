// v[-1] for length, v[-2] for capacity, v[-3] for stride

#define vec_len(v) \
	((unsigned *)(v))[-1]

#define vec_cap(v) \
	((unsigned *)(v))[-2]

#define vec_stride(v) \
	((unsigned *)(v))[-3]

unsigned vec_size(void *v) {
	return sizeof(unsigned) * 3 + vec_cap(v) * vec_stride(v);
}

static void *vec_new(unsigned cap, unsigned stride) {
	unsigned *start = malloc(sizeof(unsigned) * 3 + cap * stride);
	void *v = start + 3;
	vec_len(v) = 0;
	vec_cap(v) = cap;
	vec_stride(v) = stride;
	return v;
}

static void *vec_grow(void *v) {
	vec_cap(v) *= 2;
	unsigned *start = (unsigned *)v -3;
	start = realloc(start, vec_size(v));
	return start + 3;
}

static void *vec_maybe_grow(void *v) {
	return vec_len(v) == vec_cap(v) ? vec_grow(v) : v;
}

#define vec_push_back(v, x) \
	((v) = vec_maybe_grow(v), (v)[vec_len(v)] = (x), (v) + vec_len(v)++)

#define vec_back(v) \
	((v)[vec_len(v) - 1])
