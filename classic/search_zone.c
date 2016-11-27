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
#if defined SZ_ENH_UPD_NO_MALLOC || defined SZ_GP
static cost_t ** dummies;
#endif

/* #####   FUNCTION DEFINITIONS  -  EXPORTED FUNCTIONS   ############################ */
dlist_t * sz_list_new (const cost_t * lb, const cost_t * ub) {
	dlist_t * result = NULL;
	sz_t * sz_ini = sz_new (memcpy_or_null (ub, dim * sizeof (cost_t)));
	ref = memcpy_or_null (ub, dim * sizeof (cost_t));
	#if defined SZ_ENH_UPD_NO_MALLOC || defined SZ_GP
	dummies = malloc (dim * sizeof (cost_t*));
	for (int j=0; j<dim; j++) {
		dummies[j] = malloc (dim * sizeof (cost_t));
		for (int k = 0; k < dim; k++)
			dummies[j][k] = (k == j) ? ub[k] : lb[k];
		#ifdef SZ_GP
		sz_ini->points[j] = dummies[j];
		#else
		clist_add (&(sz_ini->points[j]), dummies[j]);
		#endif
	}
	#else
	cost_t * tab;
	for (int j=0; j<dim; j++) {
		tab = malloc (dim * sizeof (cost_t));
		for (int k = 0; k < dim; k++)
			tab[k] = (k == j) ? ub[k] : lb[k];
		list_add (&(sz_ini->points[j]), tab);
	}
	#endif
	dlist_add (&result,sz_ini);
	return result;
}

void sz_init (int dimension) {
	dim = dimension;
	new_corner = malloc (dim*sizeof (cost_t));
}

/* #####   FUNCTION DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ##################### */
sz_t * sz_new (cost_t * corner) {
	sz_t * result = malloc (sizeof (sz_t));
	result->corner = corner;
	#ifdef SZ_GP
	result->points = malloc (dim * sizeof (ccost_p_t));
	#else
		#ifdef SZ_ENH_UPD_NO_MALLOC
	result->points = calloc (dim, sizeof (clist_t*));
		#else
	result->points = calloc (dim, sizeof (list_t*));
		#endif
	#endif
	return result;
}


void sz_free (sz_t * del) {
	free (del->corner);
	#ifndef SZ_GP
	for (int j = 0; j<dim; j++)
		#ifdef SZ_ENH_UPD_NO_MALLOC
		clist_free (del->points[j]);
		#else
		list_free (del->points[j], free);
		#endif
	#endif
	free (del->points);
	free (del);
}

#ifdef SZ_GP
void sz_update (dlist_t ** sz_list, const cost_t * v) {
	int j, k;
	dlist_t * e_sz1, * e_sz2;
	sz_t * sz_p1, *sz_p2;
	bool compute_child_sz;
	ccost_p_t pt, * pts1, *pts2;
	dlist_t * refined = NULL;

	e_sz1 = *sz_list;
	while (e_sz1 != NULL) {
		sz_p1 = e_sz1->data;
		if (strong_dom (v, sz_p1->corner, dim)) 
		{
			e_sz2 = e_sz1;
			e_sz1 = e_sz1->next;
			dlist_move (sz_list, e_sz2, &refined);
		}
		else
			e_sz1 = e_sz1->next;
	}

	_foreach (e_sz1, refined) {
		sz_p1 = e_sz1->data;
		for (j = 0; j< dim; j++) {
			compute_child_sz = true;
			pts1 = sz_p1->points;
			for (k = 0; k < dim && compute_child_sz; k++) {
				if (k == j)
					continue;
				pt = pts1[k];
				if (better_eq (v[j], pt[j]))
					compute_child_sz = false;
			}
			if (compute_child_sz) {
				memcpy (new_corner, sz_p1->corner, dim*sizeof (cost_t));
				new_corner[j] = v[j];
				sz_p2 = sz_new (new_corner);
				pts2 = sz_p2->points;
				new_corner = malloc (dim*sizeof (cost_t));
				memcpy (pts2, pts1, dim*sizeof (ccost_p_t));
				pts2[j] = v;
				dlist_add (sz_list, sz_p2);
			}
		}
	}
	dlist_free_and_null (refined, sz_free);
}

#else /* NGP */
void sz_update (dlist_t ** sz_list, const cost_t * v) {
	int j, k;
	dlist_t * e_sz1, *e_sz2;
#ifdef SZ_ENH_UPD_NO_MALLOC
	clist_t * e_points;
#else
	list_t * e_points;
#endif
	const cost_t * pt;
	dlist_t * refined = NULL;
	sz_t * sz_p1, *sz_p2;
	bool compute_child_sz;

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
#ifdef SZ_ENH_UPD_NO_MALLOC
			clist_add (&(sz_p1->points[spdom_status]), v);
#else
			list_add (&(sz_p1->points[spdom_status]), memcpy_or_null (v, dim*sizeof(cost_t)));
#endif
			e_sz1 = e_sz1->next;
		}
	}

	_foreach (e_sz1, refined) {
		sz_p1 = e_sz1->data;
		for (j = 0; j< dim; j++) {
			compute_child_sz = true;
			for (k = 0; k < dim && compute_child_sz; k++) {
				if (k == j)
					continue;
				_foreach (e_points, sz_p1->points[k]) {
					const cost_t * pt = e_points->data;
					if (worse (v[j], pt[j]))
						break;
				}
				if (e_points == NULL) /* We didn't obtain a "v[j] > pt[j]" */
					compute_child_sz = false;
			}
			if (compute_child_sz) {
				new_corner = memcpy (new_corner, sz_p1->corner, dim*sizeof (cost_t));
				new_corner[j] = v[j];
				sz_p2 = sz_new (new_corner);
				new_corner = malloc (dim*sizeof (cost_t));
				dlist_add (sz_list, sz_p2);
#ifdef SZ_ENH_UPD_NO_MALLOC
				clist_add (&(sz_p2->points[j]), v);
#else
				list_add (&(sz_p2->points[j]), memcpy_or_null (v, dim*sizeof(cost_t)));
#endif
				for (k = 0; k < dim; k++) {
					if (k != j) {
						_foreach (e_points, sz_p1->points[k]) {
							pt = (const cost_t*) e_points->data;
							if (better (pt[j], v[j]))
#ifdef SZ_ENH_UPD_NO_MALLOC
								clist_add (&(sz_p2->points[k]), pt);
#else
								list_add (&(sz_p2->points[k]), memcpy_or_null (pt, dim*sizeof(cost_t)));
#endif
						}
					}
				}
			}
		}
	}
	dlist_free_and_null (refined, sz_free);
}
#endif
