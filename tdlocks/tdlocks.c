/******************************************************************************
 * tdlocks - a tool for displaying all locks in thread dumps
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
#include "td_format.h"
#include "common.h"
#include "list.h"

typedef struct {
	char* lock_id;
	char* lock_class;
	list* owner;
	list* waiting;
} t_lock;

typedef struct {
	int minimum_waiting;
	char* lock_class_filter;
	list* locks;
} lock_filter;

static v_result collect_locks(lock_filter* filter, td_thread* thread, v_result inResult);
static void free_t_lock(t_lock* locks);
static void td_print_locks(FILE* in, lock_filter* filter);
static bool lock_matches(lock_filter* filter, t_lock* lock);
static void print_help();

int main(int argc, char** argv) {
	int idx;

	/* TODO: some options like order or filtering by lock-id */
	struct option options[] = {
		{ "minimum-waiting", required_argument, 0, 'w' },
		{ "lock-class", required_argument, 0, 'c' },

		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};

	lock_filter filter;
	filter.minimum_waiting = 0;
	filter.locks = NULL;
	filter.lock_class_filter = NULL;
	
	for (int c = 0; c >= 0; ) {
		c = getopt_long(argc, argv, ":vhw:c:", options, &idx);
		switch (c) {
			/* filtering */
			case 'w':
				filter.minimum_waiting = atoi(optarg);
				break;
			case 'c':
				filter.lock_class_filter = optarg;
				break;

			/* general stuff */
			case 'v':
				PRINT_VERSION("tdlocks");
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

	td_print_locks(in, &filter);
	
	fclose(in);
	
	return EXIT_SUCCESS;
}

static void print_help() {
	printf("\
Usage: tdlocks [OPTIONS] [FILE]\n\
FILE may either be a file containing the output of tdstrip or standard input.\n\
\n\
General options:\n\
  -h, --help                   Prints this help.\n\
  -v, --version                Prints the version information.\n\n\
Filtering options:\n\
  -w NUMBER, --minimum-waiting NUMBER Just print locks that have at least\n\
                                      NUMBER of threads waiting.\n\
  -c STRING, --lock-class STRING      Just print locks on objects having\n\
                                      STRING in their full qualified class\n\
                                      names.\n");
}

/* equal method for list_find() */
static bool lock_equals(const void* lock, const void* target_lock_id) {
	if (strcmp(((t_lock*) lock)->lock_id, target_lock_id) == 0) {
		return true;
	} else {
		return false;
	}
}

static v_result keep_dump(void* userdata, const char* id) {
	return V_BASE | V_KEEP;
}

static v_result output_locks(lock_filter* filter, td_dump* dump, v_result inResult) {
	list* locks = filter->locks;

	printf("Locks in dump %s\n", dump->id);
	while (locks) {
		list* itmp;
		t_lock* lock = (t_lock*) locks->data;

		// apply lock filter 
		if (lock_matches(filter, lock)) {
			printf("Lock: %s (%s)\n", lock->lock_id, lock->lock_class);
			if (lock->owner == NULL) {
				printf("Owners: NONE, that's strange!\n");
			} else {
				int count = list_len(lock->owner);
				printf("Owners: %d\n", count);
				for (itmp = lock->owner; itmp != NULL; itmp = itmp->next) {
					// printf("\t%s\n", ((td_thread*) itmp->data)->name);
					printf("\t%s\n", (char*) itmp->data);
				}
			}

			if (lock->waiting == NULL) {
				printf("Waiting threads: none\n");
			} else {
				int count = list_len(lock->waiting);
				printf("Waiting threads: %d\n", count);
				for (itmp = lock->waiting; itmp != NULL; itmp = itmp->next) {
					// printf("\t%s\n", ((td_thread*) itmp->data)->name);
					printf("\t%s\n", (char*) itmp->data);
				}
			}

			printf("\n");
		}

		locks = locks->next;
	}
	
	printf("\n");

	if (filter->locks) {
		// reset LOCKS
		list_free(filter->locks, free_t_lock);
		filter->locks = NULL;
	}
		
	return V_BASE;
}

static v_result keep_thread(const char* id, const char* native_id, const char* java_id, td_state state) {
	return V_BASE | V_KEEP;
}

static void td_print_locks(FILE* in, lock_filter* filter) {
	dump_parser parser = td_init();
	td_set_dumpv(parser, keep_dump, output_locks);
	td_set_threadv(parser, keep_thread, collect_locks);
	td_set_userdata(parser, filter);
	
	td_parse(in, parser);
	td_free_parser(parser);
}

static bool lock_matches(lock_filter* filter, t_lock* lock) {
	int waiting = 0;
	if (lock->waiting != NULL) {
		waiting = list_len(lock->waiting);
	}

	if (waiting < filter->minimum_waiting) {
		return false;
	}
	if (filter->lock_class_filter != NULL &&
			strstr(lock->lock_class, filter->lock_class_filter) == NULL) {
		return false;
	}

	return true;
}

static v_result collect_locks(lock_filter* filter, td_thread* thread, v_result inResult) {
	list* trace;
	td_line* next;

	for (trace = thread->stacktrace; trace != NULL; trace = trace->next) {
		next = (td_line*) trace->data;
		
		if (next->type == LOCK) {
			// just respect locks
			t_lock* lock;
			bool owner;

			if (filter->locks == NULL) {
				lock = (t_lock*) malloc(sizeof(t_lock));
				lock->owner = NULL;
				lock->waiting = NULL;
				lock->lock_id = strdup(next->lockObjId);
				lock->lock_class = strdup(next->class);
				filter->locks = list_create(lock);
			} else {
				// target_lock_id is being used by lock_equals
				lock = list_find(filter->locks, lock_equals, next->lockObjId);
				if (lock == NULL) {
					lock = (t_lock*) malloc(sizeof(t_lock));
					lock->owner = NULL;
					lock->waiting = NULL;
					lock->lock_id = strdup(next->lockObjId);
					lock->lock_class = strdup(next->class);
					list_append(filter->locks, lock);
				} 				
			}

			owner = next->isLockOwner;
			
			char* tname = strdup(thread->name);

			// TODO: more differenced checks? 
			if (owner && lock->owner == NULL) {
				// lazy create owner list 
				lock->owner = list_create(tname);

			} else if (!owner && lock->waiting == NULL) {
				// lazy create waiting list 
				lock->waiting = list_create(tname);
			} else {
				list* target_list = (owner ? lock->owner : lock->waiting);
				list_append(target_list, tname);
			}
		}
	}
	
	return V_BASE;
}

static void free_t_lock(t_lock* lock) {
	free(lock->lock_id);
	free(lock->lock_class);
	list_free(lock->owner, free);
	list_free(lock->waiting, free);
	free(lock);
}
