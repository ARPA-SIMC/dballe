/*
 * dballe/db/defs - Base definitions for dballe DB implementations
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_DEFS_H
#define DBALLE_DB_DEFS_H

#include <wreport/error.h>

/**
 * Flags controlling message import
 * @{
 */
/* Import the attributes. */
#define DBA_IMPORT_ATTRS		1
/* Attempt to merge pseudoana extra information into the existing ones. */
#define DBA_IMPORT_FULL_PSEUDOANA	2
/* Message data will overwrite existing values; otherwise, trying to insert
 * existing data will cause an error. */
#define DBA_IMPORT_OVERWRITE		8
/// @}

namespace dballe {
namespace db {

/**
 * Supported formats
 */
typedef enum {
    V5 = 0,
    V6 = 1,
    MEM = 2,
    MESSAGES = 3,
} Format;

/// Base exception for database errors
struct error : public wreport::error {};

}
}
#endif
