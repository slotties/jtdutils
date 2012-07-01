/******************************************************************************
 * tdstrip_sun.c - parsing code for SUN JVM's (1.4, 1.5, 1.6)
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

#include <string.h>
#include <stdlib.h>

#include "td.h"
#include "td_format.h"
#include "tdstrip.h"
#include "list.h"

// Read buffer.
#define BUF_SIZE 512
char BUFFER[BUF_SIZE];

/*
 *  Remember previous buffer in case we have to extract
 * the date from it.
 */
#define PREV_BUF_SIZE 20
char PREV_BUFFER[PREV_BUF_SIZE];

/*
 * The pattern used for matching dates. Each ? must match a number while every
 * other character must exactly match in the target buffer.
 */
const char* date_pattern = "\?\?\?\?-\?\?-\?\? \?\?:\?\?:\?\?";

static bool trim_date_sun15(char* buffer) {
	const char* p = date_pattern;
	while (*buffer && *p) {
		if (*p == '?') {
			// must be a number
			if (*buffer < 48 && *buffer > 57) {
				return false;
			}
		} else {
			if (*buffer != *p) {
				return false;
			}
		}
		
		buffer++;
		p++;
	}
	
	*buffer = '\0';
	
	return (*p == '\0');
}

static bool sun_handle_possible_dump_start(int* dumpIndex) {
	if (strstarts(BUFFER, "Full thread dump")) {
		// next dump started -> print next dump start
		if (!trim_date_sun15(PREV_BUFFER)) {
			snprintf(PREV_BUFFER, PREV_BUF_SIZE, "%d", ++(*dumpIndex));
		}

		td_print_dumpstart(PREV_BUFFER);
		return true;
	} else {
		// remember current buffer
		strncpy(PREV_BUFFER, BUFFER, 30);
		return false;
	}
}

/*
 * read thread meta line which looks like this:
 * "NAME" prio=PRIO tid=JAVA_PID nid=PID STATE [MEMOFFSET]
 */
static void read_thread_sun() {
	static char name[128];
	static char java_id[128];
	static char native_id[128];
		
	int n;
	// ignore first char ('"')
	char* working = BUFFER + 1;
	
	// read name (value until next '"')
	n = strchr(working, '\"') - working;
	strncpy(name, working, n);
	name[n] = '\0';

	// read java pid, as we don't know how long 'prio' is
	// we just search for 'tid'
	working = strstr(working + n, "tid") + 4;
	n = strchr(working, ' ') - working;
	strncpy(java_id, working, n);
	java_id[n] = '\0';

	// read native pid
	working += n + 5;
	n = strchr(working, ' ') - working;
	strncpy(native_id, working, n);
	native_id[n] = '\0';
	
	// skip value
	working += n;
	
	td_state state;
	// last part: state
	if (strstr(working, "runnable")) {
		state = RUNNING;
	} else if (strstr(working, "Object")) {
		state = OBJECT_WAIT;
	} else if (strstr(working, "condition")) {
		state = WAITING_ON_CONDITION;
	} else if (strstr(working, "monitor")) {
		state = WAITING_FOR_MONITOR_ENTRY;
	} else if (strstr(working, "sleep")) {
		state = SLEEPING;
	} else {
		fprintf(stderr, "Unknown state defined in thread: %s. Please \
mail this to slotties@gmail.com in order to get this fixed.\n", BUFFER);
		state = UNKNOWN;
	}
		
	td_print_threadstart(name, native_id, java_id, state);
}

static void sun_handle_stacktrace() {
	static char file[128];
	static char class[1024];

	// first char is always \t, skip it
	char* working = BUFFER + 1;
	int n;
	
	if (strstarts(working, "at")) {
		int line;
		bool isConstructor = false;
		
		// skip 'at '
		working += 3;
		
		// strip code
		n = strchr(working, '(') - working;
		// constructors look like this: my.package.Class.<init>
		char* constStart = strchr(working, '<');
		if (constStart) {
			// line->class = strndup(working, constStart - working - 1);
			strncpy(class, working, constStart - working - 1);
			class[constStart - working - 1] = '\0';
			isConstructor = true;
		} else {
			// line->class = strndup(working, n);
			strncpy(class, working, n);
			class[n] = '\0';
		}
		
		// skip value + '('
		working += n + 1;
		
		char* delim = strchr(working, ':');
		if (delim) {
			// is real code
			strncpy(file, working, delim - working);
			file[delim - working] = '\0';
			// skip ':', atoi actually ignores the ending ')' so
			// we don't have to extract the number-substring
			line = atoi(delim + 1);
		} else {
			// probably is 'Native Method'
			strcpy(file, "NATIVE");
			line = -1;
		}
		
		if (isConstructor) {
			td_print_constr(file, class, line);
		} else {
			td_print_code(file, class, line);
		}
	} else if (working[0] == '-') {
		static char lockObjId[128];
	
		// skip '- '
		working += 2;
		
		// FIXME: handle 'parking to wait for'
		bool isLockOwner = strstarts(working, "locked");
		
		// skip prolog
		working = strchr(working, '<') + 1;
		
		// extract lock address
		n = strchr(working, '>') - working - 1;
		// line->lockObjId = strndup(working, n);
		strncpy(lockObjId, working, n);
		lockObjId[n] = '\0';
		
		// skip '[...](a '
		working = strchr(working, '(') + 3;
		// extract class of locking object
		n = strchr(working, ')') - working;
		// line->class = strndup(working, n);
		strncpy(class, working, n);
		class[n] = '\0';
		
		td_print_lock(class, lockObjId, isLockOwner);
	} else {
		// FIXME: unknown stacktrace line
		fprintf(stderr, 
			"Unhandled stacktrace line:\n%s\nPlease send this line to slotties@gmail.com.\n", 
			BUFFER);
		return;
	}
}

e_extr extract_sun(FILE* in) {	
	PREV_BUFFER[0] = '\0';
	int dumpIndex = 0;
	
	bool threadOpened = false;
	bool dumpOpen = false;
	
	td_print_dumpsstart();
	while (fgets(BUFFER, BUF_SIZE, in) != NULL) {
		if (dumpOpen) {
			// read threads in dump
			if (BUFFER[0] == '"') {
				if (threadOpened) {
					td_print_threadend();
				}
				
				read_thread_sun();
				
				threadOpened = true;
			} else if (BUFFER[0] == '\t') {
				// stacktrace element
				sun_handle_stacktrace();
			} else if (BUFFER[0] != '\n' && BUFFER[0] != ' ') {
				if (threadOpened) {
					td_print_threadend();
					threadOpened = false;
				}
				
				td_print_dumpend();
				dumpOpen = sun_handle_possible_dump_start(&dumpIndex);
			}
		} else {
			// check if a dump is started
			dumpOpen = sun_handle_possible_dump_start(&dumpIndex);
			threadOpened = false;
		}
	}
	// 'flush'
	if (dumpOpen) {
		if (threadOpened) {
			td_print_threadend();
		}
		
		td_print_dumpend();
	}
	td_print_dumpsends();

	return SUCCESS;
}
