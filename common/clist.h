#pragma once

typedef struct clist_t clist_t;
struct clist_t {
	clist_t * next;
	const void * data;
};

void clist_add (clist_t ** clist, const void * data);
clist_t * clist_add_ordered (clist_t ** clist, const void * data, int (*compar)(const void *, const void *));
void clist_move_head_ordered (clist_t ** clist, int (*compar)(const void *, const void *));
const void * clist_remove_head (clist_t ** clist);
//void clist_remove (clist_t ** clist, const void * data, bool all);
//void clist_free_and_null (clist_t ** clist);
static inline void clist_free (clist_t * clist) {
	clist_t *del;
	while (clist != NULL) {
		del = clist;
		clist = clist->next;
		free (del);
	}
}
#define clist_head(clist) ((clist)->data)
void clist_fprint (FILE * output, clist_t * clist, void (*data_fprint) (FILE*, const void*));
clist_t * clist_clone (clist_t * clist);

static inline void clist_print (clist_t * clist, void (*data_fprint) (FILE*, const void*)) {
	clist_fprint (stdout, clist, data_fprint);
}
int clist_length (const clist_t * clist);
#define clist_free_and_null(clist, data_free, ...) { \
	clist_t * clist_p = clist, * del; \
	while (clist_p != NULL) { \
		del = clist_p; \
		clist_p = clist_p->next; \
		data_free (del->data, ##__VA_ARGS__); \
		free (del); \
	} \
	clist = NULL; \
}
