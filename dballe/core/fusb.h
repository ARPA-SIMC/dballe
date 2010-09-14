/*
 * core/fusb - Read and write Fortran Unformatted Sequential Binary records
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

#ifndef DBALLE_CORE_FUSB_H
#define DBALLE_CORE_FUSB_H

#include <string>

namespace dballe {
struct File;

/**
 * Read a Fortran Unformatted Sequential Binary record from a file
 *
 * @return true if a record was found, false on EOF
 */
bool fusb_read(File& file, std::string& res);

/**
 * Write a Fortran Unformatted Sequential Binary record to a file
 */
void fusb_write(File& file, const std::string& res);

} // namespace dballe

#endif
/* vim:set ts=4 sw=4: */
