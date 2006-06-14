/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBA_CSV_H
#define DBA_CSV_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Routines to parse data in CSV format
 */

#include <stdio.h>

/**
 * Parse a CSV line.
 *
 * @param in
 *   The file where to read from
 * @param cols
 *   The array of char* that will hold the parsed columns.  If the line has
 *   more than `cols' columns, the exceeding ones will be ignored.
 *   Please note that you have to deallocate all the lines returned in cols.
 * @param col_max
 *   The size of cols
 * @return
 *   The number of columns found, or 0 if we hit the end of the file
 */
int dba_csv_read_next(FILE* in, char** cols, int col_max);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
