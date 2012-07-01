/******************************************************************************
 * tdstrip - a small tool for extracting java thread dumps from log files 
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
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>

#include "common.h"
#include "tdstrip.h"

static void print_help();

int main(int argc, char** argv) {	
	e_extr (*parser_fn)(FILE* in) = extract_sun;
	
	int idx = 0;
	struct option options[] = {
		{ "type", required_argument, 0, 't' },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' }
	};
	
	for (int c = 0; c >= 0; ) {
		c = getopt_long(argc, argv, ":hvt:", options, &idx);
		switch (c) {
			case 't':
				if (strcmp(optarg, "ibm") == 0) {
					parser_fn = extract_ibm;
				} else if (strcmp(optarg, "sun") == 0) {
					parser_fn = extract_sun;
				} else {
					fprintf(stderr, "Unknown parser type %s.\n", optarg);
					print_help();
					return EXIT_FAILURE;
				}
				
				break;
			case 'v':
				PRINT_VERSION("tdstrip");
				return EXIT_SUCCESS;
			case 'h': 
				print_help();
				return EXIT_SUCCESS;
			case '?':
				print_help();
				return EXIT_FAILURE;
		}
	}

	FILE* in = NULL;
	if (optind < argc) {
		in = fopen(argv[optind], "r");
		if (!in) {
			fprintf(stderr, "Could not open %s: %s.\n", argv[optind], strerror(errno));
			return EXIT_FAILURE;
		}
	} else {
		in = stdin;
	}
	
	e_extr result = parser_fn(in);
	fclose(in);
	
	switch (result) {
		case SUCCESS:
			return EXIT_SUCCESS;
		case ILLEGAL_FORMAT:
			fprintf(stderr, "Illegal format.\n");
			return EXIT_FAILURE;
		case UNEXPECTED_EOF:
			fprintf(stderr, "Unexpected EOF.\n");
			return EXIT_FAILURE;
		default: 
			fprintf(stderr, "Unknown error.\n");
			return EXIT_FAILURE;
	}
}

static void print_help() {
	printf("Usage: tdstrip [-t type] [-v|-h|LOGFILE]\n\
LOGFILE can either be a text file containing thread dumps\n\
or be omitted to use standard input.\n\
OPTION can be one of:\n\
  -t t, --type t               Selects t as parser. t can either be sun or ibm.\n\
  -h, --help                   Prints this help.\n\
  -v, --version                Prints the version information.\n");
}
