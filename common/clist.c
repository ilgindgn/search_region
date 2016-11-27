#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "clist.h"
#include "macros.h"

void clist_add (clist_t ** clist, const void * data) {
	clist_t * clist_p = malloc (sizeof (clist_t));
	assert (clist_p != NULL);
	clist_p->data = data;
	clist_p->next = *clist;
	*clist = clist_p;
}

clist_t * clist_add_ordered (clist_t ** clist, const void * data, int (*compar)(const void *, const void *)) {
	clist_t * e;
	clist_t * e_new = malloc (sizeof (clist_t));
	bool inserted = false;
	e_new->data = data;
	// Should e_new replace *clist ?
	if (*clist == NULL || (*compar) (e_new->data, (*clist)->data) <= 0) {
		e_new->next = *clist;
		*clist = e_new;
	}
	else {
		e = *clist;
		do {
			if (e->next == NULL) {
				e->next = e_new;
				e_new->next = NULL;
				inserted = true;
			}
			else if ((*compar) (e_new->data, e->next->data) <= 0) {
				// Insert betw e and e->next
				e_new->next = e->next;
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

void clist_move_head_ordered (clist_t ** clist, int (*compar)(const void *, const void *)) {
	clist_t * e, *e_move;
	bool inserted = false;
	e = e_move = *clist;
	if (e->next != NULL && (*compar) (e_move->data, e->next->data) > 0) {
		*clist = (*clist)->next;
		do {
			if (e->next == NULL) {
				e->next = e_move;
				e_move->next = NULL;
				inserted = true;
			}
			else if ((*compar) (e_move->data, e->next->data) <= 0) {
				// Insert betw e and e->next
				e_move->next = e->next;
				e->next = e_move;
				inserted = true;
			}
			else
				e = e->next;
		}
		while (!inserted);
	}
}

const void * clist_remove_head (clist_t ** clist) {
	const void * result;
	assert (*clist != NULL);
	result = (*clist)->data;
	clist_t * del = *clist;
	*clist = (*clist)->next;
	_free_and_null (del);
	return (result);
}

void clist_remove (clist_t ** clist, const void * data, bool all) {
	clist_t * current = * clist;
	clist_t * current_prev = NULL, * del;
	while (current != NULL) {
		if (current->data == data) {
			if (current_prev == NULL) {
				*clist = current->next;
				del = current;
				current = current->next;
				free (del);
			}
			else {
				del = current;
				current_prev->next = current->next;
				current = current->next;
				free (del);
			}
			if (!all) {
				break;
			}
		}
		else {
			if (current_prev == NULL)
				current_prev = current;
			current_prev = current;
			current = current->next;
		}
	}
}

/*void clist_free_and_null (clist_t ** clist) {
	clist_t * clist_p = *clist, * del;
	while (clist_p != NULL) {
		del = clist_p;
		clist_p = clist_p->next;
		free (del);
	}
	(*clist) = NULL;
}*/

void clist_fprint (FILE* output, clist_t * clist, void (*data_fprint) (FILE*, const void*)) {
	clist_t * current;
	int i = 0;
	_foreach (current, clist) {
		fprintf (output, "[%d]: ", i);
		if (data_fprint != NULL)
			(*data_fprint) (output, current->data);
		i++;
	}
	fprintf(output, "\n");
}

clist_t * clist_clone (clist_t * clist) {
	if (clist == NULL)
		return NULL;

	clist_t * result, * current, * result_current_prev, * result_current;

	result = malloc (sizeof (clist_t));
	result->data = clist->data;
	result_current_prev = result;

	_foreach (current, clist->next) {
		result_current = malloc (sizeof (clist_t));
		result_current->data = current->data;
		result_current_prev->next = result_current;
		result_current_prev = result_current;
	}
	result_current_prev->next = NULL;

	return result;
}

int clist_length (const clist_t * clist) {
	const clist_t * current; int result = 0;
	_foreach (current, clist)
		result++;
	return result;
}

void clist_remove_elem (clist_t ** clist, clist_t ** prev, clist_t ** to_remove) {
	clist_t * del;
	del = (*to_remove);
	if ((*prev) == NULL)
		(*clist) = (*to_remove) = (*to_remove)->next;
	else
		(*prev)->next = (*to_remove) = (*to_remove)->next;
	free (del);
}
