#pragma once

#include <stdbool.h>

typedef struct dlist_t dlist_t;
struct dlist_t {
	dlist_t * next;
	dlist_t * prev;
	void * data;
};

typedef struct dlist_t* dlist_p_t;

static inline dlist_t * dlist_back_to_head (dlist_t * elem) {
	if (elem == NULL)
		return elem;
	while (elem->prev != NULL)
		elem = elem->prev;
	return elem;
}

dlist_t * dlist_add (dlist_t ** list, void * data);
dlist_t * dlist_add_sorted (dlist_t ** list, void * data, int (*compar)(const void *, const void *));
void dlist_move_sorted (dlist_t ** src, dlist_t * e, dlist_t ** dst, int (*compar)(const void *, const void *));
//void dlist_move_sorted (dlist_t ** list, dlist_t * e_move, int (*compar)(const void *, const void *));
dlist_t * dlist_add_after (dlist_t ** list, dlist_t * elem, void * data);
dlist_t * dlist_add_before (dlist_t ** list, dlist_t * elem, void * data);
void dlist_move (dlist_t ** src_list, dlist_t * elem, dlist_t ** dst_list);
dlist_t * dlist_join (dlist_t * list1, dlist_t * list2);

/**
 * @brief        Join two sorted lists (in nondecreasing order wrt 'compar')
 *
 * @param list1  first list
 * @param list2  second list
 * @param compar comparison function
 *
 * @return       joint list ('list2' containing the elements of 'list1')
 */
dlist_t * dlist_join_sorted (dlist_t * list1, dlist_t * list2, int (*compar)(const void *, const void *));
void * dlist_remove_head (dlist_t ** list);
void * dlist_remove_elem (dlist_t ** list, dlist_t * elem);
void dlist_remove (dlist_t ** list, void * data, bool all);

#define dlist_free_and_null(list, data_free, ...) { \
	dlist_t * list_p = (list), * dlist_del; \
	while (list_p != NULL) { \
		dlist_del = list_p; \
		list_p = list_p->next; \
		data_free (dlist_del->data, ##__VA_ARGS__); \
		free (dlist_del); \
	} \
	(list) = NULL; \
}

#define dlist_print(list, data_fprint, ...) { \
	const dlist_t * e; \
	_foreach (e, (list)) \
		data_fprint (e->data, ##__VA_ARGS__); \
}

#define dlist_head(dlist) ((dlist)->data)

dlist_t * dlist_clone (dlist_t * list);

int dlist_length (dlist_t * list);
bool dlist_contains (const dlist_t * list, const void * data);
//bool dlist_contains (const dlist_t * list, const dlist_t * e_sz);
