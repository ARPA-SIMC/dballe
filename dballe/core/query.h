/*
 * dballe/core/query - Represent a filter for DB-All.e data
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
#ifndef DBALLE_CORE_QUERY_H
#define DBALLE_CORE_QUERY_H

#include <dballe/core/defs.h>
#include <dballe/core/record.h>
#include <wreport/varinfo.h>
#include <set>

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
/// Add minimum date, maximum date and data count details to summary query results
#define DBA_DB_MODIFIER_SUMMARY_DETAILS (1 << 8)

namespace dballe {

struct Query
{
    int ana_id = MISSING_INT;
    int prio_min = MISSING_INT;
    int prio_max = MISSING_INT;
    std::string rep_memo;
    int mobile = MISSING_INT;
    bool has_ident = false;
    std::string ident;
    Coords coords_min;
    Coords coords_max;
    Datetime datetime_min;
    Datetime datetime_max;
    Level level;
    Trange trange;
    std::set<wreport::Varcode> varcodes;
    std::string query;
    std::string ana_filter;
    std::string data_filter;
    std::string attr_filter;
    int limit = MISSING_INT;
    int block = MISSING_INT;
    int station = MISSING_INT;
    int data_id = MISSING_INT;
    bool query_station_vars = false;
    // DBA_KEY_VAR_RELATED	= 46,

    unsigned get_modifiers() const;

    void clear();

    void seti(dba_keyword key, int val);
    void setd(dba_keyword key, double val);
    void setc(dba_keyword key, const char* val);
    void setc(dba_keyword key, const std::string& val);

    void set(dba_keyword key, int val) { seti(key, val); }
    void set(dba_keyword key, double val) { setd(key, val); }
    void set(dba_keyword key, const char* val) { setc(key, val); }
    void set(dba_keyword key, const std::string& val) { setc(key, val); }

    void unset(dba_keyword key);

    /// Set the query values from the contents of a Record
    void set_from_record(const Record& rec);

    /**
     * Set a value in the record according to an assignment encoded in a string.
     *
     * String can use keywords, aliases and varcodes.  Examples: ana_id=3,
     * name=Bologna, B12012=32.4
     *
     * In case of numeric parameter, a hyphen ("-") means MISSING_INT (e.g.,
     * `leveltype2=-`).
     *
     * @param rec
     *   The record where the value is to be set.
     * @param str
     *   The string containing the assignment.
     * @return
     *   The error indicator for the function.
     */
    void set_from_string(const char* str);

    /// Same as set_from_string(str) but takes already split key and val
    void set_from_string(const char* key, const char* val);

    /// Same as setc, but parse val in the same way as Var::set_from_formatted
    void set_from_formatted(dba_keyword key, const char* val);

    /**
     * Return true if this query matches a subset of the given query.
     *
     * In other words, it returns true if this query is the same as \a other,
     * plus zero or more extra fields set, or zero or more ranges narrowed.
     */
    bool is_subquery(const Query& other) const;

    /// Print the query contents to stderr
    void print(FILE* out) const;

    /**
     * Parse the query=* modifiers specification inside the record, returning the
     * ORed flags
     */
    static unsigned parse_modifiers(const Record& rec);

    /**
     * Parse the modifiers specification given a query=* string, returning the ORed
     * flags.
     */
    static unsigned parse_modifiers(const char* str);
};

}

#endif
