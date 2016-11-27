/*
 * =====================================================================================
 *
 *       Filename:  search_zone.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/06/2012 12:16:25
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#pragma once
#define COUNT_TOUCHED

#include "dlist.h"
#include "typedefs.h"

typedef struct sz_t sz_t;
typedef struct neighbor_t neighbor_t;
typedef struct sibling_t sibling_t;

/**
 * @brief Data structure to represent a search zone
 */
struct sz_t {
	cost_t * corner; ///< Array representing the local upper bound that defines the search zone
	bool visited;
	bool empty; // KD
	neighbor_t * neighbors;
	int nneighbors;
	dlist_t * e;
	int splitindex; // KD
	sz_t * predecessor; //KD
	sz_t ** successor; //KD
	bool * flagneighbor; ///< Array representing the neighbors already defined
};

struct neighbor_t {
	sz_t * sz;
	int index;
};

struct sibling_t {
	sz_t * sz;
	int index;
};

/**
 * @brief Initialize the management of search zones
 *
 * @param dimension  dimension of the objective space
 * @param ideal_cost array that represents the cost of the ideal point
 */
void sz_init (int dimension);


/**
 * @brief    Instanciate a search region (dlist of search zones)
 *
 * @param ub upper bound for each objective function
 *
 * @return   the new search region (one search sone)
 */
dlist_t * sz_list_new (const cost_t * lb, const cost_t * ub);

/**
 * @brief         Print a search region
 *
 * @param sz_list search region
 */
void sz_list_print (const dlist_t * sz_list);
void sz_list_check (const dlist_t * sz_list);

void sz_print (const sz_t * sz);
void sz_print_edge_count (const dlist_t * sz_list);
sz_t * sz_find (dlist_t * sz_list, const cost_t * v);
bool iselementinlist (dlist_t * list, dlist_t * e_sz);
sz_t * sz_find_nonred (dlist_t * e_sz);
sz_t * sz_find_nonred2 (dlist_t * e_sz, dlist_t ** sz_list, dlist_t ** list_empty);
//sz_t * sz_prefind_econstraint (dlist_t * e_sz, const int index_econstraint);
sz_t * sz_prefind_econstraint_tmp (dlist_t * e_sz, const int index);
//void sz_find_nonred (dlist_t * sz_list, sz_t * sz);

//void sz_find_and_remove_dominatedzones (dlist_t ** sz_list, sz_t * sz);
dlist_t * sz_find_redundant (sz_t * sz);
void sz_remove (dlist_t ** sz_list, dlist_t * listR);
//void remove_pointers (dlist_t * e_sz);

//void sz_update (dlist_t ** sz_list, sz_t * szb, const cost_t * v, const int index_econstraint);
void sz_update (dlist_t ** sz_list, dlist_t ** list_empty, sz_t * szb, const cost_t * v, const int index_econstraint);
dlist_t * sz_econstraint_suppress (sz_t * szb, const cost_t * v, const int index_econstraint, dlist_t ** sz_list);
void sz_neighbors_set(dlist_t * listA, dlist_t * e_sz, const int index_econstraint, dlist_t ** list_red, dlist_t ** sz_list);
void econstraintcheck (dlist_t ** listA, dlist_t * e_sz, dlist_t ** sz_list, const int index_econstraint, dlist_t ** list_red);
void make_neighbors (sz_t * sz, sz_t * sz_cand, int index_i, int index_j);
