#pragma once

#ifndef LIST_H
#define LIST_H

typedef struct list_t list_t;
struct list_t {
	list_t * next;
	void * data;
};

void list_add (list_t ** list, void * data);
list_t * list_add_ordered (list_t ** list, void * data, int (*compar)(const void *, const void *));
void list_move_head_ordered (list_t ** list, int (*compar)(const void *, const void *));
void * list_remove_head (list_t ** list);
//void list_remove (list_t ** list, void * data, bool all);
//void list_free_and_null (list_t ** list);
static inline void list_free (list_t * list) {
	list_t *del;
	while (list != NULL) {
		del = list;
		list = list->next;
		free (del);
	}
}
#define list_head(list) ((list)->data)
void list_fprint (FILE * output, list_t * list, void (*data_fprint) (FILE*, const void*));
list_t * list_clone (list_t * list);

static inline void list_print (list_t * list, void (*data_fprint) (FILE*, const void*)) {
	list_fprint (stdout, list, data_fprint);
}
int list_length (list_t * list);
#define list_free_and_null(list, data_free, ...) { \
	list_t * list_p = list, * del; \
	while (list_p != NULL) { \
		del = list_p; \
		list_p = list_p->next; \
		data_free (del->data, ##__VA_ARGS__); \
		free (del); \
	} \
	list = NULL; \
}

#endif
