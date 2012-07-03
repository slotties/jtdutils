/******************************************************************************
 * td_xml.c - implementation of td_format.h using XML as format
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
#include <expat.h>
#include <string.h>

#include "td.h"
#include "td_format.h"
#include "list.h"

// XML Tags
#define XML_TAG_DUMP "dump"
#define XML_TAG_THREAD "thread"
#define XML_TAG_CODE "code"
#define XML_TAG_LOCK "lock"
#define XML_TAG_CONSTRUCTOR "constructor"
// XML Attributes
#define XML_ATTR_DUMP_ID "id"
#define XML_ATTR_THREAD_NAME "name"
// FIXME: rename to pid
#define XML_ATTR_THREAD_PID "native_id"
// FIXME: rename to jpid
#define XML_ATTR_THREAD_JPID "java_id"
#define XML_ATTR_THREAD_STATE "state"
#define XML_ATTR_CODE_CLASS "class"
#define XML_ATTR_CODE_FILE "file"
#define XML_ATTR_CODE_LINE "line"
#define XML_ATTR_LOCK_ID "id"
#define XML_ATTR_LOCK_CLASS "class"
#define XML_ATTR_LOCK_ISOWNER "isOwner"

// TODO: 8 kb buffer, maybe increase?
#define BUFFER_SIZE 8192

// FIXME: expat must have some way to define userdata which is passed 
//        to each callback
static dump_parser __D_PARSER = NULL;

void td_set_threadv(
	dump_parser parser,
	td_tinv in,
	td_toutv out
) {
	
	parser->thread_inv = in;
	parser->thread_outv = out;
}
void td_set_dumpv(
	dump_parser parser,
	td_dinv in,
	td_doutv out
) {
	
	parser->dump_inv = in;
	parser->dump_outv = out;	
}

void td_set_linev(
	dump_parser parser,
	td_lockv lockv,
	td_codev codev,
	td_constrv constrv
) {
	parser->lockv = lockv;
	parser->codev = codev;
	parser->constrv = constrv;
}

static void open_dump(const char** attributes) {
	const char* id = NULL;
	
	for (int i = 0; attributes[i] && !id; i += 2) {
		if (strcmp(attributes[i], XML_ATTR_DUMP_ID) == 0) {
			id = attributes[i + 1];
		}
	}

	if (__D_PARSER->dump_inv) {
		__D_PARSER->dump_result = __D_PARSER->dump_inv(__D_PARSER->userdata, id);
	}
	
	if ((__D_PARSER->dump_result & V_KEEP) == V_KEEP) {
		__D_PARSER->dump = (td_dump*) malloc(sizeof(td_dump));
		__D_PARSER->dump->threads = NULL;
		__D_PARSER->dump->id = strdup(id);
	}
}

static void open_thread(const char** attributes) {
	if ((__D_PARSER->dump_result & V_SKIP) == V_SKIP) {
		return;
	}
	
	const char* name = NULL;
	const char* native_id = NULL;
	const char* java_id = NULL;
	td_state state = UNKNOWN;
	
	for (int i = 0; attributes[i]; i += 2) {
		if (strcmp(attributes[i], XML_ATTR_THREAD_NAME) == 0) {
			name = attributes[i + 1];			
		} else if (strcmp(attributes[i], XML_ATTR_THREAD_STATE) == 0) {
			state = str2state(attributes[i + 1]);
		} else if (strcmp(attributes[i], XML_ATTR_THREAD_JPID) == 0) {
			java_id = attributes[i + 1];			
		} else if (strcmp(attributes[i], XML_ATTR_THREAD_PID) == 0) {
			native_id = attributes[i + 1];			
		}
	}

	if (__D_PARSER->thread_inv) {
		__D_PARSER->thread_result = __D_PARSER->thread_inv(__D_PARSER->userdata, name, native_id, java_id, state);
	}
	
	if ((__D_PARSER->thread_result & V_KEEP) == V_KEEP) {	
		__D_PARSER->thread = (td_thread*) malloc(sizeof(td_thread));
		__D_PARSER->thread->stacktrace = NULL;
		__D_PARSER->thread->state = state;
		__D_PARSER->thread->name = strdup(name);
		__D_PARSER->thread->native_id = strdup(native_id);
		__D_PARSER->thread->java_id = strdup(java_id);
	}
}

static void handle_stacktrace_elemenet(td_line_type type, const char** attributes) {
	if ((__D_PARSER->dump_result & V_SKIP) == V_SKIP || (__D_PARSER->thread_result & V_SKIP) == V_SKIP) {
		return;
	}
	
	unsigned int line = 0;
	const char* class = NULL;
	const char* file = NULL;
	const char* lockObjId = NULL;
	bool isLockOwner = false;
	
	for (int i = 0; attributes[i]; i += 2) {
		if (strcmp(attributes[i], XML_ATTR_CODE_CLASS) == 0) {
			class = attributes[i + 1];
		} else if (strcmp(attributes[i], XML_ATTR_CODE_FILE) == 0) {
			file = attributes[i + 1];
		} else if (strcmp(attributes[i], XML_ATTR_CODE_LINE) == 0) {
			line = atoi(attributes[i + 1]);
		} else if (strcmp(attributes[i], XML_ATTR_LOCK_ID) == 0) {
			lockObjId = attributes[i + 1];
		} else if (strcmp(attributes[i], XML_ATTR_LOCK_ISOWNER) == 0) {
			isLockOwner = (strcmp(attributes[i + 1], "1") == 0);
		}
	}
	
	v_result r = V_KEEP;
	switch (type) {
		case LOCK:
			if (__D_PARSER->lockv) {
				r = __D_PARSER->lockv(__D_PARSER->userdata, class, lockObjId, isLockOwner);
			}
			break;
		case CODE:
			if (__D_PARSER->codev) {
				r = __D_PARSER->codev(__D_PARSER->userdata, file, class, line);
			}
			break;
		case CONSTRUCTOR:
			if (__D_PARSER->constrv) {
				r = __D_PARSER->constrv(__D_PARSER->userdata, file, class, line);
			}
			break;
	}
	
	if (__D_PARSER->thread && (r & V_KEEP) == V_KEEP) {
		td_line* t_line = (td_line*) malloc(sizeof(td_line));
		t_line->type = type;
		t_line->line = line;
		t_line->class = class ? strdup(class) : NULL;
		if (file) {
			if (strcmp(file, TD_FILE_NATIVE) == 0) {
				t_line->file = TD_FILE_NATIVE;
			} else {
				t_line->file = strdup(file);
			}
		} else {
			t_line->file = NULL;
		}
		t_line->lockObjId = lockObjId ? strdup(lockObjId) : NULL;
		t_line->isLockOwner = isLockOwner;
		
		if (__D_PARSER->thread->stacktrace) {
			list_append(__D_PARSER->thread->stacktrace, t_line);
		} else {
			__D_PARSER->thread->stacktrace = list_create(t_line);
		}
	}
}

static void handle_start(void *data, const char *el, const char **attr) {
	if (strcmp(el, XML_TAG_DUMP) == 0) {
		open_dump(attr);
	} else if (strcmp(el, XML_TAG_THREAD) == 0) {
		open_thread(attr);
	} else if (strcmp(el, XML_TAG_CODE) == 0) {
		handle_stacktrace_elemenet(CODE, attr);
	} else if (strcmp(el, XML_TAG_LOCK) == 0) {
		handle_stacktrace_elemenet(LOCK, attr);
	} else if (strcmp(el, XML_TAG_CONSTRUCTOR) == 0) {
		handle_stacktrace_elemenet(CONSTRUCTOR, attr);
	}
}

static void close_thread() {
	if ((__D_PARSER->dump_result & V_SKIP) == V_SKIP) {
		return;
	}
	
	if (__D_PARSER->thread_outv) {
		__D_PARSER->thread_result = __D_PARSER->thread_outv(__D_PARSER->userdata, __D_PARSER->thread, __D_PARSER->thread_result);
	}
	
	if (__D_PARSER->thread) {
		if ((__D_PARSER->dump_result & V_KEEP) == V_KEEP && (__D_PARSER->thread_result & V_KEEP) == V_KEEP) {
			if (!__D_PARSER->dump->threads) {
				__D_PARSER->dump->threads = list_create(__D_PARSER->thread);
			} else {
				list_append(__D_PARSER->dump->threads, __D_PARSER->thread);
			}
			__D_PARSER->thread = NULL;
		} else {
			td_free_thread(__D_PARSER->thread);
			__D_PARSER->thread = NULL;
		}
	}
}

static void close_dump() {
	if (__D_PARSER->dump_outv) {
		__D_PARSER->dump_result = __D_PARSER->dump_outv(__D_PARSER->userdata, __D_PARSER->dump, __D_PARSER->dump_result);		
	}
	
	if (__D_PARSER->dump) {
		// this part can just be done if we have a dump
		if ((__D_PARSER->dump_result & V_KEEP) == V_KEEP) {
			// keep dump
			if (!__D_PARSER->dumps) {
				__D_PARSER->dumps = list_create(__D_PARSER->dump);
			} else {
				list_append(__D_PARSER->dumps, __D_PARSER->dump);
			}
			__D_PARSER->dump = NULL;
		} else {
			// free it
			td_free_dump(__D_PARSER->dump);
			__D_PARSER->dump = NULL;
		}
	}
}

static void handle_end(void *data, const char *el) {
	if (strcmp(el, XML_TAG_THREAD) == 0) {
		close_thread();
	} else if (strcmp(el, XML_TAG_DUMP) == 0) {
		close_dump();
	}
}

dump_parser td_init() {
	dump_parser parser = (dump_parser) malloc(sizeof(struct dump_parser_state));
	// nullify visitors
	parser->thread_inv = NULL;
	parser->thread_outv = NULL;
	parser->dump_inv = NULL;
	parser->dump_outv = NULL;
	parser->lockv = NULL;
	parser->codev = NULL;
	parser->constrv = NULL;
	// nullify states
	parser->dump = NULL;
	parser->dumps = NULL;
	parser->thread = NULL;
	parser->userdata = NULL;
	
	// default is to keep and not to skip anything
	parser->dump_result = V_KEEP;
	parser->thread_result = V_KEEP;
	
	parser->parser_data = XML_ParserCreate(NULL);
	XML_SetElementHandler((XML_Parser) parser->parser_data, handle_start, handle_end);
	
	return parser;
}

void td_free_parser(dump_parser parser) {
	XML_ParserFree((XML_Parser) parser->parser_data);
	list_free(parser->dumps, td_free_dump);
	free(parser);
}
	
void td_parse(FILE* in, dump_parser parser) {
	__D_PARSER = parser;
	
	char buffer[BUFFER_SIZE];
	int done = 0;
	const XML_Parser xml_parser = (XML_Parser) parser->parser_data;
	
	while (!done) {
		int len = fread(buffer, 1, BUFFER_SIZE, in);
		done = feof(in);
		if (!XML_Parse(xml_parser, buffer, len, done)) {
			printf("ERROR @ line %ld: %s\n", 
				XML_GetCurrentLineNumber(xml_parser), 
				XML_ErrorString(XML_GetErrorCode(xml_parser)));
		}
	}
	
	// cleanup state
	__D_PARSER = NULL;
}

list* td_getdumps(dump_parser parser) {
	return parser->dumps;
}

void td_print_dumpsstart() {
	printf("<?xml version=\"1.0\"?>\n");
	printf("<dumps>\n");
}
void td_print_dumpsends() {
	printf("</dumps>\n");
}

void td_print_code(const char* file, const char* class, unsigned int line) {
	printf("<code class=\"%s\" file=\"%s\" line=\"%d\"/>\n", 
		class,
		file,
		line
	);
}

void td_print_lock(const char* class, const char* lockObjId, bool isLockOwner) {
	printf("<lock id=\"%s\" class=\"%s\" isOwner=\"%d\"/>\n", 
		lockObjId, 
		class, 
		(isLockOwner ? 1 : 0)
	);
}

void td_print_constr(const char* file, const char* class, unsigned int line) {
	printf("<constructor class=\"%s\" file=\"%s\" line=\"%d\"/>\n", 
		class,
		file,
		line
	);
}

void td_print_thread(td_thread* thread) {
	td_print_threadstart(thread->name, thread->native_id, thread->java_id, thread->state);
	
	if (thread->stacktrace) {
		for (list* i = thread->stacktrace; i; i = i->next) {
			td_line* line = (td_line*) i->data;
			switch (line->type) {
				case CODE:
					td_print_code(line->file, line->class, line->line);
					break;
				case LOCK:
					td_print_lock(line->class, line->lockObjId, line->isLockOwner);
					break;
				case CONSTRUCTOR:
					td_print_constr(line->file, line->class, line->line);
					break;
			}
		}
	}
	
	td_print_threadend();
}

void td_print_dump(const td_dump* dump) {
	td_print_dumpstart(dump->id);
	
	for (list* i = dump->threads; i; i = i->next) {
		td_print_thread((td_thread*) i->data);
	}
	
	td_print_dumpend();
}

void td_print_dumpstart(const char* id) {
	printf("<dump id=\"%s\">\n", id);
	
}
void td_print_dumpend() {
	printf("</dump>\n");
}

void td_print_threadstart(const char* name, const char* native_id, const char* java_id, const td_state state) {
	printf("<thread name=\"%s\" state=\"%s\" native_id=\"%s\" java_id=\"%s\">\n",
		name,
		state2str(state),
		native_id,
		java_id
	);
}

void td_print_threadend() {
	printf("</thread>\n");
}

void td_set_userdata(dump_parser parser, void* userdata) {
	parser->userdata = userdata;	
}
