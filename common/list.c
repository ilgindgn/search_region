#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "list.h"
#include "macros.h"

void list_add (list_t ** list, void * data) {
	list_t * list_p = malloc (sizeof (list_t));
	assert (list_p != NULL);
	list_p->data = data;
	list_p->next = *list;
	*list = list_p;
}

list_t * list_add_ordered (list_t ** list, void * data, int (*compar)(const void *, const void *)) {
	list_t * e;
	list_t * e_new = malloc (sizeof (list_t));
	bool inserted = false;
	e_new->data = data;
	// Should e_new replace *list ?
	if (*list == NULL || (*compar) (e_new->data, (*list)->data) <= 0) {
		e_new->next = *list;
		*list = e_new;
	}
	else {
		e = *list;
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

void list_move_head_ordered (list_t ** list, int (*compar)(const void *, const void *)) {
	list_t * e, *e_move;
	bool inserted = false;
	e = e_move = *list;
	if (e->next != NULL && (*compar) (e_move->data, e->next->data) > 0) {
		*list = (*list)->next;
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

void * list_remove_head (list_t ** list) {
	void * result;
	assert (*list != NULL);
	result = (*list)->data;
	list_t * del = *list;
	*list = (*list)->next;
	_free_and_null (del);
	return (result);
}

void list_remove (list_t ** list, void * data, bool all) {
	list_t * current = * list;
	list_t * current_prev = NULL, * del;
	while (current != NULL) {
		if (current->data == data) {
			if (current_prev == NULL) {
				*list = current->next;
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

/*void list_free_and_null (list_t ** list) {
	list_t * list_p = *list, * del;
	while (list_p != NULL) {
		del = list_p;
		list_p = list_p->next;
		free (del);
	}
	(*list) = NULL;
}*/

void list_fprint (FILE* output, list_t * list, void (*data_fprint) (FILE*, const void*)) {
	list_t * current;
	int i = 0;
	_foreach (current, list) {
		fprintf (output, "[%d]: ", i);
		if (data_fprint != NULL)
			(*data_fprint) (output, current->data);
		i++;
	}
	fprintf(output, "\n");
}

list_t * list_clone (list_t * list) {
	if (list == NULL)
		return NULL;

	list_t * result, * current, * result_current_prev, * result_current;

	result = malloc (sizeof (list_t));
	result->data = list->data;
	result_current_prev = result;

	_foreach (current, list->next) {
		result_current = malloc (sizeof (list_t));
		result_current->data = current->data;
		result_current_prev->next = result_current;
		result_current_prev = result_current;
	}
	result_current_prev->next = NULL;

	return result;
}

int list_length (list_t * list) {
	list_t * current; int result = 0;
	_foreach (current, list)
		result++;
	return result;
}

void list_remove_elem (list_t ** list, list_t ** prev, list_t ** to_remove) {
	list_t * del;
	del = (*to_remove);
	if ((*prev) == NULL)
		(*list) = (*to_remove) = (*to_remove)->next;
	else
		(*prev)->next = (*to_remove) = (*to_remove)->next;
	free (del);
}
