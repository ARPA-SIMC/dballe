/*
 * dballe/db/modifiers - Parse query=* modifiers
 *
 * Copyright (C) 2007--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_MODIFIERS_H
#define DBA_DB_MODIFIERS_H

/**
 * Values for query modifier flags
 */
/** When values from different reports exist on the same point, only report the
 * one from the report with the highest priority */
#define DBA_DB_MODIFIER_BEST        (1 << 0)
/** Tell the database optimizer that this is a query on a database with a big
 * pseudoana table (this serves to hint the MySQL optimizer, which would not
 * otherwise apply the correct strategy */
#define DBA_DB_MODIFIER_BIGANA      (1 << 1)
/** Remove duplicates in the results */
#define DBA_DB_MODIFIER_DISTINCT    (1 << 2)
/** Include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_ANAEXTRA    (1 << 3)
/** Do not include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_NOANAEXTRA  (1 << 4)
/** Do not bother sorting the results */
#define DBA_DB_MODIFIER_UNSORTED    (1 << 5)
/** Start geting the results as soon as they are available, without waiting for
 * the database to finish building the result set.  As a side effect, it is
 * impossible to know in advance the number of results.  Currently, it does not
 * work with the MySQL ODBC driver */
#define DBA_DB_MODIFIER_STREAM      (1 << 6)
/** Sort by report after ana_id, to ease reconstructing messages on export */
#define DBA_DB_MODIFIER_SORT_FOR_EXPORT (1 << 7)

namespace dballe {
struct Record;

namespace db {

/**
 * Parse the query=* modifiers specification inside the record, returning the
 * ORed flags
 */
unsigned parse_modifiers(const Record& rec);

}
}

#endif
