/******************************************************************************
 * list.h - interface for dealing with lists
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

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

#include "common.h"

typedef struct generic_list {
	void* data;
	struct generic_list* next;
} list;

typedef bool (*list_matcher)(const void*, const void*);
typedef int (*list_comparator)(const void*, const void*);

int list_len(list* head);

list* list_create(void* data);
list* list_append(list* l, void* data);

void* list_find(list* l, list_matcher matcher, const void* fnArg);

list* list_find_node(list* l, list_matcher matcher, const void* fnArg);

void list_free(list*, void (*free_fn)(void*));
void list_delete(list** l, list* node, void (*free_fn)(void*));

// FIXME: add fnArg for the comparator
void list_insert_by(list** l, void* data, list_comparator cmp);

list* list_get_pnode(list* l, list* curr);
int list_getidx(list* head, list* node);

#endif
