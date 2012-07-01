/******************************************************************************
 * tdprobe
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
 * ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>

#include <sys/sysinfo.h>

#include <dlfcn.h>

#include <pthread.h>

#include "common.h"
#include "conf.h"
#include "list.h"
#include "td.h"
#include "td_format.h"
#include "probemodule.h"

typedef struct {
	module_t* module;
	void* dlhandle;
} module_handle_t;

typedef struct {
	module_handle_t* module;
	char* comment;
	presult result;
} module_result_t;

typedef struct {
	td_dump* dump;
	list* results;
} dump_results;

typedef struct {
	list** tests;
	conf* config;
	list* dumps;
	list* results;
}  worker_data;

static void print_results(list* results);
static void print_help();

static int get_no_workers(conf* c);

static list* create_results(list* dumps);
static list* init_tests(conf* config, list* results);
static void runTests(list* tests, list* dumps, conf* c, list* results);

static void free_module_handle_t(module_handle_t* handle);
static void free_module_result_t(module_result_t* result);
static void td_free_dump_results(dump_results* results);

pthread_mutex_t test_mutex;
pthread_mutex_t result_mutex;

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
				PRINT_VERSION("tdprobe");
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
	
	dump_parser parser = td_init();
	td_parse(in, parser);
	
	// FIXME: use /etc/jtdutils/tdprobe.conf, ~/.jtdutils/tdprobe.conf
	conf* c = conf_create("tdprobe/tdprobe.conf", NULL);
	
	list* dumps = td_getdumps(parser);
	list* results = create_results(dumps);
	list* tests = init_tests(c, results);

	runTests(tests, dumps, c, results);
		
	print_results(results);

	td_free_parser(parser);

	fclose(in);
	
	conf_free(c);
	list_free(tests, free_module_handle_t);
	list_free(results, td_free_dump_results);
	
	return EXIT_SUCCESS;
}

static void print_help() {
	printf("\
Usage: tdprobe [FILE]\n\
FILE may either be a file containing the output of tdstrip or standard input.\n\
\n\
General options:\n\
  -h, --help                   Prints this help.\n\
  -v, --version                Prints the version information.\n\
\n");
}

static list* create_results(list* dumps) {
	list* results = NULL;
	
	for (list* d = dumps; d; d = d->next) {
		dump_results* r = malloc(sizeof(dump_results));
		r->dump = (td_dump*) d->data;
		r->results = NULL;
		
		if (results) {
			list_append(results, r);
		} else {
			results = list_create(r);
		}
	}
	dump_results* r = malloc(sizeof(dump_results));
	r->dump = NULL;
	r->results = NULL;
	if (results) {
		list_append(results, r);
	} else {
		results = list_create(r);
	}
	
	return results;
}

static int get_no_workers(conf* c) {
	const char* workersStr = conf_get(c, "workersPerProcessor");
	float relativeWorkers = workersStr ? atof(workersStr) : 1.0f;
	int cores = get_nprocs();
	
	int workers = cores / relativeWorkers;
	if (workers < 1) {
		workers = 1;
	}
	
	return workers;
}

static module_handle_t* pop_module(list** modules) {
	module_handle_t* next = NULL;
	
	pthread_mutex_lock(&test_mutex);
	if (*modules) {
		next = (*modules)->data;
		list_delete(modules, *modules, NULL);
	}
	pthread_mutex_unlock(&test_mutex);
	
	return next;
}

static bool dump_results_match(dump_results* r, td_dump* target) {
	return r->dump == target;
}

static void add_result(list* results, td_dump* dump, presult result, char* comment, module_handle_t* module) {
	dump_results* r = list_find(results, dump_results_match, dump);
	module_result_t* m = malloc(sizeof(module_result_t));
	m->comment = comment;
	m->result = result;
	m->module = module;
	
	pthread_mutex_lock(&(result_mutex));
	if (r->results) {
		list_append(r->results, m);
	} else {
		r->results = list_create(m);
	}
	pthread_mutex_unlock(&(result_mutex));
}

static void invokeProbe(module_handle_t* module, list* dumps, conf* config, list* results) {
	presult (*fn)() = dlsym(module->dlhandle, "probe");
	while (dumps) {
		char* comment = malloc(sizeof(char) * 512);
		comment[0] = '\0';
		
		int r = fn(dumps->data, config, comment);
		add_result(results, (td_dump*) dumps->data, r, comment, module);
		
		dumps = dumps->next;
	}
}

static void testWorker(worker_data* data) {	
	module_handle_t* next = NULL;
	while ((next = pop_module(data->tests)) != NULL) {
		invokeProbe(next, data->dumps, data->config, data->results);
	}
}

static void runTests(list* tests, list* dumps, conf* c, list* results) {
	int effectiveWorkers = get_no_workers(c);
	pthread_t threads[effectiveWorkers];
	
	// FIXME: pass tests directly
	list* test_queue = NULL;
	for (list* l = tests; l; l = l->next) {
		if (test_queue) {
			list_append(test_queue, l->data);
		} else {
			test_queue = list_create(l->data);
		}
	}
	
	worker_data data = { &test_queue, c, dumps, results };		
	for (int i = 0; i < effectiveWorkers; i++) {
		pthread_create(&(threads[i]), NULL, testWorker, &data);
	}
	
	for (int i = 0; i < effectiveWorkers; i++) {
		pthread_join(threads[i], NULL);
	}
}

static void print_results(list* results) {	
	for (list* i = results; i; i = i->next) {
		dump_results* r = i->data;
		if (r->dump) {
			printf("\nResults for %s:\n", r->dump->id);
		} else {
			printf("\nResults of dump-spanning tests:\n");
		}

		if (!r->results) {
			printf("No tests ran.\n");
			return;
		}

		for (list* l = r->results; l; l = l->next) {
			module_result_t* tr = l->data;
			char* resultStr;
			if (tr->result == R_OK) {
				resultStr = "OK";
			} else if (tr->result == R_SUSPICIOUS) {
				resultStr = "SUSP";
			} else {
				resultStr = "BAD";
			}
			
			printf("Test: %-65s [%s]\n", tr->module->module->name, resultStr);
			if (strlen(tr->comment) > 0) {
				printf("Note: %s\n", tr->comment);
			}
		}
	}
}

static list* init_tests(conf* config, list* results) {
	char* files = conf_get(config, "modules");
	
	list* modules = NULL;
	for (char* file = strtok(files, ","); file; file = strtok(NULL, ",")) {
		if (strlen(file) > 0) {
			void* handle = dlopen(file, RTLD_NOW);
			module_t* (*get_module)();
			if (!handle) {
				fprintf(stderr, "Could not load %s: %s\n", file, dlerror());
			} else if (!(get_module = dlsym(handle, "get_module"))) {
				dlerror();
				fprintf(stderr, "%s does not implement get_module().\n", file);
			} else if (!dlsym(handle, "probe")) {
				dlerror();
				fprintf(stderr, "%s does not implement probe().\n", file);
			} else {
				dlerror();
				
				module_t* nfo = get_module();
				if (nfo) {
					printf("Loaded %s.\n", file);
					module_handle_t* mh = malloc(sizeof(module_handle_t));
					mh->module = nfo;
					mh->dlhandle = handle;
					
					if (!modules) {
						modules = list_create(mh);
					} else {
						list_append(modules, mh);
					}	
				} else {
					fprintf(stderr, "%s does not return a module_t* from get_module.\n", file);
				}
			}
		}
	}
	
	return modules;
}

static void free_module_result_t(module_result_t* result) {
	free(result->comment);
	free(result);
}

static void free_module_handle_t(module_handle_t* handle) {
	dlclose(handle->dlhandle);
	free(handle);
}

static void td_free_dump_results(dump_results* results) {
	if (results->results) {
		list_free(results->results, free_module_result_t);
	}
	free(results);
}
