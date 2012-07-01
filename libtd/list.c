/******************************************************************************
 * list.c - implementation of list.h
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
#include <stdbool.h>

#include "list.h"

int list_len(list* head) {
	int len = 0;

	while(head) {
		len++;
		head = head->next;
	}

	return len;
}

list* list_create(void* data) {
	list* l = (list*) malloc(sizeof(list));
	if (l) {
		l->data = data;
		l->next = NULL;
	} else {
		free(l);
		l = NULL;
	}

	return l;
}

list* list_append(list* l, void* data) {
	if (l) {
		list* next;

		while (l->next) {
			l = l->next;
		}

		next = list_create(data);
		l->next = next;

		return next;
	} else {
		return NULL;
	}
}

void* list_find(list* l, list_matcher matcher, const void* fnArg) {
	list* node = list_find_node(l, matcher, fnArg);
	if (node) {
		return node->data;
	} else {
		return NULL;
	}
}

list* list_find_node(list* l, list_matcher matcher, const void* fnArg) {
	while (l) {
		if (matcher(l->data, fnArg)) {
			return l;
		}

		l = l->next;
	}

	return NULL;
}

void list_free(list* l, void (*free_fn)(void*)) {
	list* next;
	
	while (l) {
		next = l->next;

		if (free_fn) {
			free_fn(l->data);
		}
		free(l);

		l = next;		
	}
}

void list_delete(list** l, list* node, void (*free_fn)(void*)) {
	if (*l == node) {
		// special handling for first node
		*l = node->next;
		if (free_fn) {
			free_fn(node->data);
		}
		free(node);
	} else {	
		list* prev;
		for(prev = *l; prev != NULL && prev->next != node; prev = prev->next);
		
		if (prev) {
			prev->next = node->next;
			if (free_fn) {
				free_fn(node->data);
			}
			free(node);
		} // otherwise it's not found
	}
}

void list_insert_by(list** l, void* data, list_comparator cmp) {
	list* r = *l;
	list* prev = NULL;
	
	list* newNode = list_create(data);
		
	while (r != NULL) {
		if (cmp(data, r->data) < 0) {
			// insert before
			if (prev == NULL) {
				// special handling for the head
				newNode->next = *l;
				*l = newNode;
			} else {
				newNode->next = prev->next;
				prev->next = newNode;
			}
			
			// done
			return;
		}
		// otherwise we just go on with the search...
		prev = r;
		r = r->next;
	}
	
	// append
	prev->next = newNode;
}

list* list_get_pnode(list* l, list* curr) {
	list* c = l;
	
	if (c == curr) {
		return NULL;
	}
	
	while (c && c->next != curr) {
		c = c->next;
	}
	
	return c;
}

int list_getidx(list* head, list* node) {
	int i;
	for (i = 0; head != node; head = head->next) {
		i++;
	}
	
	return i;
}
