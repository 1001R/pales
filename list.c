#include "list.h"
#include <stdlib.h>

list_t list_create() {
	list_t l = (list_t) malloc(sizeof(struct list));
	memset(l, 0, sizeof(*l));
	return l;
}

list_t list_add(list_t l, void *data) {
	l->next = list_create();
	l->next->data = data;
}

