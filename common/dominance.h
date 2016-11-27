#pragma once

#include <stdbool.h>
#include "typedefs.h"

bool weak_dom     (const cost_t * v1, const cost_t * v2, int p);
bool strong_dom   (const cost_t * v1, const cost_t * v2, int p);
bool dom          (const cost_t * v1, const cost_t * v2, int p);
cost_t getcomponent (const cost_t * v, const int index_econstraint); // KD
#define SPDOM_ND -1
#define SPDOM_D  -2
int special_dom (const cost_t * v1, const cost_t * v2, int p);
double dominance_get_spdom_comp (void);
double dominance_get_other_comp (void);
