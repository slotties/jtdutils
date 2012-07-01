/******************************************************************************
 * conf.h - the definition of a very minimalistic config parser
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

#ifndef CONFIG_H
#define CONFIG_H

struct config_file {
	void* state;
};
typedef struct config_file conf;

conf* conf_create(char* default_file, char* user_file);
void conf_free(conf* file);

const char* conf_get(conf* c, const char* name);
const char* conf_get_def(conf* c, const char* name, const char* default_value);

int conf_geti_def(conf* c, const char* name, int default_value);

float conf_getf_def(conf* c, const char* name, float default_value);

#endif
