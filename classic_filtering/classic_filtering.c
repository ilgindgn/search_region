#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>

#include "dlist.h"
#include "macros.h"
#include "stats.h"
#include "output.h"
#include "input.h"

#include "dominance.h"

#include "search_zone.h"

#define MSG_SIZE 1000

static int npoint, nobj;

// {{{ Functions
void help (char * cmd) {
	fprintf(stderr,
"Usage: %s [options] <instance file>\n\n\
Options:\n\
\t-S <file> output summary to <file> (append if exists)\n\
\t-a append output to existing file\n\
\t-D <file> output detailed summary to <file>\n\
\t-s <n> output detailed summary every n points\n\
\t-n <n> output detailed summary n times (priority over previous option)\n\
\t-m operating mode: o = Original/simple (select points for the update one by one)\n\
\t                   s = Simulate a solution strategy based on scalarization\n\
", cmd);
	return;
}

int point_comp (const void * v1, const void * v2) {
	const cost_t * iv1 = * ((const cost_t **) v1), * iv2 = *((const cost_t **) v2);
	cost_t siv1 = 0, siv2 = 0;
	for (int j = 0; j < nobj; j++)
		siv1 += iv1[j], siv2 += iv2[j];
	if (better (siv1, siv2))
		return -1;
	else if (siv1 == siv2)
		return 0;
	else
		return 1;
}
// }}}

int main (int argc, char ** argv) {
	/*{{{ declarations*/
	int c;
	struct stat     statbuf;

	/* Input */
	cost_t ** points = NULL;
	cost_t * lb, * ub;

	/* Output files */
	char * summary_file            = NULL;
	bool append = false;
	char * detailed_file            = NULL;
	bool summary_file_exists, detailed_file_exists;
	FILE * F_summary_file          = NULL;
	FILE * F_detailed_file          = NULL;

	char mode = 's';

	int output_every = 0, ntimes = 1;

	/* Chronos and counters */
	stats_chrono_t * main_chrono = stats_chrono_new ();
	int nlub = 0;
	int i;
	// }}}


	/*{{{ parse arguments*/
	opterr = 0;
	char * options = "hS:D:s:n:m:a";

	while ((c = getopt (argc, argv, options)) != -1)
		switch (c) {
			case 'h':
				help (argv[0]);
				exit (EXIT_SUCCESS);
			case 'S':
				summary_file = optarg;
				break;
			case 'a':
				append = true;
				break;
			case 'D':
				detailed_file = optarg;
				break;
			case 's':
				output_every = atoi (optarg);
				break;
			case 'n':
				ntimes = atoi (optarg);
				break;
			case 'm':
				mode = optarg[0];
			 	if (mode != 'o' && mode != 's') {	
					fprintf(stderr, "Error: unknown mode '%c'.\n\n", mode);
					help (argv[0]);
					exit (EXIT_FAILURE);
				}
				break;
			case '?':
				if (optopt == 's' || optopt == 'S' || optopt == 'D')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				exit (EXIT_FAILURE);
			default:
				abort ();
		}

	if (optind == argc) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}
	/*}}} parse arguments*/
	
	points = csv2int (argv[argc-1], & npoint, & nobj);
	assert (points != NULL);

	if (ntimes != 1)
		output_every = npoint / ntimes;

	if (detailed_file != NULL) {
		if (append) {
			detailed_file_exists = stat(detailed_file, &statbuf) == 0 ? true : false;
			F_detailed_file = fopen (detailed_file, "a");
		}
		else {
			F_detailed_file = fopen (detailed_file, "w");
		}
		assert (F_detailed_file != NULL);
		if (!append || !detailed_file_exists) {
			fprintf (F_detailed_file, "\"d\" \"npoints\" \"nlub\" \"total_time\"\n");
		}
	}

	sz_init (nobj);

	lb = memcpy_or_null (points[0], nobj * sizeof (cost_t));
	ub = memcpy_or_null (points[0], nobj * sizeof (cost_t));
	for (i = 1; i < npoint; i++) {
		for (int j = 0; j<nobj; j++) {
			if (worse (points[i][j], ub[j]))
				ub[j] = points[i][j];
			if (better (points[i][j], lb[j]))
				lb[j] = points[i][j];
		}
	}
	/* We want strict bounds */
	for (int j = 0; j<nobj; j++) {
		degrade(ub[j]), enhance(lb[j]);
	}

#ifdef MAXIMIZE
	printf("Info: Assuming MAXIMIZATION.\n");
#else
	printf("Info: Assuming MINIMIZATION.\n");
#endif

	printf("Info: Using initial upper bound ");
	println_cost_tab (ub, nobj);
	printf("Info: Using initial lower bound ");
	println_cost_tab (lb, nobj);

	dlist_t * sz_list = sz_list_new (lb, ub);
	

	if (mode == 's') {
		printf("Info: using simulation mode.\n");

		dlist_t * old_list = NULL;

		qsort (points, npoint, sizeof (cost_t*), point_comp);
		dlist_t * points_list = NULL;
		for (i=0; i<npoint; i++) {
			dlist_add (&points_list, points[i]);
		}
		
		stats_chrono_start (main_chrono);
		printf("\n");
		while (sz_list != NULL) {
			sz_t * cur_sz = sz_list->data;
			bool point_found = false;
			dlist_t * e_pt = points_list;
			while (e_pt != NULL && !point_found) {
				point_found = strong_dom (e_pt->data, cur_sz->corner, nobj);
				if (!point_found)
					e_pt = e_pt->next;
			}
			if (point_found) {
				sz_update (&sz_list, old_list, e_pt->data);
				dlist_remove_elem (&points_list, e_pt);
			}
			else {
				dlist_move (&sz_list,sz_list,&old_list);
				/*dlist_remove_head (&sz_list);
				sz_free (cur_sz);*/
				nlub++;
			}
		}
		printf("\n");
		stats_chrono_stop (main_chrono);
	}
  else if (mode == 'o') {
		printf("Info: using simple mode.\n");

		stats_chrono_start (main_chrono);
		printf("\n");
		for (i=0; i<npoint; i++) {
	/*#ifndef NDEBUG
			printf("Processings point %9d\n", i);
			fflush (stdout);
#endif*/
			sz_update (&sz_list, NULL, points[i]);
			/*if (i % 100 == 0)*/
				/*printf("\rProcessed %6d points.", i);*/
			if (output_every > 0 
					&& detailed_file != NULL 
					&& i>0 
					&& i % output_every == 0) {
				stats_chrono_stop (main_chrono);
				printf ("\rProcessed %d points...", i); 
				fprintf (F_detailed_file, "%d %d %d %e\n", 
					nobj,
					i,
					dlist_length (sz_list),
					main_chrono->elapsed
				);
				stats_chrono_start (main_chrono);
			}
		}
		printf("\n");
		stats_chrono_stop (main_chrono);
		nlub = dlist_length (sz_list);
		if (detailed_file != NULL 
				&& (output_every == 0 || i % output_every == 0)) {
			printf ("\rProcessed %d points.\n", i); 
			fflush (stdout);
			fprintf (F_detailed_file, "%d %d %d %e\n", 
				nobj,
				i,
				nlub,
				main_chrono->elapsed
			);
		}
	}

	
	/* {{{ Display results and save ND vectors list */
	// {{{ Summary...
	printf("Number of local upper bounds: %d\n", nlub);
	printf("Elapsed time: %.2f seconds.\n", main_chrono->elapsed);
	struct rusage ru_after;
	getrusage (RUSAGE_SELF, &ru_after);
	double maxmem = ru_after.ru_maxrss/(double) 1000; /* MB */
	printf("Total memory used (getrusage): %.2fMB\n", maxmem);

  /*printf("Hypervolume: %f\n", hypervolume(sz_list));*/

	if (summary_file != NULL) {
		if (append) {
			summary_file_exists = stat(summary_file, &statbuf) == 0 ? true : false;
			F_summary_file = fopen (summary_file, "a");
		}
		else {
			F_summary_file = fopen (summary_file, "w");
		}
		assert (F_summary_file != NULL);
		if (!append || !summary_file_exists) {
			fprintf (F_summary_file, "\"d\" \"npoints\" \"nlub\" \"total_time\" \"maxmem\"\n");
		}
		fprintf (F_summary_file, "%d %d %d %e %e\n", 
			nobj,
			npoint, 
			nlub, 
			main_chrono->elapsed,
			maxmem
		);
		(void) fclose (F_summary_file);
	}

	if (detailed_file != NULL) {
		fclose (F_detailed_file);
	}
	// }}}
	
	exit (EXIT_SUCCESS);
}

// vim:foldmethod=marker:
