/*
 * =====================================================================================
 *
 *       Filename:  search_zone.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  12/06/2012 12:16:09
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:
 *
 * =====================================================================================
 */

/* #####   HEADER FILE INCLUDES   ################################################### */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "macros.h"
#include "dlist.h"
#include "dominance.h"
#include "output.h"
#include "stats.h"

#include "search_zone.h"

/*#define SZ_CHRONO*/

/* #####   MACROS  -  LOCAL TO THIS SOURCE FILE   ################################### */

/* #####   TYPE DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ######################### */

/* #####   DATA TYPES  -  LOCAL TO THIS SOURCE FILE   ############################### */
/* #####   VARIABLES  -  LOCAL TO THIS SOURCE FILE   ################################ */

/* Cache */
static sibling_t * siblings = NULL;

/* Global data */
static int dim = 0;
static const cost_t * lb, *ub;
static cost_t ** dummies;

/* Counters */
#ifdef SZ_CHRONO
static stats_chrono_t * chrono = NULL;
#endif

/* #####   PROTOTYPES  -  LOCAL TO THIS SOURCE FILE   ############################### */
static inline sz_t * sz_new (const cost_t * corner);
static inline void sz_free (sz_t * del, dlist_t ** sz_list);
/*static void sz_print (const sz_t * sz);*/

/* #####   FUNCTION DEFINITIONS  -  EXPORTED FUNCTIONS   ############################ */
dlist_t * sz_list_new (const cost_t * lb0, const cost_t * ub0) {
	dlist_t * result = NULL;
	lb = lb0, ub = ub0;
	sz_t * sz_ini = sz_new (ub);
	dummies = malloc (dim * sizeof (cost_t*));
	for (int j=0; j<dim; j++) {
		dummies[j] = malloc (dim * sizeof (cost_t));
		for (int k = 0; k < dim; k++)
			dummies[j][k] = (k == j) ? ub[k] : lb[k];
	}
	sz_ini->e = dlist_add (&result,sz_ini);
	sz_ini->nneighbors = dim;
	return result;
}

void sz_init (int dimension) {
	dim = dimension;
#ifdef SZ_CHRONO
	chrono = stats_chrono_new ();
#endif
	siblings = malloc (dim * sizeof (sibling_t));
}

void sz_list_print (const dlist_t * sz_list) {
	dlist_print (sz_list, sz_print);
}

void sz_print_edge_count (const dlist_t * sz_list) {
	int ** tab = calloc (dim, sizeof (int*));
	for (int i = 0; i < dim; i++)
		tab[i] = calloc (dim, sizeof (int));
	const dlist_t *e = sz_list;
	while (e != NULL) {
		const sz_t * sz = e->data;
		for (int i = 0; i < dim; i++) {
			if (sz->neighbors[i].sz != NULL) {
				int j = sz->neighbors[i].index;
				if (i < j)
					tab[i][j]++;
				else
					tab[j][i]++;
			}
		}
		e = e->next;
	}
	for (int i = 0; i < dim; i++)
		for (int j = i+1; j < dim; j++)
			printf("(%d, %d): %6d\n", i, j, tab[i][j]);
}

/* #####   FUNCTION DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ##################### */
sz_t * sz_new (const cost_t * corner) {
	sz_t * result = malloc (sizeof (sz_t));
	result->corner = malloc (dim * sizeof (cost_t));
	memcpy (result->corner, corner, dim * sizeof (cost_t));
	result->neighbors = calloc (dim, sizeof (neighbor_t));
	result->visited = false;
	result->empty = false;
	result->splitindex = 0;
	result->flagneighbor = calloc (dim, sizeof (bool));
	result->predecessor = NULL;
	result->successor = calloc (dim, sizeof (sz_t*));
	return result;
}


void sz_free (sz_t * del, dlist_t ** sz_list) {
	free (del->corner);
	free (del->neighbors);
	free (del->flagneighbor);
	free (del->successor);
	dlist_remove_elem (sz_list,del->e);
	free (del);
}

void sz_print (const sz_t * sz) {
	printf("Search zone %p:\n\t", sz);
	print_cost_tab (sz->corner,dim);
	printf(", empty: %2d\n", sz->empty);
	printf("\tNeighbors:\n");
	for (int j = 0; j < dim; j++) {
		if (sz->neighbors[j].sz != NULL) {
			printf("\t\tat %2d: %p, ", j, sz->neighbors[j].sz);
			//			printf("\t\tat %2d: %p, associated index: %2d\n", j, sz->neighbors[j].sz, sz->neighbors[j].index);
			//			printf("\t\tat %2d: ", j);
			print_cost_tab (sz->neighbors[j].sz->corner,dim);
			printf(", neighbor empty: %2d\n", sz->neighbors[j].sz->empty);
			//printf("\n");
		}
		else
			printf("\t\tat %2d: no neighbor\n", j);
	}
}

void sz_list_check (const dlist_t * sz_list) {
	const dlist_t * e_sz;
	const sz_t * sz;
	int j;
	_foreach (e_sz, sz_list) {
		sz = e_sz->data;
		for (j = 0; j < dim; j++) {
			if (sz->neighbors[j].sz != NULL)
				if (sz->neighbors[j].sz->neighbors[sz->neighbors[j].index].index != j) {
					printf("*** Inconsistency in neighborhood relation.\n");
					exit (EXIT_FAILURE);
				}
		}
	}
}

/* Find a zone containing point v */
sz_t * sz_find (dlist_t * sz_list, const cost_t * v) {
	dlist_t * e_sz;
	sz_t * sz, *result = NULL;
	_foreach (e_sz, sz_list) {
		sz = e_sz->data;
		if (strong_dom (v, sz->corner, dim)) {
			result = sz;
			break;
		}
	}
	return result;
}



/*  Find a zone that is non-redundant or is given by two identical upper bound vectors (possible in NGP case) */
// TODO: Input e_sz no longer needed when also sz_list is input!
sz_t * sz_find_nonred2 (dlist_t * e_sz, dlist_t ** sz_list, dlist_t ** list_empty) {

	sz_t * sz = NULL;
	dlist_t * e_tmp; //, * e_tmp_next;
	int j, reverseindex;
	bool containedinlist = false; // nonredfound = false;
	dlist_t * list1 = NULL, * list2 = NULL, * tabulist = NULL;

#ifndef NDEBUG
	printf("Initial search zone: \n");
	sz_print (e_sz->data);
#endif

	sz = e_sz->data;
	while (sz->e != NULL) {

#ifndef NDEBUG
		printf("Current search zone: \n");
		sz_print (sz);
#endif


		// check whether zone is redundant or not
		// therefore, all j-neighbors have to be scanned, it is sufficient to compare the lubs in component j only
		for (j = 0; j< dim; j++) {
			if (sz->neighbors[j].sz == NULL || (sz->corner[j] > sz->neighbors[j].sz->corner[j])) {
				// nothing to do
#ifndef NDEBUG
				printf("ok in component %2d \n",j);
#endif
			}
			else {
				// e_sz is redundant (neighbor exists and has equal value in comp j)

#ifndef NDEBUG
				printf("Current sz: \n");
				sz_print (sz);
				printf("Neighbor: \n");
				sz_print (sz->neighbors[j].sz);
#endif

				// if neighbor is known to be empty, we can remove the current sz and check the next element in sz_list
				if (sz->neighbors[j].sz->empty) // TODO: can we prevent this case?
				{
					sz->empty = true;
					e_tmp = sz->e;
					//e_tmp_next = e_tmp->next;
					dlist_add (list_empty, sz);
					dlist_remove_elem (sz_list, sz->e); // removes e_tmp from sz_list

#ifndef NDEBUG
					//sz_print(sz);
					printf("List1: \n");
					sz_list_print (list1);
					printf("List2: \n");
					sz_list_print (list2);
					printf("Tabulist: \n");
					sz_list_print (tabulist);
					printf("Search zone with empty %2d neighbor detected \n", j);
					//exit (EXIT_FAILURE);
#endif

					// we can remove all elements of list2 and tabulist since they are empty
					if (list2 != NULL) { // i.e. sz with identical ub exists -> remove as well

						//list2 = dlist_join(list2, tabulist);
#ifndef NDEBUG
						printf("List2: \n");
						sz_list_print (list2);
#endif

						e_tmp = list2;
						while (e_tmp != NULL) {
							sz = e_tmp->data;
							sz->empty = true;
							dlist_add (list_empty, sz);
							e_tmp = e_tmp->next;
							dlist_remove_elem (sz_list, sz->e); // removes e_tmp from sz_list
						}

						// Clear list2 TODO: to be improved (include into while-loop above!)
						while (list2 != NULL) {
							dlist_t * del = list2;
							list2 = list2->next;
							free (del);
						}
					}


					// the following if-statement should be redundant, however its removal causes segmentation faults!?
					if (tabulist != NULL) { // i.e. sz with identical ub exists -> remove as well
						e_tmp = tabulist;
						while (e_tmp != NULL) {
							sz = e_tmp->data;
							sz->empty = true;
							dlist_add (list_empty, sz);
							e_tmp = e_tmp->next;
							dlist_remove_elem (sz_list, sz->e); // removes e_tmp from sz_list
						}

						// Clear tabulist
						while (tabulist != NULL) {
							dlist_t * del = tabulist;
							tabulist = tabulist->next;
							free (del);
						}
					}

					// it might happen that the (only) element of list1 has now become empty
					if (list1 == NULL) {
						// set element to be investigated next
						e_tmp = *sz_list;
						if (e_tmp != NULL) {
							dlist_add (&list1, e_tmp->data);
						}
						else
							sz = NULL; // Algorithm terminates, sz_list empty!
					}

#ifndef NDEBUG
					printf("List1: \n");
					sz_list_print (list1);
					printf("List2: \n");
					sz_list_print (list2);
					printf("Tabulist: \n");
					sz_list_print (tabulist);
					//printf("*** STOP Search zone with empty %2d neighbor: \n", j);
					//exit (EXIT_FAILURE);
#endif

					break;
				}
				else {

					// check whether lubs are equal (note that both differ at most in components j and "reverseindex")
					reverseindex = sz->neighbors[j].index;

					if (sz->corner[reverseindex] < sz->neighbors[j].sz->corner[reverseindex]) {
						dlist_add (&list1, sz->neighbors[j].sz);
						// comment: the test != instead of < produced mistakes, i.e. redundancies !?
						break; // stop for-loop as soon as dominating box found
					}
					else {
						// check whether element is already contained in list to avoid cycles
						containedinlist = dlist_contains (tabulist, sz->neighbors[j].sz);

						// Only insert if not in tabulist
						if (!containedinlist) {
							// check whether element is already contained in list2
							containedinlist = dlist_contains (list2, sz->neighbors[j].sz);
							if (!containedinlist) {
								dlist_add (&list2, sz->neighbors[j].sz);
							}
						}
					}
				}
			}
#ifndef NDEBUG
			printf("List1: \n");
			sz_list_print (list1);
			printf("List2: \n");
			sz_list_print (list2);
			printf("Tabulist: \n");
			sz_list_print (tabulist);
#endif
		} // end of loop over all components j

#ifndef NDEBUG
		printf("All lists after for-loop: List1: \n");
		sz_list_print (list1);
		printf("List2: \n");
		sz_list_print (list2);
		printf("Tabulist: \n");
		sz_list_print (tabulist);
#endif


		if (list1 == NULL && list2 == NULL) {
			// in this case the current sz is non-redundant
			break;
		}
		else if (list1 != NULL) { // i.e. a sz dominating the current one has been found
			e_sz = list1;
			sz = e_sz->data;

			// Clear list1
			while (list1 != NULL) {
				dlist_t * del = list1;
				list1 = list1->next;
				free (del);
			}

			// Clear list2
			while (list2 != NULL) {
				dlist_t * del = list2;
				list2 = list2->next;
				free (del);
			}
		}
		else { // case list1 = NULL, list2 != NULL
			if (sz->empty) {
#ifndef NDEBUG
				printf("Test: List1: \n");
				sz_list_print (list1);
				printf("List2: \n");
				sz_list_print (list2);
				printf("Tabulist: \n");
				sz_list_print (tabulist);
#endif
				// list2 (and possibly tabulist) contains search zones that have identical ub-vectors and can be removed
				list2 = dlist_join(list2, tabulist);
				e_tmp = list2;
				while (e_tmp != NULL) {
					sz = e_tmp->data;
					sz->empty = true;
					dlist_add (list_empty, sz);
					e_tmp = e_tmp->next;
					dlist_remove_elem (sz_list, sz->e); // removes e_tmp from sz_list
				}

				// Clear list2
				while (list2 != NULL) {
					dlist_t * del = list2;
					list2 = list2->next;
					free (del);
				}

				// Clear tabulist (should be redundant)
				while (tabulist != NULL) {
					dlist_t * del = tabulist;
					tabulist = tabulist->next;
					free (del);
				}

#ifndef NDEBUG
				printf("Test: Lists after Clearing: List1: \n");
				sz_list_print (list1);
				printf("List2: \n");
				sz_list_print (list2);
				printf("Tabulist: \n");
				sz_list_print (tabulist);
#endif

				// set element to be investigated next
				e_sz = *sz_list;
				if (e_sz != NULL) {
					sz = e_sz->data;
				}
				else
					sz = NULL; // Algorithm terminates, sz_list empty!

			}
			else {
				dlist_add (&tabulist, sz);

#ifndef NDEBUG
				printf("Tabulist: \n");
				sz_list_print (tabulist);
#endif

				e_sz = list2;
				sz = e_sz->data;

				// remove first entry of list2
				dlist_t * del = list2;
				list2 = list2->next;
				free (del);
			}
		}
	}

#ifndef NDEBUG
	printf("Tabulist vor Clearing: \n");
	sz_list_print (tabulist);
	printf("Search zone to be returned: \n");
	sz_print (sz);
#endif

	// Clear tabulist
	while (tabulist != NULL) {
		dlist_t * del = tabulist;
		tabulist = tabulist->next; //  hier gibts ein Problem!?
		free (del);
#ifndef NDEBUG
		printf("Tabulist while Clearing: \n");
		sz_list_print (tabulist);
#endif

	}

#ifndef NDEBUG
	printf("Tabulist after clearing: \n");
	sz_list_print (tabulist);
#endif

	return sz;
}



/*  Find zones that are contained in current zone, returned list contains at least sz */
// TODO: no return needed anymore in new version with ->empty!
dlist_t * sz_find_redundant (sz_t * sz) {

	dlist_t * e_sz;
	sz_t * sz_nb, * sz_tmp;
	dlist_t * listR = NULL;
	dlist_t * list_tmp = NULL;
	bool containedinlist = false;
	int j;

	sz->empty = true;
	dlist_add (&list_tmp, sz); // note: routine adds at the beginning, not at the end

	// generate listR: contains sz and possibly further zones contained in sz
	while (list_tmp != NULL) {

		e_sz = list_tmp; // take first element from list
		sz_tmp = e_sz->data;

		for (j = 0; j< dim; j++) {
			if (sz_tmp->neighbors[j].sz != NULL && !(sz_tmp->neighbors[j].sz->empty)) {

				sz_nb = sz_tmp->neighbors[j].sz;

				// check whether sz_nb is contained in sz (= sz dominates sz_nb)
				if (weak_dom (sz_nb->corner, sz->corner, dim)) {
					// check whether already contained in listR, otherwise append
					containedinlist = dlist_contains (listR, sz_nb);

					// if zone has not been investigated so far
					if (!containedinlist) {
						// check whether element is already on the stack of list_tmp
						containedinlist = dlist_contains (list_tmp, sz_nb);
						if (!containedinlist) {
							sz_nb->empty = true;
							dlist_add (&list_tmp, sz_nb);
						}
					}
				}
			}
		}
		dlist_move (&list_tmp, e_sz, &listR); // move sz_tmp from list_tmp to listR
	}
	return listR;
}






void sz_update (dlist_t ** sz_list, dlist_t ** list_empty, sz_t * szb, const cost_t * v, const int index_econstraint) {
	//void sz_update (dlist_t ** sz_list, sz_t * szb, const cost_t * v, const int index_econstraint) {
	int i, j, k, neighbor_index;
	dlist_t * e_sz, * e_sz_next;
	sz_t * sz, * new_sz, * neighbor_sz, * sz_pred, * sz_nb, * sz_cand;
	dlist_t * listO = NULL, *listA = NULL, *listR = NULL, *listN = NULL; // listR: old LUBs to be _R_emoved at the end
	int last_sibling = -1;
	bool nbfound;
	int reverseindex;


	// for e-constraint variant: find lubs for which descendant wrt selected component can be suppressed (will be removed afterwards)
	dlist_t * list_red;
	if (index_econstraint > -1) {
		list_red = sz_econstraint_suppress (szb, v, index_econstraint, sz_list);
	}

	dlist_add (&listO, szb);

	while (listO != NULL) {
		e_sz = listO;
		sz = e_sz->data;
		if (sz->visited) {
			dlist_remove_elem (&listO,e_sz);
			continue;
		}
		else {
			dlist_move (&listO, e_sz, &listR);
			sz->visited = true;
		}

		for (j = 0; j< dim; j++) {

			if (sz->neighbors[j].sz == NULL || worse_eq (v[j], sz->neighbors[j].sz->corner[j])) {

				/* Create a new valid local upper bound */
				new_sz = sz_new (sz->corner);
				new_sz->corner[j] = v[j];
				new_sz->splitindex = j;
				if (sz->neighbors[j].sz != NULL && sz->neighbors[j].sz->empty && (v[j] == sz->neighbors[j].sz->corner[j])) {
					new_sz->empty = true;
#ifndef NDEBUG
					printf("Empty search zone: \n");
					sz_print (new_sz);
#endif
				}
				new_sz->predecessor = sz;
				sz->successor[j] = new_sz;
				last_sibling++;
				siblings[last_sibling].sz = new_sz;
				siblings[last_sibling].index = j;


				/* Link with easy neighbor */
				new_sz->neighbors[j] = sz->neighbors[j];
				neighbor_sz = sz->neighbors[j].sz;
				// if (old) neighbor exists, relink backwards
				if (neighbor_sz != NULL) {
					neighbor_index = sz->neighbors[j].index;
					neighbor_sz->neighbors[neighbor_index].sz = new_sz;
				}
				new_sz->flagneighbor[j] = true;

				/* Link with siblings that are neighbors */
				for (k = 0; k < last_sibling; k++) {
					neighbor_sz = siblings[k].sz;
					neighbor_index = siblings[k].index;
					new_sz->neighbors[neighbor_index].sz = neighbor_sz;
					new_sz->neighbors[neighbor_index].index = j;
					neighbor_sz->neighbors[j].sz = new_sz;
					neighbor_sz->neighbors[j].index = neighbor_index;

					new_sz->flagneighbor[neighbor_index] = true;
				}
				new_sz->nneighbors = last_sibling+1;


				/*  components of new_sz for which neighbor can be determined in an easy way */
				for (k = j+1; k < dim; k++) {
					if (sz->neighbors[k].sz == NULL || worse_eq (v[k], sz->neighbors[k].sz->corner[k])) {
						new_sz->flagneighbor[k] = true;
						(new_sz->nneighbors)++;
					}
				}
			}
			else if (sz->neighbors[j].sz != NULL && !(sz->neighbors[j].sz->visited)) {
				dlist_add (&listO, sz->neighbors[j].sz);
			}
			else {
				//sz->neighbors[j].sz = NULL; // TODO: ask Renaud for which this serves?
			}
		}


		for (j = 0; j <= last_sibling; j++) {
			/* Add to list of newly created local upper bounds */
			siblings[j].sz->e = dlist_add (&listA, siblings[j].sz);
		}
		last_sibling = -1;
	}

	e_sz = listA;
	while (e_sz != NULL) {
		sz = e_sz->data;

		for (j = 0; j < dim; j++) {

			if (sz->nneighbors == dim) {
				e_sz = e_sz->next;
				break;
			}

			// if component j of sz still misses its neighbor
			if (!sz->flagneighbor[j]) {
				nbfound = false;
				i = sz->splitindex;
				sz_pred = sz->predecessor;
				//sz_print (sz_pred);
				sz_nb = sz_pred->neighbors[j].sz; // go to j-neighbor
				//sz_print (sz_nb);
				reverseindex = sz_pred->neighbors[j].index;
#ifndef NDEBUG
				printf("\n");
				sz_print (sz);
				printf("\nNeighbor j: %d, Split i: %d, ", j, i);
				printf("Reverseindex: %d ", reverseindex);
#endif


				while (!nbfound) {
					if (reverseindex == i) {
						// check whether descendant exists, if yes, neighbor found
						if (sz_nb->successor[j] != NULL) {
							sz_cand = sz_nb->successor[j];
							make_neighbors (sz, sz_cand, reverseindex, j);
							nbfound = true;
							//printf("Done:\n");
							//sz_print (sz);
							//sz_print (sz_cand);

						}
						else {
							// go to j-neighbor
							reverseindex = sz_nb->neighbors[j].index;


#ifndef NDEBUG
							printf("%d ", reverseindex);
#endif
							//printf("Reverseindex: %d \n", reverseindex);

							sz_nb = sz_nb->neighbors[j].sz;
							//sz_print (sz_nb);
						}
					}
					else {
						// check whether descendant exists, if yes, neighbor found
						if (sz_nb->successor[i] != NULL) {
							sz_cand = sz_nb->successor[i];
							make_neighbors (sz, sz_cand, reverseindex, j);
							nbfound = true;
							//printf("Done:\n");
							//sz_print (sz);
							//sz_print (sz_cand);

						}
						else {
							// go to i-neighbor
							reverseindex = sz_nb->neighbors[i].index;

#ifndef NDEBUG
							printf("%d ", reverseindex);
#endif
							//printf("Reverseindex: %d \n", reverseindex);

							sz_nb = sz_nb->neighbors[i].sz;
							//sz_print (sz_nb);

						}
					}
				}
			}

		}
		// check whether all neighbors have been set
		assert (sz->nneighbors >= dim);
	}

	// move all elements of listA to sz_list (alternative implementation)
	e_sz = listA;
	while (e_sz != NULL) {
		e_sz_next = e_sz->next;
		sz = e_sz->data;
		if (!sz->empty) {
			dlist_move (&listA, e_sz, sz_list);
		}
		else
			dlist_move (&listA, e_sz, list_empty);
		e_sz = e_sz_next;
	}
	// end alternative implementation

	/* Clear listR */
	dlist_free_and_null (listR, sz_free, sz_list);
}

void sz_neighbors_set(dlist_t * listA, dlist_t * e_sz, const int index_econstraint, dlist_t ** list_red, dlist_t ** sz_list) {

	// normal version without using properties of e-constraint method
	if (index_econstraint < 0 || list_red == NULL) {
		dlist_move (&listA, e_sz, sz_list);
	}
	else {
		//  e-constraint-check
		econstraintcheck (&listA, e_sz, sz_list, index_econstraint, list_red);
	}
}


/* Stuff not needed for e-constraint version
	 /
	 /
	 /
	 -----------------------------------------------*/

/*  Remove all elements of listR from sz_list and remove pointers to these elements */
void sz_remove (dlist_t ** sz_list, dlist_t * listR) {

	dlist_t * e_sz, * e_sz_tmp, * e_sz_nb;
	sz_t * sz, * sz_nb;
	bool containedinlist = false;
	int j;

	if (listR != NULL) {
		e_sz = listR;

		// (if-else distinction not needed necessarily, but runs faster since case dlist_length (listR) == 1 occurs mostly (only in GP!))
		if (dlist_length (listR) == 1) {
			sz = e_sz->data;

			// set reverse neighbor pointer to NULL
			for (j = 0; j< dim; j++) {
				if (sz->neighbors[j].sz != NULL) {
					sz_nb = sz->neighbors[j].sz;
					sz_nb->neighbors[sz->neighbors[j].index].sz = NULL;
				}
			}
			dlist_remove_elem (sz_list, sz->e);
			sz->e = NULL;
		}
		else {
			while (e_sz != NULL) {
				sz = e_sz->data;

				// set reverse neighbor pointer to NULL
				for (j = 0; j< dim; j++) {
					if (sz->neighbors[j].sz != NULL) {
						sz_nb = sz->neighbors[j].sz;
						e_sz_nb = sz_nb->e;

						// check whether zone already contained listR
						containedinlist = dlist_contains (listR, e_sz_nb->data);

						// if zone not contained in listR, remove reverse neighbor relation
						if (!containedinlist) {
							sz_nb->neighbors[sz->neighbors[j].index].sz = NULL;
						}
					}
				}
				e_sz_tmp = e_sz->next;
				e_sz = e_sz_tmp;
			}

			// Clear listR and all associated elements in sz_list
			dlist_free_and_null (listR, sz_free, sz_list);
		}
	}
}


dlist_t * sz_econstraint_suppress (sz_t * szb, const cost_t * v, const int index_econstraint, dlist_t ** sz_list) {

	dlist_t * e_sz_tmp, * e_sz_next_tmp;
	sz_t * sz_tmp;
	dlist_t * list_red;

	// find redundant zones of szb
	list_red = sz_find_redundant (szb);

	// reduce this list:
	// (a) search zones that can be removed completely (and which wouldn't have been decomposed in the following) and
	// (b) search zones for which no split wrt component index_econstraint has to be made
	if (dlist_length (list_red) > 1) {
		dlist_t * list_red_remove = NULL;

		// move an element of list_red to list_red_remove if the current point is larger than the corner in component index_econstraint
		e_sz_tmp = list_red;
		while (e_sz_tmp != NULL) {
			sz_tmp = e_sz_tmp->data;
			e_sz_next_tmp = e_sz_tmp->next;
			if (sz_tmp->corner[index_econstraint] <= v[index_econstraint]) {
				// these elements are not contained in listR
				dlist_move (&list_red, e_sz_tmp, &list_red_remove);
			}
			else if (sz_tmp->neighbors[index_econstraint].sz == NULL || worse_eq (v[index_econstraint], sz_tmp->neighbors[index_econstraint].sz->corner[index_econstraint])) {
				// sz remains in list_red, contained in listR
			}
			else {
				// remove element from list_red (contained in listA and decomposed as usual)
				dlist_remove_elem (&list_red, e_sz_tmp);
			}
			e_sz_tmp = e_sz_next_tmp;
		}
		if (list_red_remove != NULL) {
			// remove all elements from list_red_remove and pointers to them
			sz_remove (sz_list, list_red_remove);
		}
	}
	// list_red only contains zones for which no split wrt component index_econstraint is required
	return list_red;
}




void econstraintcheck (dlist_t ** listA, dlist_t * e_sz, dlist_t ** sz_list, const int index_econstraint, dlist_t ** list_red) {

	bool is_szb = false;
	int j;
	sz_t * sz, * neighbor_sz, * sz_tmp;
	dlist_t * e_sz_tmp;

	sz = e_sz->data;

	// check whether sz to be removed has been derived from one of the search zones of list_red
	if (sz->splitindex == index_econstraint) {
		e_sz_tmp = *list_red;
		while (e_sz_tmp != NULL) {
			sz_tmp = e_sz_tmp->data;
			is_szb = true;
			for (j = 0; j < dim; j++) {
				if (j != index_econstraint && sz->corner[j] != sz_tmp->corner[j]) {
					is_szb = false;
					e_sz_tmp = e_sz_tmp->next;
					break;
				}
			}
			if (is_szb)
				break;
		}

		// if sz_tmp is the "origin" of sz
		if (is_szb) {
			// remove all pointers to sz
			// set reverse neighbor pointer to NULL
			for (j = 0; j< dim; j++) {
				if (sz->neighbors[j].sz != NULL) {
					neighbor_sz = sz->neighbors[j].sz;
					neighbor_sz->neighbors[sz->neighbors[j].index].sz = NULL;
				}
			}
			// remove sz from listA (hence do not insert into sz_list)
			dlist_remove_elem (listA, e_sz);

			dlist_remove_elem (list_red, e_sz_tmp); // TODO: elements are contained in listR
		}
		else
			dlist_move (listA, e_sz, sz_list);
	}
	else {
		dlist_move (listA, e_sz, sz_list);
	}
}

// make two search zones neighbors: sz_cand is j-neighbor of sz, sz is i-nb of sz_cand
void make_neighbors (sz_t * sz, sz_t * sz_cand, int i, int j) {

	sz->neighbors[j].sz = sz_cand;
	sz->neighbors[j].index = i;
	sz->nneighbors++;
	sz->flagneighbor[j] = true;

	sz_cand->neighbors[i].sz = sz;
	sz_cand->neighbors[i].index = j;
	sz_cand->nneighbors++;
	sz_cand->flagneighbor[i] = true;
}

// e-constraint: Select a zone which has no neighbor wrt component index_econstraint
sz_t * sz_prefind_econstraint_tmp (dlist_t * e_sz, const int index) {
	sz_t * sz, * sz_nb;
	dlist_t * candlist = NULL;
	int i,j;
	bool nonredundant;


	while (e_sz != NULL) {
		sz = e_sz->data;

		if (sz->neighbors[index].sz == NULL ) {

			// check whether sz is non-redundant
			nonredundant = true;
			for (i = 0; i< dim; i++) {
				if (sz->neighbors[i].sz != NULL) {
					sz_nb = sz->neighbors[i].sz;
					if (sz->corner[i] <= sz_nb->corner[i]) {
						nonredundant = false;
						break;
					}
				}
			}
			if (nonredundant) {
				return sz;
			}
			else {
				// save element
				dlist_add (&candlist, sz);
				e_sz = e_sz->next;
			}
		}
		else {
			e_sz = e_sz->next;
		}
	}

#ifndef NDEBUG
	printf("Processing Candlist \n");
	/*if (candlist != NULL) {
		printf("Candlist: \n");
		dlist_print (candlist, sz_print);
		printf("\n");
		}*/
#endif

	// handle case that sz has a neighbor with identical corner
	e_sz = candlist;
	while (e_sz != NULL) {
		sz = e_sz->data;

		// check whether sz equals one of its neighbors
		for (i = 0; i< dim; i++) {
			if (sz->neighbors[i].sz != NULL) {
				nonredundant = true;
				sz_nb = sz->neighbors[i].sz;
				for (j = 0; j< dim; j++) {
					if (sz->corner[j] != sz_nb->corner[j]) {
						nonredundant = false;
						break;
					}
				}
				if (nonredundant) {
					sz_print (sz);
					sz_print (sz_nb);

					return sz;
				}
			}
		}
		e_sz = e_sz->next;
	}

	// Frage: Ist es sonst egal, welche sz wir benutzen???

#ifndef NDEBUG
	printf("Still no sz found \n");
#endif

	// get first element of candlist and find a non-redundant sz
	e_sz = candlist;
	sz = sz_find_nonred (e_sz);

	// now we might still have the situation that the non-red sz has an index-neighbor not being contained in sz!!

	bool rightboxfound = false;

	if (sz->neighbors[index].sz == NULL) {
		rightboxfound = true; // this case is excluded before
	}
	else {
		sz_nb = sz->neighbors[index].sz;
		dlist_t * chain = NULL;
		dlist_add (&chain, sz);

		// construct sequence of (index_econstraint)-neighbors until no one exists, then select correct non-redundant box

		while (sz_nb != NULL) {
			dlist_add (&chain, sz_nb);
			sz_nb = sz_nb->neighbors[index].sz;
		}

		// find first non-redundant sz in list chain
		e_sz = chain;

		while (e_sz != NULL) {
			sz = e_sz->data;
			rightboxfound = true;
			// check whether zone is redundant or not
			for (j = 0; j< dim; j++) {
				if (sz->neighbors[j].sz != NULL && sz->corner[j] <= sz->neighbors[j].sz->corner[j]){
					rightboxfound = false;
				}
			}
			if (rightboxfound || sz == e_sz->next->data)
				break;
			else
				e_sz = e_sz->next;
		}
	}

	return sz;
}

/*  Find a zone that is not dominated by another zone (it is non-redundant or it is given by two identical upper bound vectors (possible in NGP case))
	 compares all components of neighbors */
sz_t * sz_find_nonred (dlist_t * e_sz) {

	sz_t * sz = NULL;
	bool containedinlist = false, nonredfound = false, szcontained = false, szequal = false;
	dlist_t * list1 = NULL, * list2 = NULL, * tabulist = NULL;
	int i,j;

#ifndef NDEBUG
	printf("Initial search zone: \n");
	sz_print (e_sz->data);
#endif

	sz = e_sz->data;
	while (!nonredfound && sz->e != NULL) {
		//sz = e_sz->data;

		// check whether zone is redundant
		for (j = 0; j< dim; j++) {

			if (sz->neighbors[j].sz != NULL) {

				// test whether sz is contained in its j-neighbor
				for (i =0; i< dim; i++) {
					if (sz->corner[i] > sz->neighbors[j].sz->corner[i]) {
						szcontained = false;
						break;
					}
					else
						szcontained = true;
				}

				// test whether sz equals sz_tmp
				if (szcontained) {
					for (i =0; i< dim; i++) {
						if (sz->corner[i] != sz->neighbors[j].sz->corner[i]) {
							szequal = false;
							break;
						}
						else
							szequal = true;
					}
				}
				else
					szequal = false;

				// check for non-redundance
				if (szcontained) {
					if (!szequal)
						dlist_add (&list1, sz->neighbors[j].sz);
					else {
						// check whether element is already contained in tabulist
						containedinlist = dlist_contains (tabulist, sz->neighbors[j].sz);

						// Only insert if not in tabulist
						if (!containedinlist) {
							dlist_add (&list2, sz->neighbors[j].sz);
						}
					}
				}
			}
		}

		//
		if (list1 == NULL && list2 == NULL) {
			nonredfound = true;
		}
		else if (list1 != NULL) {
			e_sz = list1;
			sz = e_sz->data;
#ifndef NDEBUG
			printf("Next search zone: \n");
			sz_print (e_sz->data);
#endif
			//list1 = NULL; //@ Renaud: we have to free these lists without removing the elements stored (which are still valid elements of sz_list!)
			//list2 = NULL;
			// Clear list1
			while (list1 != NULL) {
				dlist_t * del = list1;
				list1 = list1->next;
				free (del);
			}

			// Clear list2
			while (list2 != NULL) {
				dlist_t * del = list2;
				list2 = list2->next;
				free (del);
			}

		}
		else { // case list1 = NULL, list2 != NULL
			dlist_add (&tabulist, sz);
			e_sz = list2;
			sz = e_sz->data;

#ifndef NDEBUG
			printf("Next search zone (same!): \n");
			sz_print (e_sz->data);
			printf("Tabulist: \n");
			sz_list_print (tabulist);
#endif
			//dlist_remove_head (&list2);
			dlist_t * del = list2;
			list2 = list2->next;
			free (del);

		}
	}

	// Clear tabulist
	while (tabulist != NULL) {
		dlist_t * del = tabulist;
		tabulist = tabulist->next;
		free (del);
	}

	return sz;
}


