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
/** Remove duplicates in the results */
#define DBA_DB_MODIFIER_ANAEXTRA    (1 << 3)
/** Do not bother sorting the results */
#define DBA_DB_MODIFIER_UNSORTED    (1 << 5)
/** Sort by report after ana_id, to ease reconstructing messages on export */
#define DBA_DB_MODIFIER_SORT_FOR_EXPORT (1 << 7)
/// Add minimum date, maximum date and data count details to summary query results
#define DBA_DB_MODIFIER_SUMMARY_DETAILS (1 << 8)

namespace dballe {

struct JSONWriter;

struct Query
{
    static const uint32_t WANT_MISSING_IDENT =  (1 << 0);
    static const uint32_t WANT_MISSING_LTYPE1 = (1 << 1);
    static const uint32_t WANT_MISSING_L1 =     (1 << 2);
    static const uint32_t WANT_MISSING_LTYPE2 = (1 << 3);
    static const uint32_t WANT_MISSING_L2 =     (1 << 4);
    static const uint32_t WANT_MISSING_PIND =   (1 << 5);
    static const uint32_t WANT_MISSING_P1 =     (1 << 6);
    static const uint32_t WANT_MISSING_P2 =     (1 << 7);

    /**
     * Set a bit a 1 with WANT_MISSING_* constants to specify that the query
     * wants results in which the corresponding field is set to a missing
     * value.
     */
    uint32_t want_missing = 0;
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
     * Set a record from a ", "-separated string of assignments.
     *
     * The implementation is not efficient and the method is not safe for any
     * input, since ", " could appear in a station identifier. It is however
     * useful to quickly create test queries for unit testing.
     */
    void set_from_test_string(const std::string& s);

    /**
     * Return true if this query matches a subset of the given query.
     *
     * In other words, it returns true if this query is the same as \a other,
     * plus zero or more extra fields set, or zero or more ranges narrowed.
     */
    bool is_subquery(const Query& other) const;

    /**
     * Generate a sequence of dba_keyword and unique_ptr<Var> for all contents
     * of the query that can be represented in a record.
     */
    void to_vars(std::function<void(dba_keyword, std::unique_ptr<wreport::Var>)> dest) const;

    /// Print the query contents to stderr
    void print(FILE* out) const;

    /// Send the contents to a JSONWriter
    void serialize(JSONWriter& out) const;

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
