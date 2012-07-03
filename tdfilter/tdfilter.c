/******************************************************************************
 * tdfilter - a tool for filtering specific thread dumps
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
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <errno.h>

#include "td.h"
#include "td_format.h"
#include "common.h"

typedef struct {
	int from_idx;
	int to_idx;
	char* from_id;
	char* to_id;
	bool is_range;
	bool inside_range;
} d_filter;

typedef struct {
	list* filters;
	int curr_idx;
} filter_state;

static void print_filtered(FILE* in, filter_state* state);
static bool update_range_knowledge(filter_state* state, const char* dump_id, d_filter* filter);
static bool add_idx_filter(filter_state* state, char* value);
static bool add_id_filter(filter_state* state, const char* value);
static void free_d_filter(void* filter);
static void print_help();

int main(int argc, char** argv) {
	int c = 1;
	int idx;

	FILE* in;

	struct option options[] = {
		{ "index", required_argument, 0, 'x' },
		{ "id", required_argument, 0, 'i' },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};
	
	filter_state state;
	state.curr_idx = 0;
	state.filters = NULL;

	while (c >= 0) {
		c = getopt_long(argc, argv, ":vhi:x:", options, &idx);
		switch (c) {
			/* general stuff */
			case 'x':
				if (!add_idx_filter(&state, optarg)) {
					// fast fail
					return EXIT_FAILURE;
				}
				break;
			case 'i':
				if (!add_id_filter(&state, optarg)) {
					return EXIT_FAILURE;
				}
				break;
			case 'v':
				PRINT_VERSION("tdfilter");
				return EXIT_SUCCESS;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			case '?':
				print_help();
				return EXIT_FAILURE;
		}
	}

	/* resolve input */
	if (optind < argc) {
		in = fopen(argv[optind], "r");
		if (!in) {
			fprintf(stderr, "Could not open %s: %s.\n", argv[optind], strerror(errno));
			return EXIT_FAILURE;
		}
	} else {
		in = stdin;
	}

	print_filtered(in, &state);
	
	fclose(in);
	
	if (state.filters) {
		list_free(state.filters, &free_d_filter);
	}
	
	return EXIT_SUCCESS;
}

static void print_help() {
	printf("\
Usage: tdfilter [-x index[-to_index]] [-i id[--to_id]] [FILE]\n\
FILE may either be a file containing the output of tdstrip or standard input.\n\
\n\
General options:\n\
  -h, --help       Prints this help.\n\
  -v, --version    Prints the version information.\n\
\n\
Filtering options:\n\
  -x I1[-I2], --index I1[-I2]   Keep all dumps between index I1 and I2 (exclusive)\n\
  -i I1[--I2], --id I1[--I2]    Keep all dumps between I1 and I2 (inclusive)\n\
");
}

static void free_d_filter(void* filterObj) {
	d_filter* filter = (d_filter*) filterObj;
	if (filter->from_id) {
		free(filter->from_id);
	}
	if (filter->to_id) {
		free(filter->to_id);
	}
	free(filter);
}

static bool add_idx_filter(filter_state* state, char* value) {
	const char* range_delim = "-";

	d_filter* filter = (d_filter*) malloc(sizeof(d_filter));
	if (!filter) {
		free(filter);
		fprintf(stderr, "Not enough memory left.\n");
		return false;
	}
	
	filter->from_id = NULL;
	filter->to_id = NULL;
	filter->inside_range = false;

	if (strstr(value, range_delim)) {
		// range
		char* from = strtok(value, range_delim);
		char* to = strtok(NULL, range_delim);
		if (from == NULL || to == NULL) {
			fprintf(stderr, "Ignoring malformed index range filter %s.\n", value);
			free(filter);
			return true;
		}

		filter->from_idx = atoi(from);
		filter->to_idx = atoi(to);
		filter->is_range = true;
	} else {
		// single select
		filter->from_idx = atoi(value);
		filter->is_range = false;
	}

	if (!state->filters) {
		state->filters = list_create(filter);
	} else {
		list_append(state->filters, filter);
	}
	
	return true;
}

static bool add_id_filter(filter_state* state, const char* value) {
	d_filter* filter = (d_filter*) malloc(sizeof(d_filter));
	if (!filter) {
		fprintf(stderr, "Could not allocate memory!\n");
		free(filter);
		return false;
	}
	
	filter->from_id = NULL;
	filter->to_id = NULL;
	filter->inside_range = false;

	char* delim = strstr(value, "--");
	if (delim == NULL) {
		// single select
		filter->is_range = false;
		
		filter->from_id = strdup(value);
	} else {
		// range select
		filter->is_range = true;
		
		int n = delim - value;
		filter->from_id = strndup(value, n);
		
		int len = strlen(value);
		filter->to_id = strndup(delim + 2, len - n);
	}

	if (!state->filters) {
		state->filters = list_create(filter);
	} else {
		list_append(state->filters, filter);
	}
	
	return true;
}

static bool update_range_knowledge(filter_state* state, const char* dump_id, d_filter* filter) {
	bool inside_range = false;
	
	if (filter->from_id) {
		// id-based
		if (filter->inside_range) {
			// wait for range end
			if (strcmp(dump_id, filter->to_id) == 0) {
				filter->inside_range = false;
				// required for inclusive range
				inside_range = true;
			} else {
				inside_range = filter->inside_range;
			}
		} else {
			// wait for range start
			if (strcmp(dump_id, filter->from_id) == 0) {
				filter->inside_range = true;
				inside_range = true;
			}
		}
	} else {
		// index-based
		filter->inside_range = (
			state->curr_idx >= filter->from_idx && 
			state->curr_idx < filter->to_idx);
			
		inside_range = filter->inside_range;
	}
	
	return inside_range;
}

static v_result filter_dump(void* stateObj, const char* id) {
	filter_state* state = (filter_state*) stateObj;
	
	bool accepted = false;
	
	list* filters = state->filters;
	// in worst case we have to update all range information
	// so we can't break out the loop earlier than we'd like to
	while (filters) {
		d_filter* filter = (d_filter*) filters->data;
		
		if (filter->is_range) {
			bool rt = update_range_knowledge(state, id, filter);
			if (!accepted) {
				accepted = rt;
			}
		} else if (!accepted) {
			// check single selection
			if (filter->from_id == NULL) {
				// check index
				accepted = (state->curr_idx == filter->from_idx);
			} else {
				// check id
				accepted = (strcmp(id, filter->from_id) == 0);
			}
		}

		filters = filters->next;
	}
	
	state->curr_idx++;
	
	if (accepted) {
		td_print_dumpstart(id);
		return V_BASE;
	} else {
		return V_BASE | V_SKIP;
	}
}

static v_result copy_dump_end(void* userdata, td_dump* dump, v_result inResult) {
	if ((inResult & V_SKIP) == 0) {
		td_print_dumpend();
	}
	
	return V_BASE;
}

static v_result copy_thread(void* userdata, const char* name, const char* native_id, const char* java_id, td_state state) {
	td_print_threadstart(name, native_id, java_id, state);
	return V_BASE;
}

static v_result copy_thread_end(void* userdata, td_thread* thread, v_result inResult) {
	td_print_threadend();
	return V_BASE;
}

static v_result copy_lock(void* userdata, const char* class, const char* lockObjId, bool isLockOwner) {
	td_print_lock(class, lockObjId, isLockOwner);
	return V_BASE;
}
static v_result copy_code(void* userdata, const char* file, const char* class, unsigned int line) {
	td_print_code(file, class, line);
	return V_BASE;
}
static v_result copy_constructor(void* userdata, const char* file, const char* class, unsigned int line) {
	td_print_constr(file, class, line);
	return V_BASE;
}

static void print_filtered(FILE* in, filter_state* state) {	
	dump_parser parser = td_init();
	
	td_set_dumpv(parser, filter_dump, copy_dump_end);
	td_set_threadv(parser, copy_thread, copy_thread_end);
	td_set_linev(parser, copy_lock, copy_code, copy_constructor);
	td_set_userdata(parser, state);
	
	td_print_dumpsstart();
	td_parse(in, parser);
	td_print_dumpsends();
	
	td_free_parser(parser);
}

