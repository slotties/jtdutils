/******************************************************************************
 * common.h - common constants for all jtdutils
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

#ifndef COMMON_H
#define COMMON_H

// the jtdutils version
#define VERSION "0.6"

// generic for printing the version
#define PRINT_VERSION(tool) printf(\
"%s, version %s\n\
written by Stefan Lotties <slotties@gmail.com>\n",\
tool, VERSION);

#endif
