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

#include "search_zone.h"

/* #####   MACROS  -  LOCAL TO THIS SOURCE FILE   ################################### */

/* #####   TYPE DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ######################### */

/* #####   DATA TYPES  -  LOCAL TO THIS SOURCE FILE   ############################### */
/* #####   VARIABLES  -  LOCAL TO THIS SOURCE FILE   ################################ */
static cost_t * new_corner = NULL;
static int dim = 0;
static cost_t * ref;

/* #####   PROTOTYPES  -  LOCAL TO THIS SOURCE FILE   ############################### */
static inline sz_t * sz_new (cost_t * corner);
#if !(defined SZ_GP)
static dlist_t ** wdominated;
#endif

/* #####   FUNCTION DEFINITIONS  -  EXPORTED FUNCTIONS   ############################ */
dlist_t * sz_list_new (const cost_t * lb, const cost_t * ub) {
	dlist_t * result = NULL;
	sz_t * sz_ini = sz_new (memcpy_or_null (ub, dim * sizeof (cost_t)));
	ref = memcpy_or_null (ub, dim * sizeof (cost_t));
	dlist_add (&result,sz_ini);
	return result;
}

void sz_init (int dimension) {
	dim = dimension;
	new_corner = malloc (dim*sizeof (cost_t));
#if !(defined SZ_GP)
	wdominated = calloc (dim, sizeof (dlist_t*));
#endif
}

/* #####   FUNCTION DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ##################### */
sz_t * sz_new (cost_t * corner) {
	sz_t * result = malloc (sizeof (sz_t));
	result->corner = corner;
	return result;
}


void sz_free (sz_t * del) {
	free (del->corner);
	free (del);
}

#ifdef SZ_GP /* Basic/GP */
void sz_update (dlist_t ** sz_list, dlist_t * old_list, const cost_t * v) {
	int j;
	dlist_t * e_sz1, * e_sz2;
	dlist_t * refined = NULL;
	sz_t * sz_p1, *sz_p2;
	bool dominates;

	e_sz1 = *sz_list;
	while (e_sz1 != NULL) {
		sz_p1 = e_sz1->data;
		if (strong_dom (v, sz_p1->corner, dim)) {
			e_sz2 = e_sz1;
			e_sz1 = e_sz1->next;
			dlist_move (sz_list, e_sz2, &refined);
		}
		else
			e_sz1 = e_sz1->next;
	}
	
	/*printf ("%d\n", dlist_length (refined));*/
	_foreach (e_sz1, refined) {
		sz_p1 = e_sz1->data;
		for (j = 0; j< dim; j++) {
			memcpy (new_corner, sz_p1->corner, dim*sizeof (cost_t));
			new_corner[j] = v[j];
			dominates = false;
			// Check for redundant zones
			for (e_sz2 = refined; e_sz2 != NULL && ! dominates; e_sz2 = e_sz2->next) {
				sz_p2 = e_sz2->data;
				if (sz_p2 != sz_p1) {
					dominates = weak_dom (new_corner, sz_p2->corner, dim);
					/*printf("%p\n", sz_p2);*/
				}
			}
			if (! dominates) {
				sz_p2 = sz_new (new_corner);
				dlist_add (sz_list, sz_p2);
				new_corner = malloc (dim*sizeof (cost_t));
			}
		}
	}
	dlist_free_and_null (refined, sz_free);
}

#else /* Basic/NGP */
void sz_update (dlist_t ** sz_list, dlist_t * old_list, const cost_t * v) {
	int j;
	dlist_t * e_sz1, * e_sz2;
	dlist_t * refined = NULL;
	sz_t * sz_p1, *sz_p2;
	bool dominates;

	e_sz1 = *sz_list;
	while (e_sz1 != NULL) {
		sz_p1 = e_sz1->data;
		int spdom_status = special_dom (v, sz_p1->corner, dim);
		if (spdom_status == SPDOM_ND) {
			e_sz1 = e_sz1->next;
		}
		else if (spdom_status == SPDOM_D) {
			e_sz2 = e_sz1;
			e_sz1 = e_sz1->next;
			dlist_move (sz_list, e_sz2, &refined);
		}
		else {
			dlist_add (&wdominated[spdom_status], sz_p1);
			e_sz1 = e_sz1->next;
		}
	}
	e_sz1 = old_list;
	while (e_sz1 != NULL) {
		sz_p1 = e_sz1->data;
		int spdom_status = special_dom (v, sz_p1->corner, dim);
		if (spdom_status == SPDOM_ND) {
			e_sz1 = e_sz1->next;
		}
		else {
			assert (spdom_status != SPDOM_D);
			dlist_add (&wdominated[spdom_status], sz_p1);
			e_sz1 = e_sz1->next;
		}
	}

	_foreach (e_sz1, refined) {
		sz_p1 = e_sz1->data;
		for (j = 0; j< dim; j++) {
			new_corner = memcpy (new_corner, sz_p1->corner, dim*sizeof (cost_t));
			new_corner[j] = v[j];
			dominates = false;
			// Check for redundant zones
			for (e_sz2 = refined; e_sz2 != NULL && ! dominates; e_sz2 = e_sz2->next)
				dominates = ((sz_t*)(e_sz2->data) != sz_p1 && weak_dom (new_corner, ((sz_t*)e_sz2->data)->corner, dim));
			for (e_sz2 = wdominated[j]; e_sz2 != NULL && ! dominates; e_sz2 = e_sz2->next)
				dominates = weak_dom (new_corner, ((sz_t*)e_sz2->data)->corner, dim);
			if (! dominates) {
				sz_p2 = sz_new (new_corner);
				dlist_add (sz_list, sz_p2);
				new_corner = malloc (dim*sizeof (cost_t));
			}
		}
	}
	for (j=0; j<dim; j++)
		dlist_free_and_null (wdominated[j], (void));
	dlist_free_and_null (refined, sz_free);
}
#endif
