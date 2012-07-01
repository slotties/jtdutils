/******************************************************************************
 * tdstrip_ibm.c - parsing code for IBM v9 JVM's
 *
 * Copyright (C) 2010 Stefan Lotties <slotties@gmail.com>
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
 * 
 * SEE
 * http://publib.boulder.ibm.com/infocenter/javasdk/v1r4m2/topic/com.ibm.java.doc.diagnostics.142/html/interpretjavadump.html#interpretjavadump
 * ***************************************************************************/

#include <string.h>
#include <stdlib.h>

#include "td.h"
#include "td_format.h"
#include "tdstrip.h"
#include "list.h"

#define BUF_SIZE 512
char BUFFER[BUF_SIZE];

static void strcrepl(char* str, char x, char y) {
	while (*str) {
		if (*str == x) {
			*str = y;
		}
		
		str++;
	}
}

/*
 * Samples:
 * 4XESTACKTRACE          at com/ibm/misc/SignalDispatcher.waitForSignal(Native Method)
 * 4XESTACKTRACE          at com/sap/engine/boot/Start.main(Start.java:34)
 * 
 * Special things to do:
 * - replace / with . in full qualified class name
 */
static void ibm_print_stacktrace(char* buffer) {
	static char file[128];
	static char class[1024];
	static int line;
	
	/* Unfortunately locks are in a different section than stacktraces
	 * and we don't have the information of *where* the lock is placed
	 * inside the stacktrace, therefore we currently won't parse and 
	 * mixin locks.
	 * 
	 * The next problem is that I currently don't know how to differ
	 * code lines from constructors.
	 */
	// FIXME: mixin locks
	// FIXME: identify constructors
	
	char* working = strstr(buffer, "at ") + 3;
	int n = strchr(working, '(') - working;
	
	strncpy(class, working, n);
	class[n] = '\0';	
	strcrepl(class, '/', '.');
	
	working += n + 1;

	char* delim = strchr(working, ':');
	if (delim) {
		strncpy(file, working, delim - working);
		file[delim - working] = '\0';
		
		// skip : = +1
		line = atoi(delim + 1);
	} else {
		// native
		strcpy(file, "NATIVE");
		line = -1;
	}
	
	td_print_code(file, class, line);
}

/*
 * A thread header looks like:
 * 3XMTHREADINFO      "main" (TID:0x0000000000FDCF00, sys_thread_t:0x0000000000FC4FF0, state:CW, native ID:0x0000000000003CA8) prio=5
 */
static void ibm_td_print_thread(char* buffer) {
	static char name[128];
	static char java_id[128];
	static char native_id[128];
	
	int n;
	char* tmp = strstr(buffer, "\"") + 1;
	
	n = strstr(tmp, "\"") - tmp;
	strncpy(name, tmp, n);
	name[n] = '\0';
	
	// TID: = +4
	tmp = strstr(tmp + n, "TID:") + 4;
	n = strstr(tmp, ",") - tmp;
	strncpy(java_id, tmp, n);
	java_id[n] = '\0';
	
	// state
	td_state state;
	tmp = strstr(tmp + n, "state:");
	if (strstarts(tmp, "state:R")) {
		state = RUNNING;
	} else if (strstarts(tmp, "state:CW")) {
		/* snipped from the docs:
		 * The values of state can be:
		 * * R - Runnable - the thread is able to run when given the chance.
		 * * CW - Condition Wait - the thread is waiting. For example, because:
		 *   * A sleep() call is made.
		 *   * The thread has been blocked for I/O.
		 *   * A synchronized method of an object locked by another thread has been called.
		 *   * The thread is synchronizing with another thread with a join() call.
		 * 
		 * Currently we don't have the chance to difference the waiting states,
		 * so we just mark it as WAITING_ON_CONDITION.
		 */
		state = WAITING_ON_CONDITION;
	} else {
		// FIXME: I've seen state:B in a javacore file, but no clue what exactly that means...will be fixed later
		state = UNKNOWN;
	}
	
	// native ID: = +10
	tmp = strstr(tmp, "native ID:") + 10;
	n = strstr(tmp, ")") - tmp;
	strncpy(native_id, tmp, n);
	native_id[n] = '\0';	
	
	td_print_threadstart(name, native_id, java_id, state);
}

e_extr extract_ibm(FILE* in) {	
	bool threadOpened = false;
	bool dumpOpened = false;
	
	char dumpId[32];
	
	td_print_dumpsstart();
	while (fgets(BUFFER, BUF_SIZE, in) != NULL) {
		if (strstarts(BUFFER, "1TIDATETIME")) {
			// the date field has an own tag
			char* start = strstr(BUFFER, "20");
			int len = strlen(start) - 1;
			strncpy(dumpId, start, len);
			dumpId[len] = '\0';
			
		} else if (strstarts(BUFFER, "2XMFULLTHDDUMP")) {
			// dump *really* starts now			
			td_print_dumpstart(dumpId);
			dumpOpened = true;
			
		} else if (dumpOpened && strstarts(BUFFER, "3XMTHREADINFO")) {
			// print thread header
			if (threadOpened) {
				// close current thread
				td_print_threadend();
			}
			
			ibm_td_print_thread(BUFFER);
			
			threadOpened = true;
		} else if (threadOpened && strstarts(BUFFER, "4XESTACKTRACE")) {
			// print stacktrace
			ibm_print_stacktrace(BUFFER);
			
		} else if (dumpOpened && strstarts(BUFFER, "NULL")) {
			// dumps ends here
			if (threadOpened) {
				td_print_threadend();
				threadOpened = false;
			}
			
			td_print_dumpend();
			dumpOpened = false;
		}
	}
	
	// should never happen, because the javacore files are very
	// nice to parse, but we better ensure the XML is valid...
	if (dumpOpened) {
		if (threadOpened) {
			td_print_threadend();
		}
		
		td_print_dumpend();
	}
	
	td_print_dumpsends();

	return SUCCESS;
}

