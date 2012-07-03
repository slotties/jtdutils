/******************************************************************************
 * td.c - an implementation of td.h
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include <expat.h>

#include "td.h"
#include "common.h"
#include "list.h"

void td_free_tf(td_filter* filter) {
	if (filter->value.value != NULL) {
		free(filter->value.value);
		filter->value.value = NULL;
	}
	if (filter->value.regexp != NULL) {
		regfree(filter->value.regexp);
		filter->value.regexp = NULL;
	}
}

static void free_td_line(td_line* line) {
	if (line->file && !td_isnative(line))
		free(line->file);
		
	free(line->class);	
	free(line->lockObjId);
	free(line);
}

void td_free_thread(td_thread* thread) {
	if (thread == NULL) {
		return;
	}

	if (thread->name) {
		free(thread->name);
		thread->name = NULL;
	}
	if (thread->native_id) {
		free(thread->native_id);
		thread->native_id = NULL;
	}
	if (thread->java_id) {
		free(thread->java_id);
		thread->java_id = NULL;
	}
	if (thread->stacktrace) {
		list_free(thread->stacktrace, &free_td_line);
		thread->stacktrace = NULL;
	}
	
	free(thread);
}

void td_free_dump(td_dump* dump) {
	free(dump->id);
	list_free(dump->threads, &td_free_thread);
	free(dump);
}

td_state str2state(const char* buffer) {
	if (strstarts(buffer, STATE_RUN)) {
		return RUNNING;
		
	} else if (strstarts(buffer, STATE_OBJ_WAIT)) {
		return OBJECT_WAIT;
		
	} else if (strstarts(buffer, STATE_WAIT_COND)) {
		return WAITING_ON_CONDITION;
		
	} else if (strstarts(buffer, STATE_WAIT_MON)) {
		return WAITING_FOR_MONITOR_ENTRY;
		
	} else if (strstarts(buffer, STATE_SLEEP)) {
		return SLEEPING;
		
	} else {
		return UNKNOWN;
	}
}

const char* state2str(td_state state) {
	switch(state) {
		case RUNNING:
			return STATE_RUN;
		case OBJECT_WAIT:
			return STATE_OBJ_WAIT;
		case WAITING_ON_CONDITION:
			return STATE_WAIT_COND;
		case WAITING_FOR_MONITOR_ENTRY:
			return STATE_WAIT_MON;
		case SLEEPING:
			return STATE_SLEEP;
		default:
			return STATE_UNKNOWN;
	}
}

bool td_filtermatches(const td_thread* meta, const td_filter* filter) {
	if (filter->value.value != NULL || filter->value.regexp != NULL) {
		list* line;

		switch (filter->field) {
			case NAME:
				return filter->fptr_filter(meta->name, &(filter->value));
			case JAVA_PID:
				return filter->fptr_filter(meta->java_id, &(filter->value));
			case PID:
				return filter->fptr_filter(meta->native_id, &(filter->value));
			case STACKTRACE:
				line = meta->stacktrace;
				while (line != NULL) {
					td_line* l = (td_line*)line->data;
					if (l->class && filter->fptr_filter(l->class, &(filter->value))) {
						return true;
					}

					line = line->next;
				}
				break;
		}
	}
	
	return false;
}

bool td_tfcmp(const char* value, const td_tfval* tfv) {
	return (strcmp(value, tfv->value) == 0);
}

bool td_tfstarts(const char* value, const td_tfval* tfv) {
	return strstarts(value, tfv->value);
}

bool td_tfcont(const char* value, const td_tfval* tfv) {
	return strstr(value, tfv->value) != NULL;
}

bool td_tfreg(const char* value, const td_tfval* tfv) {
	return (regexec(tfv->regexp, value, 0, NULL, 0) == 0);
}

bool strstarts(const char* buffer, const char* target) {
	while (*buffer && *target) {
		if (*buffer != *target) {
			return false;
		}
		
		buffer++;
		target++;
	}
	
	return true;
}

bool td_isnative(const td_line* line) {
	return line && strcmp(line->file, TD_FILE_NATIVE) == 0;
}
