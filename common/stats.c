#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/resource.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stats.h"

void stats_print_long (stats_t ** stats, int n) {
	int i;
	for (i=0; i<n; i++)
		printf("%s: %d\n", stats[i]->long_name, stats[i]->val);
}

void stats_fprinta_short (const char * filename, stats_t ** stats, int n) {
	bool output_file_exists;
	FILE * output_file = NULL;
	struct stat statbuf;
	int i;

	output_file_exists = stat (filename, &statbuf) == 0 ? true : false;
	output_file = fopen (filename, "a");
	assert (output_file != NULL);
	if (!output_file_exists) {
		for (i=0; i<n; i++)
			fprintf(output_file, "\"%s\" ", stats[i]->short_name);
		fprintf (output_file, "\n");
	}
	for (i=0; i<n; i++)
		fprintf(output_file, "%d ", stats[i]->val);
	fprintf (output_file, "\n");
	fclose (output_file);
}

stats_t ** stats_new (int nstats) {
	stats_t ** stats;
	int j;
	stats = calloc ((size_t) nstats, sizeof (stats_t*));
	for (j=0; j<nstats; j++)
		stats[j] = calloc (1, sizeof (stats_t));
	return (stats);
}

stats_chrono_t * stats_chrono_new () {
	stats_chrono_t * chrono = malloc (sizeof (stats_chrono_t));
	chrono->elapsed = 0;
	return (chrono);
}
