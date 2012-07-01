/******************************************************************************
 * td_format.h - interface for the thread dump exchange format
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

#include "td.h"

// This type is used to represent the 'opinion' a visitor has.
typedef u_int8_t v_result;

/* BASE is the base-mask. This mask disables skipping and keeping.
 * That means, that V_BASE just parses any sub items and does not keep
 * any data in memory after.
 */
#define V_BASE 0

/* SKIP means to skip any sub items. 
 * This flag is just respected on in-visitors, as skipping on 
 * out-visitors is meaningless.
 */
#define V_SKIP 1

/* KEEP means to keep any data. 
 * If this flag is returned by an in-visitor the header information are
 * kept in memory.
 * If this flag is returned by an out-visitor the whole object, including
 * all sub items, is kept in memory.
 */
#define V_KEEP 2
/*
 * Example: V_BASE | V_KEEP
 * In this mode the parser has to keep the data in memory to parse
 * all sub items.
 * 
 * Example: V_BASE | V_SKIP
 * The parser may not keep any data in memory and has to skip all
 * sub items.
 * 
 * Example: V_BASE | V_KEEP | V_SKIP
 * The parser has to keep the header information in memory and has to 
 * skip all sub items.
 */

typedef v_result (*td_tinv)(void* userdata, const char* name, const char* native_id, const char* java_id, td_state state);
typedef v_result (*td_toutv)(void* userdata, td_thread* thread, v_result inResult);

typedef v_result (*td_dinv)(void* userdata, const char* id);
typedef v_result (*td_doutv)(void* userdata, td_dump* dump, v_result inResult);
	
typedef v_result (*td_lockv)(void* userdata, const char* class, const char* lockObjId, bool isLockOwner);
typedef v_result (*td_codev)(void* userdata, const char* file, const char* class, unsigned int line);
typedef v_result (*td_constrv)(void* userdata, const char* file, const char* class, unsigned int line);

struct dump_parser_state {
	// This pointer may be used for any parser implementation to store
	// implementation-dependant information.
	void* parser_data;
	
	// These pointers represent the current processed dump and thread. 
	// Currently I assume that each implementation may need such a state
	// object.
	td_dump* dump;
	td_thread* thread;	
	list* dumps;
	
	// These variables represent the visit results of the last in-/out-
	// visitor.
	v_result dump_result;
	v_result thread_result;
	
	// Here are a all possible visitors.
	td_tinv thread_inv;
	td_toutv thread_outv;

	td_dinv dump_inv;
	td_doutv dump_outv;
	
	td_lockv lockv;
	td_constrv constrv;
	td_codev codev;	
	
	void* userdata;
};
typedef struct dump_parser_state* dump_parser;

// Creates a new parser. Currently I don't know how to switch between
// possible implementations (shared libraries maybe?), so there's no
// special way to chose the implementation.
dump_parser td_init();
// Destroys the given parser.
void td_free_parser(dump_parser parser);

// Begins the parsing process using the given parser.
void td_parse(FILE* in, dump_parser parser);

// Sets the visitors used for thread visits.
void td_set_threadv(
	dump_parser parser,
	td_tinv in,
	td_toutv out
);
// Sets the visitors used for dump visits.
void td_set_dumpv(
	dump_parser parser,
	td_dinv in,
	td_doutv out
);
// Sets the visitors for stacktrace visits.
void td_set_linev(
	dump_parser parser,
	td_lockv lockv,
	td_codev codev,
	td_constrv constrv
);

void td_set_userdata(dump_parser parser, void* userdata);

// Return all parsed dumps stored in the given parser.
list* td_getdumps(dump_parser parser);

// Output
void td_print_dumpsstart();
void td_print_dumpsends();

void td_print_dump(const td_dump* dump);
void td_print_thread(td_thread* thread);

// atomar access for optimized output
void td_print_dumpstart(const char* id);
void td_print_dumpend();

void td_print_threadstart(const char* name, const char* native_id, const char* java_id, td_state state);
void td_print_threadend();

void td_print_code(const char* file, const char* class, unsigned int line);
void td_print_lock(const char* class, const char* lockObjId, bool isLockOwner);
void td_print_constr(const char* file, const char* class, unsigned int line);
