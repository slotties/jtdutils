/******************************************************************************
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
#include <string.h>
#include <regex.h>

#include "list.h"
#include "conf.h"

typedef struct {
	char* name;
	char* value;
} conf_entry;

typedef struct {
	list* user;
	list* def;
} conf_state;

static list* read_config(char* file, regex_t* entry_regex) {
	if (!file) {
		return NULL;
	}
	
	FILE* in = fopen(file, "r");
	if (!in) {
		return NULL;
	}
	
	regmatch_t m[3];
	list* entries = NULL;
	
	char buffer[512];
	while (fgets(buffer, 512, in) != NULL) {
		if (buffer[0] != '#' && regexec(entry_regex, buffer, 3, m, 0) == 0) {
			char* name = strndup(buffer + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
			
			// -1 because we don't want the line break
			char* value = strndup(buffer + m[2].rm_so, m[2].rm_eo - m[2].rm_so - 1);
			
			conf_entry* e = (conf_entry*) malloc(sizeof(conf_entry));
			e->name = name;
			e->value = value;
			
			if (!entries) {
				entries = list_create(e);
			} else {
				list_append(entries, e);
			}
		}
	}
	
	fclose(in);
	
	return entries;
}

conf* conf_create(char* default_file, char* user_file) {
	regex_t* regex = malloc(sizeof(regex_t));
	int comp_r = regcomp(regex, "([a-zA-Z\\._-]*)=(.*)", REG_EXTENDED);
	if (comp_r != 0) {
		char err[512];
		regerror(comp_r, regex, err, 512);
		fprintf(stderr, "Your regex library is broken: %s\n", err);
		return NULL;
	}
	
	conf_state* state = (conf_state*) malloc(sizeof(conf_state));
	state->def = read_config(default_file, regex);
	state->user = read_config(user_file, regex);
	
	regfree(regex);
	free(regex);
	
	if (!state->def && !state->user) {
		fprintf(stderr, "No config files found.\n");
		free(state);		
		return NULL;
	}
	
	conf* f = (conf*) malloc(sizeof(conf));
	f->state = state;
	
	return f;
}

static void conf_entry_free(conf_entry* e) {
	free(e->name);
	free(e->value);
	free(e);
}

void conf_free(conf* file) {
	conf_state* c = file->state;
	if (c->user) {
		list_free(c->user, conf_entry_free);
	}
	if (c->def) {
		list_free(c->def, conf_entry_free);
	}
	
	free(c);
	free(file);
}

static bool find_entry(const void* curr, const void* target) {
	return (strcmp(((conf_entry*) curr)->name, target) == 0);
}

const char* conf_get(conf* cfg, const char* name) {	
	conf_state* c = cfg->state;
	conf_entry* value = NULL;
	if (c->user) {
		value = list_find(c->user, find_entry, name);
	}
	if (!value && c->def) {
		value = list_find(c->def, find_entry, name);
	}
		
	return value ? value->value : NULL;
}

const char* conf_get_def(conf* c, const char* name, const char* default_value) {
	const char* v = conf_get(c, name);
	return v ? v : default_value;
}

int conf_geti(conf* c, const char* name) {
	const char* v = conf_get(c, name);
	return v ? atoi(v) : 0;
}

int conf_geti_def(conf* c, const char* name, int default_value) {
	const char* v = conf_get(c, name);
	return v ? atoi(v) : default_value;
}

float conf_getf_def(conf* c, const char* name, float default_value) {
	const char* v = conf_get(c, name);
	return v ? atof(v) : default_value;
}
