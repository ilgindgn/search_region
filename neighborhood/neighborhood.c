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
#define ECONSTRAINT

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
	stats_chrono_t * find_chrono = stats_chrono_new ();
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
			fprintf (F_detailed_file, 
					"\"d\" \"npoints\" \"total_time\" \"find_time\" \"nlub\"\n");
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
	dlist_t * list_empty = NULL; //  list for lubs in simulation mode that turned out to be empty
	
	// KD number of points generated
	int numpoints = 0;
	const int index_econstraint = -1; // any number between -1 and nobj-1:
	if (index_econstraint < -1 || index_econstraint > nobj-1) {
		fprintf(stderr, "Error: Index for e-constraint method is not chosen appropriately.\n\n");
		exit (EXIT_FAILURE);
	}
	// -1 refers to not using e-constraint method, 0 to nobj-1 to the respective component used

	if (mode == 's') {
		printf("Info: using simulation mode.\n");


		
		qsort (points, npoint, sizeof (cost_t*), point_comp);
		dlist_t * points_list = NULL;
		for (i=0; i<npoint; i++) {
			//println_cost_tab (points[i], nobj); // KD
			dlist_add (&points_list, points[i]);
		}
		
		
		stats_chrono_start (main_chrono);
		printf("\n");
		
		sz_t * cur_sz; // KD
		
		while (sz_list != NULL) {
			// Select search zone
			
			// GP case: select any local upper bound, e.g. the first one
			//sz_t * cur_sz = sz_list->data;

			/* general case: select a non-redundant local upper bound starting the search from the first element in sz_list */
			dlist_t * e_sz = sz_list;
			
#ifndef NDEBUG
			printf("\nFirst search zone in list: \n");
			sz_print (e_sz->data);
#endif
			
			// e-constraint variant: start from a particular search zone
#ifdef ECONSTRAINT
			if (index_econstraint > -1) {
				cur_sz = sz_prefind_econstraint_tmp (e_sz, index_econstraint);
			}
			else {
#endif
				// find a non-redundant search zone
				cur_sz = sz_find_nonred2 (e_sz, &sz_list, &list_empty);
				// it might happen that no (non-empty) search zone exists after calling nonred
				if (sz_list == NULL)
					break;
				
				
#ifndef NDEBUG
				printf("Nonred search zone: \n");
				sz_print (cur_sz);
				if (cur_sz->empty) {
					printf("*** Stop: empty sz selected. \n");
					exit (EXIT_FAILURE);
				}
					
#endif

#ifdef ECONSTRAINT
			}
#endif

			
			bool point_found = false;
			dlist_t * e_pt = points_list;
			
#ifdef ECONSTRAINT
			if (index_econstraint < 0) {
#endif
				while (e_pt != NULL && !point_found) {
					point_found = strong_dom (e_pt->data, cur_sz->corner, nobj);
					if (!point_found)
						e_pt = e_pt->next;
				}
#ifdef ECONSTRAINT
			}
			else {
				//  mimic e-constraint scalarization wrt component index_econstraint
				bool point_found_tmp;
				int valcomp = ub[index_econstraint]+1;
				int valcomptmp = 0;
				dlist_t * e_pt_tmp = e_pt;
				
				while (e_pt != NULL) {
					point_found_tmp = strong_dom (e_pt->data, cur_sz->corner, nobj);
					if (point_found_tmp) {
						point_found = true;
						valcomptmp = getcomponent(e_pt->data, index_econstraint);
						
						if (valcomptmp < valcomp) {
							valcomp = valcomptmp;
							e_pt_tmp = e_pt;
						}
					}
					e_pt = e_pt->next;
				}
				e_pt = e_pt_tmp;
			}
#endif

			if (point_found) {

				numpoints++;
				
#ifndef NDEBUG
				printf("Point found: ");
				println_cost_tab (e_pt->data, nobj);
				printf("Number of points generated: %d\n", numpoints); // KD
				printf("\n");
#endif
				
				sz_update (&sz_list, &list_empty, cur_sz, e_pt->data, index_econstraint);
				dlist_remove_elem (&points_list, e_pt);
			}
			else {
				
#ifndef NDEBUG
				printf("No point found \n\n");
#endif

				// GP case
				//dlist_remove_elem (&sz_list, cur_sz->e);
				//cur_sz->e = NULL;

				// General case
				dlist_t * listR = sz_find_redundant (cur_sz);
				
#ifndef NDEBUG
				printf("List of redundant search zones: \n");
				dlist_print (listR, sz_print);
				printf("\n");
#endif
				//sz_remove (&sz_list, listR);
				
				
				//  move empty search zones from listR (sz_list) to list list_empty
				dlist_t * e_tmp, * e_tmp_next;
				e_tmp = listR;
				while (e_tmp != NULL) {
#ifndef NDEBUG
					printf("Sz to be moved from listR to list_empty: \n");
					sz_print (e_tmp->data);
#endif
					e_tmp_next = e_tmp->next;
					dlist_move (&listR, e_tmp, &list_empty); // move e_tmp from listR to list_empty
					sz_t * sz_tmp = e_tmp->data;
					dlist_remove_elem (&sz_list, sz_tmp->e); // removes e_tmp from sz_list
					e_tmp = e_tmp_next;
				}
				
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
			
			
			stats_chrono_start (find_chrono);
			sz_t * cur_sz = sz_find (sz_list, points[i]);
			stats_chrono_stop (find_chrono);
      if (cur_sz == NULL)
        continue; /* Ignore dominated points */
			sz_update (&sz_list, &list_empty, cur_sz, points[i], index_econstraint);
			if (output_every > 0 
					&& detailed_file != NULL 
					&& i>0 
					&& i % output_every == 0) {
				stats_chrono_stop (main_chrono);
				printf ("\rProcessed %d points...", i); 
				fprintf (F_detailed_file, "%d %d %e %e %d\n", 
					nobj,
					i,
					main_chrono->elapsed, 
					find_chrono->elapsed,
					dlist_length (sz_list)
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
			fprintf (F_detailed_file, "%d %d %e %e %d\n", 
				nobj,
				i,
				main_chrono->elapsed, 
				find_chrono->elapsed,
				nlub
			);
		}
	}
	
	
	/* {{{ Display results and save ND vectors list */
	// {{{ Summary...
	printf("Number of local upper bounds: %d\n", nlub);
	printf("Number of points generated: %d\n", numpoints); // KD
	int totalnumprob = nlub + numpoints;
	printf("Number of pseudo-scalarizations solved: %d\n", totalnumprob); // KD
	printf("Elapsed time: %.2f seconds.\n", main_chrono->elapsed);
	printf("Time spent on find_zone(): %.2f seconds.\n", find_chrono->elapsed);

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
			char summary_header[MSG_SIZE] = "\"d\" \"npoints\" \"total_time\" \"find_time\" \"nlub\" \"maxmem\"";
			fprintf (F_summary_file, "%s\n", summary_header);
		}
		fprintf (F_summary_file, "%d %d %e %e %d %e\n", 
			nobj,
			npoint, 
			main_chrono->elapsed, 
			find_chrono->elapsed,
			nlub,
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
