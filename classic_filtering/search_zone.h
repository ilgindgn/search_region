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

#include "dlist.h"
#include "typedefs.h"

typedef struct sz_t sz_t;

/**
 * @brief Data structure to represent a search zone
 */
struct sz_t {
	cost_t * corner; ///< Array representing the local upper bound that defines the search zone
};

/**
 * @brief Initialize the management of search zones
 *
 * @param dimension  dimension of tyhe objective space
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

void sz_update (dlist_t ** sz_list, dlist_t * old_list, const cost_t * v);
void sz_free (sz_t * del);
