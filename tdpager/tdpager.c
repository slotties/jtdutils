/******************************************************************************
 * tdpager - a tool paging between thread dumps and threads
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
#include <errno.h>
#include <curses.h>

#include "td.h"
#include "common.h"
#include "td_format.h"

struct PagerFunction {
	int key;
	bool (*fn)();
};

static void print_help();
static void page_threads(FILE* in, struct PagerFunction* fn);
static bool compare_same_thread(void* data, void* );

static void screen_init();
static void screen_destroy();
static void update_display();

// pager functions
static bool quit();
static bool show_dump(list* dump_node);
static bool nextd_dump();
static bool prev_dump();
static bool show_thread(list* thread_node);
static bool next_thread();
static bool prev_thread();
static bool stick_thread();
static bool unstick_thread();

static bool line_down();
static bool line_up();

#define PAGE_SIZE 10
static bool page_down();
static bool page_up();

// state 
list* DUMPS = NULL;
list* CURRENtd_dump = NULL;
list* CURRENT_THREAD = NULL;

int LEN_DUMPS = -1;
int LEN_THREADS = -1;
int stack_offset = 0;
td_thread* STICKY_THREAD = NULL;

int main(int argc, char** argv) {
	int idx;
	
	struct option options[] = {
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};
	
	for (int c = 0; c >= 0; ) {
		c = getopt_long(argc, argv, ":vh", options, &idx);
		switch (c) {
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
		fprintf(stderr, "tdprobe does not support stdin-redirection yet.\n");
		return EXIT_FAILURE;
	}
	
	// initialize paging functions
	struct PagerFunction functions[] = {
		{ 'q', quit },
		
		{ 't', nextd_dump },
		{ 'T', prev_dump },
		{ 'n', next_thread },
		{ 'b', prev_thread },
		{ KEY_RIGHT, next_thread },
		{ KEY_LEFT, prev_thread },
		
		{ 's', stick_thread },
		{ 'u', unstick_thread },
		
		{ KEY_UP, line_up },
		{ KEY_DOWN, line_down },
		{ KEY_PPAGE, page_up },
		{ KEY_NPAGE, page_down },
		
		{ -1, NULL }
	};
	
	screen_init();
	page_threads(in, functions);
	screen_destroy();
	
	fclose(in);
	
	return EXIT_SUCCESS;
}

static void screen_init() {
	initscr();
	// configure input
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	
	curs_set(0);
	
	refresh();
}

static void screen_destroy() {
	endwin();
}

static void page_threads(FILE* in, struct PagerFunction* functions) {
	dump_parser parser = td_init();
	td_parse(in, parser);

	DUMPS = td_getdumps(parser);
	show_dump(DUMPS);

	char c;
	struct PagerFunction* tmp;
	bool quit = false;
	while (!quit) {
		update_display();
		refresh();
		
		c = getch();

		// search function
		tmp = functions;
		while ((*tmp).key != -1 && (*tmp).key != c) {
			tmp++;
		}
		
		// execute it, when one is found
		if ((*tmp).key != -1) {
			quit = !(*tmp).fn();
		}
	}
	
	td_free_parser(parser);	
}

static void print_help() {
	printf("\
Usage: tdpager [-vh] [FILE]\n\
FILE may either be a file containing the output of tdstrip or standard input.\n\
\n\
General options:\n\
  -h, --help                   Prints this help.\n\
  -v, --version                Prints the version information.\n\
\n");
}

static bool quit() {
	return false;
}

static bool show_dump(list* dump_node) {
	if (dump_node) {
		CURRENtd_dump = dump_node;
		LEN_THREADS = -1;
		
		list* threads = ((td_dump*) CURRENtd_dump->data)->threads;
		list* thread = NULL;
		if (STICKY_THREAD) {
			thread = list_find_node(threads, compare_same_thread, STICKY_THREAD);
		}
		
		if (!thread) {
			thread = threads;
		}
		
		show_thread(thread);
	}
		
	return true;
}

static bool nextd_dump() {
	return show_dump(CURRENtd_dump->next);
}

static bool prev_dump() {
	return show_dump(list_get_pnode(DUMPS, CURRENtd_dump));
}

static bool show_thread(list* thread_node) {
	if (!thread_node) {
		return true;
	}
	
	if (LEN_DUMPS < 0) {
		LEN_DUMPS = list_len(DUMPS);
	}
	if (LEN_THREADS < 0) {
		LEN_THREADS = list_len(((td_dump*) CURRENtd_dump->data)->threads);
	}
	
	CURRENT_THREAD = thread_node;
	stack_offset = 0;
	
	return true;
}
	
static void update_display() {
	clear();
	
	// display thread information
	td_thread* t = (td_thread*) CURRENT_THREAD->data;
	td_dump* d = (td_dump*) CURRENtd_dump->data;
	
	char t_index[64];
	sprintf(t_index, "(%d/%d)", 
		list_getidx(d->threads, CURRENT_THREAD) + 1, LEN_THREADS);
					
	char d_index[64];
	sprintf(d_index, "(%d/%d)", 
		list_getidx(DUMPS, CURRENtd_dump) + 1, LEN_DUMPS);
			
	mvprintw(0, 0, "%-69s %10s", d->id, d_index);
	mvprintw(1, 0, "%-69s %10s", t->name, t_index);
		
	mvprintw(2, 0, "State: %s, PID: %s, Java PID: %s",
		state2str(t->state), t->native_id, t->java_id
	);
		
	int line_offset = 3;
	int max_line = LINES - 1 + stack_offset;
	int y = line_offset;
	for (list* st_line = t->stacktrace; st_line && y < max_line; st_line = st_line->next) {		
		if ((y - stack_offset - line_offset) < 0) {
			y++;
			continue;
		}
		
		td_line* line = (td_line*) st_line->data;
		
		switch (line->type) {
			case LOCK:
				if (line->isLockOwner) {
					mvprintw(y - stack_offset, 4, 
						"- locked <%s> (a %s)", line->lockObjId, line->class);
				} else {
					mvprintw(y - stack_offset, 4, 
						"- waiting on <%s> (a %s)", line->lockObjId, line->class);
				}
				break;
				
			case CODE:
				mvprintw(y - stack_offset, 4, 
					"at %s(%s:%d)", line->class, line->file, line->line);				
				break;
				
			case CONSTRUCTOR:
				mvprintw(y - stack_offset, 4, 
					"at %s<init> (%s:%d)", line->class, line->file, line->line);				
				break;
		}
		
		y++;
	}
	
	if (STICKY_THREAD) {
		mvprintw(LINES - 1, 0, "Sticky thread: %s (%s)", 
			STICKY_THREAD->name, STICKY_THREAD->native_id);
	}
}

static bool next_thread() {
	return show_thread(CURRENT_THREAD->next);
}

static bool prev_thread() {
	return show_thread(list_get_pnode(((td_dump*) CURRENtd_dump->data)->threads, CURRENT_THREAD));
}

static bool compare_same_thread(void* data, void* sticky_thread) {
	char* current = ((td_thread*) sticky_thread)->native_id;
	char* next = ((td_thread*) data)->native_id;
	
	return strcmp(current, next) == 0;
}

static bool stick_thread() {
	STICKY_THREAD = ((td_thread*) CURRENT_THREAD->data);
	return true;
}

static bool unstick_thread() {
	STICKY_THREAD = NULL;
	return true;
}

static bool line_down() {
	list* threads = ((td_thread*) CURRENT_THREAD->data)->stacktrace;
	int len = list_len(threads);
	if ((len - stack_offset) > (LINES - 2)) {
		stack_offset++;
	}
	
	return true;
}

static bool line_up() {
	if (stack_offset > 0) {
		stack_offset--;
	}
	
	return true;
}

static bool page_down() {
	list* threads = ((td_thread*) CURRENT_THREAD->data)->stacktrace;
	int len = list_len(threads);
	if ((len - stack_offset + PAGE_SIZE) > (LINES - 2)) {
		stack_offset += PAGE_SIZE;
	} else {
		stack_offset = len - PAGE_SIZE;
	}
	
	return true;
}

static bool page_up() {
	if (stack_offset >= PAGE_SIZE) {
		stack_offset -= PAGE_SIZE;
	} else {
		stack_offset = 0;
	}
	
	return true;
}
