#ifndef LIST_H
#define LIST_H

typedef struct list {
	void *data;
	struct list *next;
} *list_t;



#endif
