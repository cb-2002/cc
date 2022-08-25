typedef struct ListNode ListNode;
struct ListNode {
	ListNode *next;
	void *data;
};

static ListNode *list_node_new(void *next, void *data) {
	ListNode *n = malloc(sizeof(ListNode));
	n->next = next;
	n->data = data;
	return n;
}

typedef struct List List;
struct List {
	ListNode *front;
};

List list_new() {
	return (List){0};
}

ListNode *list_begin(List l) {
	return l.front;
}

ListNode *list_end(List l) {
	return NULL;
}

void *list_get(ListNode *n) {
	return n->data;
}

void list_push_back(List *l, void *data) {
	l->front = list_node_new(l->front, data);
}

#define LIST_FOR_EACH(it, l) \
	for(ListNode *(it) = list_begin(l); (it) != list_end(l); (it) = (it)->next)
