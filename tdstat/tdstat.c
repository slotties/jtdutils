/******************************************************************************
 * tdstat - a tool for printing statistics of a tdstrip-stream
 *
 * Copyright (C) 2009 Stefan Lotties <slotties@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the 
 * Free Software Foundation; either version 3 of the License, or (at your 
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 * ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <regex.h>
#include <errno.h>

#include "td.h"
#include "common.h"
#include "td_format.h"

enum stat_type { DUMP, FILTER };

typedef struct {
	char* name;
	enum stat_type type;
	
	int running;
	int sleeping;
	int waiting_on_condition;
	int waiting_on_monitor;
	int waiting;
	int unknown;
} stats;

struct thread_filter_stats {
	td_filter filter;
	stats total;
	struct thread_filter_stats* next;
};
typedef struct thread_filter_stats t_fstats;

// states
stats CURR_DUMP_STATS;
t_fstats* FILTERS = NULL;
	
static void print_stats(FILE* in);
static void reset_stats(stats* stat);
static void print_stat(stats* curr);
static void increase_stats(stats* s, td_state state);
static void print_help();

int main(int argc, char** argv) {
	int idx;
	
	t_fstats* fstats_curr = NULL;
	
	char* tmp_name;
	char* tmp_method;
	char* filtering_token = ",";
	
	int comp_r;
	
	struct option options[] = {
		{ "filter", required_argument, 0, 'f' },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};
	
	for (int c = 0; c >= 0; ) {
		c = getopt_long(argc, argv, ":f:vh", options, &idx);
		switch (c) {
			case 'f':
				if (!FILTERS) {
					FILTERS = (t_fstats*) malloc(sizeof(t_fstats));
					fstats_curr = FILTERS;
				} else {
					t_fstats* next = (t_fstats*) malloc(sizeof(t_fstats));
					fstats_curr->next = next;
					fstats_curr = next;
					fstats_curr->next = NULL;
				}
				
				tmp_name = strtok(optarg, filtering_token);
				fstats_curr->filter.value.value = tmp_name;
				fstats_curr->total.name = tmp_name;
				fstats_curr->total.type = FILTER;
				
				tmp_method = strtok(NULL, filtering_token);
				switch (tmp_method[0]) {
					case 's':
						fstats_curr->filter.fptr_filter = td_tfstarts;
						break;
					case 'c':
						fstats_curr->filter.fptr_filter = td_tfcont;
						break;
					case 'r':
						/* compile regular expression filter */
						fstats_curr->filter.value.regexp = (regex_t*) malloc(sizeof(regex_t));
						comp_r = regcomp(fstats_curr->filter.value.regexp, fstats_curr->filter.value.value, REG_EXTENDED);
						if (comp_r != 0) {
							/* compilation error */
							char error_msg[512];
							regerror(comp_r, fstats_curr->filter.value.regexp, error_msg, 512);
							fprintf(stderr, "Could not compile regular expression: %s\n", error_msg);
							return EXIT_FAILURE;
						}
						fstats_curr->filter.fptr_filter = td_tfreg;
						break;
					case 'e':
					default:
						fstats_curr->filter.fptr_filter = td_tfcmp;
						break;						
				}
				break;
			case 'v':
				PRINT_VERSION("tdstat");
				return EXIT_SUCCESS;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			case '?':
				print_help();
				return EXIT_FAILURE;
		}
	}
	
	FILE* in;
	if (optind < argc) {
		in = fopen(argv[optind], "r");
		if (!in) {
			fprintf(stderr, "Could not open %s: %s.\n", argv[optind], strerror(errno));
			return EXIT_FAILURE;
		}
	} else {
		in = stdin;
	}
	
	print_stats(in);

	// cleanup
	while (FILTERS) {
		t_fstats* next;
		next = FILTERS->next;

		if (FILTERS->filter.value.regexp) {
			regfree(FILTERS->filter.value.regexp);
			free(FILTERS->filter.value.regexp);
		}
		free(FILTERS);
			
		FILTERS = next;
	}
	
	fclose(in);
	
	return EXIT_SUCCESS;
}

static void print_help() {
	printf("\
Usage: tdstat [-f filter_value,filtering_method] [FILE]\n\
FILE may either be a file containing the output of tdstrip or standard input.\n\
\n\
General options:\n\
  -h, --help                   Prints this help.\n\
  -v, --version                Prints the version information.\n\
  -f, --filter                 Additionally prints statistics for the\n\
                               selected filter. The argument has to be\n\
                               in format 'filtering_value,filtering_method'.\n\
                               The filtering_value is the value to filter for.\n\
                               The filtering_method is one of s, e, c and r\n\
                               standing for starts-with, exact, contains and\n\
                               regular expression.\n\
                               The -f parameter can be specified multiple times.\n\
\n");
}

static v_result reset_all_stats(void* userdata, const char* id) {
	reset_stats(&CURR_DUMP_STATS);
	
	for (t_fstats* f = FILTERS; f; f = f->next) {
		reset_stats(&(f->total));
	}
	
	CURR_DUMP_STATS.name = strdup(id);
	
	return V_BASE;
}

static v_result update_stats(void* userdata, const char* name, const char* native_id, const char* java_id, td_state state) {
	increase_stats(&CURR_DUMP_STATS, state);
	for (t_fstats* f = FILTERS; f; f = f->next) {
		if (f->filter.fptr_filter(name, &(f->filter.value))) {
		increase_stats(&(f->total), state);
		}
	}
	
	// no need for the thread anymore
	return (V_BASE | V_SKIP);
}

static v_result td_print_dump_stats(void* userdata, td_dump* dump, v_result inResult) {
	print_stat(&CURR_DUMP_STATS);
	free(CURR_DUMP_STATS.name);
	
	for (t_fstats* f = FILTERS; f; f = f->next) {
		print_stat(&(f->total));
	}
	
	// no need to keep the dump in memory
	return false;
}

static void print_stats(FILE* in) {
	CURR_DUMP_STATS.type = DUMP;

	printf("\
                                   |  RUN | OBJ_WAI | WAI_MON | WAI_CON | SLEEP\n\
-----------------------------------|------|---------|---------|---------|------\n\
");
	
	dump_parser parser = td_init();

	td_set_dumpv(parser, reset_all_stats, td_print_dump_stats);
	td_set_threadv(parser, update_stats, NULL);
	
	td_parse(in, parser);
	td_free_parser(parser);
}

static void increase_stats(stats* s, td_state state) {
	switch (state) {
		case RUNNING:
			s->running++;
			break;
		case WAITING_FOR_MONITOR_ENTRY:
			s->waiting_on_monitor++;
			break;
		case OBJECT_WAIT:
			s->waiting++;
			break;
		case WAITING_ON_CONDITION:
			s->waiting_on_condition++;
			break;
		case SLEEPING:
			s->sleeping++;
			break;
		case UNKNOWN:
			s->unknown++;
			break;
	}
}

static void print_stat(stats* curr) {
	char* prefix;
	switch (curr->type) {
		case DUMP:
			prefix = "D:";
			break;
		case FILTER:
			prefix = "F:";
			break;
	}
	
	printf("%-10s%24s %6d %9d %9d %9d %6d\n", 
		prefix,
		curr->name,
		curr->running,
		curr->waiting,
		curr->waiting_on_monitor,
		curr->waiting_on_condition,
		curr->sleeping);	
}

static void reset_stats(stats* curr) {
	curr->running = 0;
	curr->waiting = 0;
	curr->waiting_on_condition = 0;
	curr->waiting_on_monitor = 0;
	curr->sleeping = 0;	
	curr->unknown = 0;
}

