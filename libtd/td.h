/******************************************************************************
 * td.h - an interface defining structures and functions for working with
 *        java thread dumps
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

#ifndef TD_H
#define TD_H

#include <stdio.h>
#include <regex.h>

#include "common.h"
#include "list.h"

#define STATE_RUN "RUN"
#define STATE_OBJ_WAIT "OBJ_WAIT"
#define STATE_WAIT_COND "WAIT_COND"
#define STATE_WAIT_MON "WAIT_MON"
#define STATE_SLEEP "SLEEP"
#define STATE_UNKNOWN "UNKNOWN"

typedef enum { RUNNING = 1, OBJECT_WAIT = 2, WAITING_ON_CONDITION = 3, WAITING_FOR_MONITOR_ENTRY = 4, SLEEPING = 5, UNKNOWN = 6 } td_state;

typedef struct {
	char* name;
	char* native_id;
	char* java_id;
	td_state state;
	list* stacktrace;	
}  td_thread;

typedef enum { LOCK, CODE, CONSTRUCTOR } td_line_type;
typedef struct {
	td_line_type type;
	unsigned int line;
	char* file;
	char* class;
	
	char* lockObjId;
	bool isLockOwner;
} td_line;

#define TD_FILE_NATIVE "NATIVE"

typedef struct {
	char* value;
	regex_t* regexp;
} td_tfval;

typedef enum { NAME = 1, JAVA_PID = 2, PID = 4, STATE = 8, STACKTRACE = 16 } td_tfield;

typedef struct {
	td_tfval value;
	td_tfield field;
	bool (*fptr_filter)(const char*, const td_tfval*);
} td_filter;

typedef struct {
	char* id;
	list* threads;
} td_dump;

void td_free_tf(td_filter* filter);
void td_free_thread(td_thread* thread);
void td_free_dump(td_dump* dump);

td_state str2state(const char* buffer);
const char* state2str(td_state state);

bool td_isnative(const td_line* line);

// Filtering functions
bool td_tfcmp(const char* value, const td_tfval* tfv);
bool td_tfstarts(const char* value, const td_tfval* tfv);
bool td_tfcont(const char* value, const td_tfval* tfv);
bool td_tfreg(const char* value, const td_tfval* tfv); 
bool td_filtermatches(const td_thread* meta, const td_filter* filter);

bool strstarts(const char* buffer, const char* target);

#endif
