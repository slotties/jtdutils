/******************************************************************************
 * tdgrep - a tool for filtering/grepping data from tdstrip-streams
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
#include <regex.h>
#include <errno.h>

#include "td.h"
#include "list.h"
#include "td_format.h"
#include "common.h"

enum count_result { PASSED, FAILED, UNCHECKED };
struct t_cnt {
	int count;
	enum count_result result;
	char* id;
};

typedef struct {
	list* thread_counts;
	td_filter filter;
	int abs_min;
	bool filter_by_occ;
} grep_state;

static void printd_filtered(FILE* in, grep_state* state);
static void print_help();
static bool t_cnt_equals(const void* t, const void* target);
static void free_t_cnt(void* counter);

int main(int argc, char** argv) {
	int idx;
	int comp_r;

	struct option options[] = {
		{ "pid", no_argument, 0, 'p' }, 
		{ "name", no_argument, 0, 'n' }, 
		{ "stacktrace", no_argument, 0, 't' },
		{ "java-pid", no_argument, 0, 'j' },
		{ "min-occurences", required_argument, 0, 'm' },
		{ "exact", no_argument, 0, 'e' }, 
		{ "starts-with", no_argument, 0, 's' }, 
		{ "contains", no_argument, 0, 'c' },
		{ "regular-expression", no_argument, 0, 'r' }, 
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};

	grep_state state;
	
	state.filter.value.value = NULL;
	state.filter.value.regexp = NULL;
	state.filter.field = STACKTRACE;
	// FIXME state.thread_filter.min_count = 0.0f;
	state.abs_min = 1;
	state.filter_by_occ = false;
	state.filter.fptr_filter = td_tfcont;

	state.thread_counts = NULL;
	
	float min;
	
	for (int c = 0; c >= 0; ) {
		c = getopt_long(argc, argv, ":pntjm:escrvh", options, &idx);
		switch (c) {
			/* thread filter */
			case 'm':
				min = atof(optarg);
				if (min > 1.0f) {
					state.abs_min = (int) min;
				}
				
				state.filter_by_occ = true;
				
				break;
			case 'p':
				state.filter.field = PID;
				break;
			case 'n':
				state.filter.field = NAME;
				break;
			case 't':
				state.filter.field = STACKTRACE;
				break;
			case 'j':
				state.filter.field = JAVA_PID;
				break;
			case 'e':
				state.filter.fptr_filter = td_tfcmp;
				break;
			case 's':
				state.filter.fptr_filter = td_tfstarts;
				break;
			case 'c':
				state.filter.fptr_filter = td_tfcont;
				break;
			case 'r':
				state.filter.fptr_filter = td_tfreg;
				break;

			/* general stuff */
			case 'v':
				PRINT_VERSION("tdgrep");
				return EXIT_SUCCESS;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			case '?':
				print_help();
				return EXIT_FAILURE;
		}
	}

	/* resolve expression */
	if (optind < argc) {
		if (state.filter.fptr_filter == td_tfreg) {
			state.filter.value.regexp = (regex_t*) malloc(sizeof(regex_t));
			comp_r = regcomp(state.filter.value.regexp, argv[optind++], REG_EXTENDED);
			if (comp_r != 0) {
				/* compilation error */
				char error[512];
				regerror(comp_r, state.filter.value.regexp, error, 512);
				fprintf(stderr, "Could not compile regular expression: %s\n", error);
				return EXIT_FAILURE;
			}
		} else {
			state.filter.value.value = argv[optind++];
		}
	} else {
		fprintf(stderr, "No string to grep for defined.\n");
		return EXIT_FAILURE;
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

	printd_filtered(in, &state);
	
	fclose(in);
	
	if (state.filter.value.regexp != NULL) {
		regfree(state.filter.value.regexp);
	}
	
	return EXIT_SUCCESS;
}

void print_help() {
	printf("\
Usage: tdgrep EXPRESSION [FIELD] [METHOD] [-m NUMBER] [FILE]\n\
FILE can be either a file containing the output of tdstrip or be standard input.\n\
\n\
General options:\n\
  -h, --help                           Prints this help.\n\
  -v, --version                        Prints the version information.\n\n\
  -m NUMBER, --min-occurences NUMBER   A filtered thread has to occur in at least NUMBER dumps.\n\
\n\
FIELD is the field which is used for filtering:\n\
  -p, --pid         Native PID\n\
  -j, --java-pid    JVM internal PID\n\
  -n, --name        Thread name\n\
  -t, --stacktrace  Any code line (either class or method) of the stacktrace.\n\
\n\
METHOD is the method used for applying the filter:\n\
  -s, --starts-with                    The field has to start with the expression\n\
  -c, --contains                       The field has to contain the expression\n\
  -e, --exact                          The field has to be exactly the expression\n\
  -r, --regular-expression             The field has to match the regular expression\n\
");
}

/* equals method required for list_find() */
static bool t_cnt_equals(const void* t, const void* target) {
	return (strcmp(((struct t_cnt*) t)->id, target) == 0);
}

static void filter_min_count(grep_state* state, list* dumps) {
	while (dumps) {
		list* t = ((td_dump*) dumps->data)->threads;
		while (t) {
			struct t_cnt* counter = list_find(state->thread_counts, t_cnt_equals, ((td_thread*) t->data)->java_id);
			if (counter->result == UNCHECKED) {
				if (counter->count >= state->abs_min) {
					counter->result = PASSED;
				} else {
					counter->result = FAILED;
				}
			}
			
			list* next = t->next;
			if (counter->result == FAILED) {
				list_delete(&(((td_dump*) dumps->data)->threads), t, td_free_thread);
			}
			t = next;
		}
				
		dumps = dumps->next;
	}
	
	// cleanup
	list_free(state->thread_counts, free_t_cnt);
}

static void add_thread_count(grep_state* state, const char* id) {
	struct t_cnt* counter = NULL;
	if (state->thread_counts) {
		// list exists already, search for node
		counter = list_find(state->thread_counts, t_cnt_equals, id);
	}
		
	if (counter) {
		// increase counter
		counter->count++;
	} else {
		// init counter
		counter = (struct t_cnt*) malloc(sizeof(struct t_cnt));
		counter->id = strdup(id);
		counter->count = 1;
		counter->result = UNCHECKED;
		
		if (state->thread_counts) {
			list_append(state->thread_counts, counter);
		} else {
			state->thread_counts = list_create(counter);
		}
	}
}

static v_result filter_by_attribute(void* gstatep, const char* name, const char* native_id, const char* java_id, 
	td_state state) {

	grep_state* gstate = (grep_state*) gstatep;

	const char* val = NULL;
	switch (gstate->filter.field) {
		case NAME:
			val = name;
			break;
		case JAVA_PID:
			val = java_id;
			break;
		case PID:
			val = native_id;
			break;
		default:
			val = NULL;
			break;
	};
	
	v_result r = V_BASE;
	if (val) {
		if (!gstate->filter.fptr_filter(val, &(gstate->filter.value))) {
			r |= V_SKIP;
		} else {			
			if (!gstate->filter_by_occ) {
				td_print_threadstart(name, native_id, java_id, state);
			} else {
				r |= V_KEEP;
			}
		}
	} else {
		r |= V_KEEP;
	}
	
	return r;
}

static v_result copy_lock(void* statep, const char* class, const char* lockObjId, bool isLockOwner) {
	grep_state* state = (grep_state*) statep;
	
	if (state->filter.field == STACKTRACE || state->filter_by_occ) {
		return V_BASE | V_KEEP;
	} else {
		td_print_lock(class, lockObjId, isLockOwner);
		return V_BASE;
	}
}
static v_result copy_code(void* statep, const char* file, const char* class, unsigned int line) {
	grep_state* state = (grep_state*) statep;
	
	if (state->filter.field == STACKTRACE || state->filter_by_occ) {
		return V_BASE | V_KEEP;
	} else {
		td_print_code(file, class, line);
		return V_BASE;
	}	
}
static v_result copy_constructor(void* statep, const char* file, const char* class, unsigned int line) {
	grep_state* state = (grep_state*) statep;
	
	if (state->filter.field == STACKTRACE || state->filter_by_occ) {
		return V_BASE | V_KEEP;
	} else {
		td_print_constr(file, class, line);
		return V_BASE ;
	}
}

static v_result filter_by_object(void* statep, td_thread* thread, v_result inResult) {
	v_result r = V_BASE;
	
	grep_state* state = (grep_state*) statep;
	
	if (state->filter.field == STACKTRACE) {
		// late accepting
		if (td_filtermatches(thread, &(state->filter))) {
			if (!state->filter_by_occ) {
				// print the whole thread now
				td_print_thread(thread);
			} else {
				r |= V_KEEP;
			}
		}
	} else {
		// if it was *not* skipped, we print the end now
		if ((inResult & V_SKIP) == 0) {
			if (!state->filter_by_occ) {
				td_print_threadend();
			} else {
				r |= V_KEEP;
			}
		}
	}
	
	if ((r & V_KEEP) == V_KEEP) {
		add_thread_count(state, thread->java_id);
	}
		
	return r;
}

v_result copy_dump_start(void* state, const char* id) {
	if (!((grep_state*) state)->filter_by_occ) {
		td_print_dumpstart(id);
		return V_BASE;
	} else {
		return V_BASE | V_KEEP;
	}
}

v_result copy_dump_end(void* state, td_dump* dump, v_result in) {
	if (!((grep_state*) state)->filter_by_occ) {
		td_print_dumpend();
		return V_BASE;
	} else {
		return V_BASE | V_KEEP;
	}
}

void printd_filtered(FILE* in, grep_state* state) {
	dump_parser parser = td_init();

	td_set_dumpv(parser, copy_dump_start, copy_dump_end);
	td_set_threadv(parser, filter_by_attribute, filter_by_object);
	td_set_linev(parser, copy_lock, copy_code, copy_constructor);
	
	td_set_userdata(parser, state);
		
	td_print_dumpsstart();
	td_parse(in, parser);
		
	if (state->filter_by_occ) {
		// now do the min_count stuff
		list* dumps = td_getdumps(parser);
		filter_min_count(state, dumps);
		
		while (dumps) {
			td_print_dump(dumps->data);
			dumps = dumps->next;
		}
	}
	
	td_print_dumpsends();
	
	td_free_parser(parser);
}

static void free_t_cnt(void* counterp) {
	struct t_cnt* counter = (struct t_cnt*) counterp;
	
	free(counter->id);
	free(counter);
}
