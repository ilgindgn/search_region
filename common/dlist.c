#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "dlist.h"
#include "macros.h"

dlist_t * dlist_add (dlist_t ** list, void * data) {
	dlist_t * list_p = malloc (sizeof (dlist_t));
	assert (list_p != NULL);
	list_p->data = data;
	list_p->next = *list;
	list_p->prev = NULL;
	if (*list != NULL)
		(*list)->prev = list_p;
	*list = list_p;
	return list_p;
}

dlist_t * dlist_add_sorted (dlist_t ** list, void * data, int (*compar)(const void *, const void *)) {
	dlist_t * e;
	dlist_t * e_new = malloc (sizeof (dlist_t));
	assert (e_new != NULL);
	bool inserted = false;
	e_new->data = data;
	// Should e_new replace *list ?
	if (*list == NULL || (*compar) (e_new->data, (*list)->data) <= 0) {
		e_new->next = *list;
		e_new->prev = NULL;
		if (*list != NULL)
			(*list)->prev = e_new;
		*list = e_new;
	}
	else {
		e = *list;
		do {
			if (e->next == NULL) {
				e->next = e_new;
				e_new->next = NULL;
				e_new->prev = e;
				inserted = true;
			}
			else if ((*compar) (e_new->data, e->next->data) <= 0) {
				// Insert betw e and e->next
				e_new->next = e->next;
				e_new->prev = e;
				e->next->prev = e_new;
				e->next = e_new;
				inserted = true;
			}
			else
				e = e->next;
		}
		while (!inserted);
	}
	return e_new;
}

void dlist_move_sorted (dlist_t ** src, dlist_t * e, dlist_t ** dst, int (*compar)(const void *, const void *)) {
	dlist_t * e_dst;
	bool inserted = false;
	/* Remove 'e' from 'src' */
	if (e->prev != NULL)
		e->prev->next = e->next;
	else
		*src = e->next;
	if (e->next != NULL)
		e->next->prev = e->prev;
	/* Add 'e' to sorted  'dst' */
	if (*dst == NULL) {
		*dst = e;
		e->prev = e->next = NULL;
	}
	else {
		e_dst = *dst;
		do {
			if ((*compar) (e->data, e_dst->data) <= 0) {
				/* Insert e before e_dst */
				e->prev = e_dst->prev;
				e->next = e_dst;
				if (e_dst->prev != NULL)
					e_dst->prev->next = e;
				else
					*dst = e;
				e_dst->prev = e;
				inserted = true;
			}
			else if (e_dst->next == NULL) {
				/* Insert 'e' after 'e_dst' */
				e_dst->next = e;
				e->prev = e_dst;
				e->next = NULL;
				inserted = true;
			}
			else {
				e_dst = e_dst->next;
			}
		}
		while (!inserted);
	}
}

/*void dlist_move_sorted (dlist_t ** list, dlist_t * e_move, int (*compar)(const void *, const void *)) {
	dlist_t * e;
	bool inserted = false;
	e = e_move;
	if (e->next != NULL && (*compar) (e_move->data, e->next->data) > 0) {
		if (e_move == *list) {
			*list = (*list)->next;
			(*list)->prev = NULL;
		}
		else {
			e_move->prev->next = e_move->next;
			e_move->next->prev = e_move->prev;
		}
		do {
			if (e->next == NULL) {
				e->next = e_move;
				e_move->next = NULL;
				e_move->prev = e;
				inserted = true;
			}
			else if ((*compar) (e_move->data, e->next->data) <= 0) {
				// Insert betw e and e->next
				e_move->next = e->next;
				e_move->prev = e;
				e->next->prev = e_move;
				e->next = e_move;
				inserted = true;
			}
			else
				e = e->next;
		}
		while (!inserted);
	}
}*/

dlist_t * dlist_add_after (dlist_t ** list, dlist_t *elem, void * data) {
	dlist_t * list_p = malloc (sizeof (dlist_t));
	list_p->data = data;
	list_p->prev = elem;
	if (*list == NULL && elem == NULL) {
		*list = list_p;
		list_p->next = NULL;
	}
	else {
		assert (elem != NULL);
		list_p->next = elem->next;
		if (elem->next != NULL)
			elem->next->prev = list_p;
		elem->next = list_p;
	}
	return list_p;
}

dlist_t * dlist_add_before (dlist_t ** list, dlist_t *elem, void * data) {
	dlist_t * list_p = malloc (sizeof (dlist_t));
	list_p->data = data;
	list_p->next = elem;
	list_p->prev = elem->prev;
	if (elem->prev != NULL)
		elem->prev->next = list_p;
	else
		(*list) = list_p;
	elem->prev = list_p;
	return list_p;
}

void dlist_move (dlist_t ** src_list, dlist_t * elem, dlist_t ** dst_list) {
	if (elem->prev != NULL)
		elem->prev->next = elem->next;
	else
		*src_list = elem->next;
	if (elem->next != NULL)
		elem->next->prev = elem->prev;
	if (*dst_list != NULL)
		(*dst_list)->prev = elem;
	elem->next = (*dst_list);
	elem->prev = NULL;
	(*dst_list) = elem;
	return;
}

dlist_t * dlist_join (dlist_t * list1, dlist_t * list2) {
	dlist_t * e = list1;
	if (list1 == NULL)
		return list2;
	else if (list2 == NULL)
		return list1;
	else {
		while (e->next != NULL)
			e = e->next;
		e->next = list2;
		list2->prev = e;
		return list1;
	}
}

dlist_t * dlist_join_sorted (dlist_t * list1, dlist_t * list2, int (*compar)(const void *, const void *)) {
	dlist_t * e1 = list1, *e1_next;
	dlist_t * e2 = list2;
	if (list1 == NULL)
		return list2;
	else if (list2 == NULL)
		return list1;
	else {
		do {
			if ((*compar) (e1->data, e2->data) <= 0) {
				/* Insert e1 before e2 */
				e1_next = e1->next;
				e1->prev = e2->prev;
				e1->next = e2;
				if (e2->prev != NULL)
					e2->prev->next = e1;
				else
					list2 = e1;
				e2->prev = e1;
				e1 = e1_next;
			}
			else if (e2->next == NULL) {
				/* Insert e1 and successors after e2 */
				e2->next = e1;
				e1->prev = e2;
				break;
			}
			else {
				e2 = e2->next;
			}
		}
		while (e1 != NULL);
	}
	return list2;
}

void * dlist_remove_head (dlist_t ** list) {
	void * result;
	assert (*list != NULL);
	result = (*list)->data;
	dlist_t * del = *list;
	*list = (*list)->next;
	if (*list != NULL)
		(*list)->prev = NULL;
	_free_and_null (del);
	return (result);
}

void * dlist_remove_elem (dlist_t ** list, dlist_t * elem) {
	void * result;
	assert (elem != NULL);
	result = elem->data;
	dlist_t *del = elem;
	if (elem->prev != NULL)
		elem->prev->next = elem->next;
	else
		*list = elem->next;
	if (elem->next != NULL)
		elem->next->prev = elem->prev;
	_free_and_null (del);
	return (result);
}

void dlist_remove (dlist_t ** list, void * data, bool all) {
	dlist_t * current = * list;
	dlist_t * del;
	while (current != NULL) {
		if (current->data == data) {
			del = current;
			if (current->prev != NULL)
				current->prev->next = current->next;
			else
				*list = current->next;
			if (current->next != NULL)
				current->next->prev = current->prev;
			current = current->next;
			free (del);
			if (!all) {
				break;
			}
		}
		else
			current = current->next;
	}
}

/*dlist_t * dlist_clone (dlist_t * list) {
	if (list == NULL)
		return NULL;

	dlist_t * result, * current, * result_current_prev, * result_current;

	result = malloc (sizeof (dlist_t));
	result->data = list->data;
	result_current_prev = result;

	_foreach (current, list->next) {
		result_current = malloc (sizeof (dlist_t));
		result_current->data = current->data;
		result_current_prev->next = result_current;
		result_current_prev = result_current;
	}
	result_current_prev->next = NULL;

	return result;
}*/

int dlist_length (dlist_t * list) {
	dlist_t * current; int result = 0;
	_foreach (current, list)
		result++;
	return result;
}


/* KD: Check whether a given element is contained in list */
//bool dlist_contains (const dlist_t * list, const dlist_t * e_sz) {
bool dlist_contains (const dlist_t * list, const void * data) {

	const dlist_t * e_sz_tmp;
	bool containedinlist = false;
	
	
	if (list != NULL) {
		e_sz_tmp = list;
		
		while (e_sz_tmp != NULL) {
			if (data == e_sz_tmp->data) {
				containedinlist = true;
				break;
			}
			else {
				e_sz_tmp = e_sz_tmp->next;
			}
		}
	}
	return containedinlist;
}
