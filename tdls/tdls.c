/******************************************************************************
 * tdls - a tool for listing all threads of a tdstrip-stream
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
#include <stdbool.h>
#include <errno.h>

#include "td.h"
#include "td_format.h"
#include "common.h"
#include "list.h"

#define hex2dec(str) strtoul(str, NULL, 16)

struct thread_output_options {
	bool print_hex_pids;
	td_tfield sort_by;
	bool desc_order;
	unsigned short int fields;
	list* thread_list;
} OUTPUT;

static void print_td_list(FILE* in);
static void td_list(const char* id, list* threads);
static void print_help();

int main(int argc, char** argv) {
	struct option options[] = {
		// sort options
		{ "sort-jpid", no_argument, 0, 'J' },
		{ "sort-pid", no_argument, 0, 'P' },
		{ "sort-name", no_argument, 0, 'N' },
		{ "sort-state", no_argument, 0, 'S' },
		
		// special field selectors
		{ "java-pid", no_argument, 0, 'j' },

		// other options
		{ "hex-pids", no_argument, 0, 'H' }, 
		{ "reverse", no_argument, 0, 'r' },
		
		// common stuff
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};
	
	// define default output options
	OUTPUT.fields = NAME | PID | STATE;
	OUTPUT.sort_by = NAME;
	OUTPUT.desc_order = false;
	OUTPUT.print_hex_pids = false;
	OUTPUT.thread_list = NULL;
	
	int idx;
	for (int c = 0; c >= 0; ) {
		c = getopt_long(argc, argv, ":JPNSjHrvh", options, &idx);
		switch (c) {
			case 'J':
				OUTPUT.sort_by = JAVA_PID;
				break;
			case 'N':
				OUTPUT.sort_by = NAME;
				break;
			case 'P':
				OUTPUT.sort_by = PID;
				break;
			case 'S':
				OUTPUT.sort_by = STATE;
				break;
				
			case 'j':
				OUTPUT.fields |= JAVA_PID;
				break;
								
			case 'H':
				OUTPUT.print_hex_pids = true;
				break;
			case 'r':
				OUTPUT.desc_order = false;
				break;
				
			case 'v':
				PRINT_VERSION("tdls");
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
	
	print_td_list(in);
	
	fclose(in);
	
	return EXIT_SUCCESS;
}

static void print_help() {
	printf("Usage: tdls [-P|-N|-S] [-j] [-Hr] [FILE]\n\
FILE may either be a file containing the output of tdstrip or standard input.\n\
\n\
General options:\n\
  -H, --hex-pids         Print all thread ID's in hexadecimal rather than\n\
                         decimal format.\n\
  -r, --reverse          Reverses the sort order.\n\
  -h, --help             Prints this help.\n\
  -v, --version          Prints the version information.\n\n\
Sort options:\n\
  -P, --sort-pid         Sort by native PID.\n\
  -J, --sort-jpid        Sort by java PID.\n\
  -N, --sort-name        Sort by thread name.\n\
  -S, --sort-state       Sort by thread state.\n\n\
Field selectors:\n\
  -j, --java-pid         Display the java PID.\
\n");
}

static v_result ls_dump(void* userdata, td_dump* dump, v_result inResult) {
	td_list(dump->id, OUTPUT.thread_list);
	
	list_free(OUTPUT.thread_list, NULL);
	OUTPUT.thread_list = NULL;
	
	return V_BASE;
}

static v_result keep_dumps(void* userdata, const char* id) {
	return (V_BASE | V_KEEP);
}

static v_result skip_stacktrace(void* userdata, const char* name, const char* native_id, const char* java_id, const td_state state) {
	return (V_BASE | V_SKIP | V_KEEP);
}

static int pid_cmp(const char* p1, const char* p2) {
	unsigned long n1 = hex2dec(p1);
	unsigned long n2 = hex2dec(p2);
	
	if (n1 > n2) {
		return 1;
	} else if (n1 < n2) {
		return -1;
	} else {
		return 0;
	}
}

static int state_cmp(td_state t1, td_state t2) {
	if (t1 > t2) {
		return 1;
	} else if (t1 < t2) {
		return -1;
	} else {
		return 0;
	}
}

static int thread_cmp(const td_thread* data, const td_thread* other) {	
	switch (OUTPUT.sort_by) {
		case NAME:
			return strcmp(data->name, other->name);
					
		case PID:
			return pid_cmp(data->native_id, other->native_id);
			
		case JAVA_PID:
			return pid_cmp(data->java_id, other->java_id);
			
		case STATE:
			return state_cmp(data->state, other->state);
		
		default:
			return 0;
	};
}

static v_result add_thread(void* userdata, td_thread* thread, v_result inResult) {
	if (OUTPUT.thread_list) {
		list_insert_by(&(OUTPUT.thread_list), thread, thread_cmp);
	} else {
		OUTPUT.thread_list = list_create(thread);
	}
	
	return (V_BASE | V_KEEP);
}

static void print_td_list(FILE* in) {
	dump_parser parser = td_init();
	td_set_dumpv(parser, keep_dumps, ls_dump);
	td_set_threadv(parser, skip_stacktrace, add_thread);
	
	td_parse(in, parser);
	td_free_parser(parser);
}

static void td_list(const char* id, list* threads) {
	char state;
	
	printf("\n%s\n", id);
	
	printf("Total threads: %d\n", list_len(threads));
	
	while (threads) {
		td_thread* thread = (td_thread*) threads->data;
		if ((OUTPUT.fields & JAVA_PID) == JAVA_PID) {
			if (OUTPUT.print_hex_pids) {
				printf("%10s ", thread->java_id);
			} else {
				printf("%20ld ", hex2dec(thread->java_id));
			}
		}
		if ((OUTPUT.fields & PID) == PID) {
			if (OUTPUT.print_hex_pids) {
				printf("%20s ", thread->native_id);
			} else {
				printf("%20ld ", hex2dec(thread->native_id));
			}
		}
		
		switch (thread->state) {
			case RUNNING:
				state = 'R';
				break;
			case OBJECT_WAIT:
				state = 'O';
				break;
			case WAITING_ON_CONDITION:
				state = 'C';
				break;
			case WAITING_FOR_MONITOR_ENTRY:
				state = 'M';
				break;
			case SLEEPING:
				state = 'S';
				break;
			case UNKNOWN:
				state = 'U';
				break;
		}
		
		printf("%c ", state);

		printf("%s\n", thread->name);

		threads = threads->next;
	}
}
