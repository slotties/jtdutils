/******************************************************************************
 * tdstrip.h - interface for all tdstrip modules
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

typedef enum enum_extraction_result { 
	// no problems
	SUCCESS, 
	// The dump has not the expected format
	ILLEGAL_FORMAT, 
	// Unexpected EOF from stdin/log file
	UNEXPECTED_EOF,  
	// Memory could not be allocated
	MEMORY_ERROR 
} e_extr;

// FIXME: replace extract_sun and extract_ibm by some shared object 
// 		  loading mechanism.

// Extraction function for SUN JVM 1.4, 1.5 and 1.6
e_extr extract_sun(FILE* in);

// IBM v9
e_extr extract_ibm(FILE* in);
