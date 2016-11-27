#include "dominance.h"
#include "macros.h"
#include "typedefs.h"

#define TH .00001

static double spdom_comp = 0;
static double other_comp = 0;

bool weak_dom (const cost_t * v1, const cost_t * v2, int p) {
	int i;
	for (i=0; i < p; i++) {
#ifdef DOM_COUNT_COMP
		other_comp++;
#endif
		if (worse (v1[i], v2[i]))
			return false;
	}
	return true;
}

bool strong_dom (const cost_t * v1, const cost_t * v2, int p) {
	/*static int count = 0;
	count++;
	if (count % 100000 == 0)
		printf("%d\n", count);*/
	int i;
	for (i=0; i < p; i++) {
#ifdef DOM_COUNT_COMP
		spdom_comp++;
#endif
		if (worse_eq(v1[i], v2[i]))
			return false;
	}
	return true;
}

bool dom (const cost_t * v1, const cost_t * v2, int p) {
	int i;
	bool different = false;
	for (i=0; i < p; i++) {
#ifdef DOM_COUNT_COMP
		other_comp++;
#endif
		if (worse (v1[i], v2[i]))
			return false;
		else if (!different && better (v1[i], v2[i]))
			different = true;
	}
	return different;
}

// KD
cost_t getcomponent (const cost_t * v, const int index_econstraint) {
	return v[index_econstraint];
}


/**
 * @brief    Special dominance
 *
 * @param v1
 * @param v2
 * @param p
 *
 * @return   SPDOM_D if v1 < v2, i if v1[i] == v2[i] and v1[-i] < v2[-i], SPDOM_ND otherwise
 */
int special_dom (const cost_t * v1, const cost_t * v2, int p) {
	int eq_index = SPDOM_D;
	for (int i = 0; i<p; i++) {
#ifdef DOM_COUNT_COMP
		spdom_comp++;
#endif
		if (v1[i] == v2[i]) {
			if (eq_index == SPDOM_D)
				eq_index = i;
			else
				return SPDOM_ND;
		}
		else {
#ifdef DOM_COUNT_COMP
			spdom_comp++;
#endif
			if (worse(v1[i], v2[i]))
				return SPDOM_ND;
		}
	}
	return eq_index;
}

double dominance_get_spdom_comp (void) {
	return spdom_comp;
}

double dominance_get_other_comp (void) {
	return other_comp;
}
