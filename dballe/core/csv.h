/*
 * dballe/csv - CSV reading functions
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/** @file
 * @ingroup core
 * Routines to parse data in CSV format
 */

#include <vector>
#include <string>
#include <stdio.h>

namespace dballe
{

/**
 * Parse a CSV line.
 *
 * @param in
 *   The file where to read from
 * @param cols
 *   The parsed columns.
 * @return
 *   true if a new line was found, else false
 */
bool csv_read_next(FILE* in, std::vector<std::string>& cols);

}

/* vim:set ts=4 sw=4: */
#endif
