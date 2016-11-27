#pragma once

#include <sys/resource.h>

typedef struct stats_t stats_t;
struct stats_t {
	char * short_name;
	char * long_name;
	int val;
};

typedef struct stats_s_t stats_s_t;
struct stats_s_t {
	stats_t ** stats;
	int n;
};

typedef struct stats_chrono_t stats_chrono_t;
struct stats_chrono_t {
	struct rusage ru_before;
	struct rusage ru_after;
	double elapsed;
};

static inline void stats_chrono_start (stats_chrono_t * chrono) {
	getrusage (RUSAGE_SELF, &(chrono->ru_before));
}

static inline void stats_chrono_stop (stats_chrono_t * chrono) {
	struct timeval tv1, tv2;
	getrusage (RUSAGE_SELF, &(chrono->ru_after));
	tv1 = chrono->ru_before.ru_utime;
	tv2 = chrono->ru_after.ru_utime;
	chrono->elapsed += tv2.tv_sec + tv2.tv_usec * 1e-6 - tv1.tv_sec - tv1.tv_usec * 1e-6;
}

stats_chrono_t * stats_chrono_new ();

void stats_print_long (stats_t ** stats, int n);
void stats_fprinta_short (const char * filename, stats_t ** stats, int n);
stats_t ** stats_new (int nstats);
