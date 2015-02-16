/*
 * db/summary - High level code for working with DB-All.e DB summaries
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

#ifndef DBALLE_DB_SUMMARY_H
#define DBALLE_DB_SUMMARY_H

#include <dballe/core/record.h>
#include <dballe/db/db.h>
#include <vector>
#include <set>

namespace dballe {
namespace db {

class Matcher;
class Summary;

namespace summary {

/// Represent whether a summary can satisfy a given query
enum Support
{
    /// It cannot.
    UNSUPPORTED = 0,
    /// The query may select less data than this summary can estimate.
    OVERESTIMATED = 1,
    /// The query selects data that this summary can estimate exactly.
    EXACT = 2,
};

struct Entry
{
    int ana_id;
    std::string rep_memo;
    dballe::Level level;
    dballe::Trange trange;
    wreport::Varcode varcode;
    dballe::Datetime datemin;
    dballe::Datetime datemax;
    int count = MISSING_INT;

    Entry(db::Cursor& cur, bool want_details);
};

}

class Summary
{
protected:
    // Query that generated this summary
    Query query;

    // Summary of items for the currently active filter
    std::vector<summary::Entry> summary;

    void aggregate(const summary::Entry& entry);

public:
    Summary(const Query& query);

    // True if the summary has been filled with data
    bool valid = false;

    std::set<int> all_stations;
    std::set<std::string> all_reports;
    std::set<dballe::Level> all_levels;
    std::set<dballe::Trange> all_tranges;
    std::set<wreport::Varcode> all_varcodes;
    std::set<std::string> all_idents;

    // Last known minimum datetime for the data that we have
    dballe::Datetime dtmin;
    // Last known maximum datetime for the data that we have
    dballe::Datetime dtmax;
    // Last known count for the data that we have
    unsigned count = MISSING_INT;

    /// Return true if the summary has been filled with data
    bool is_valid() const { return valid; }

    const Datetime& datetime_min() const { return dtmin; }
    const Datetime& datetime_max() const { return dtmax; }
    unsigned data_count() const { return count; }

    /**
     * Checks if this summary correctly generate a
     * summary for the given query.
     */
    summary::Support supports(const Query& query) const;

    /// Add an entry to the summary taken from the current status of \a cur
    void add_summary(db::Cursor& cur, bool with_details);

    /// Add a copy of an existing entry
    void add_entry(const summary::Entry& entry);

    /// Iterate all values in the summary
    bool iterate(std::function<bool(const summary::Entry&)> f) const;
};

namespace summary {

/**
 * Stack of summary in increasing order of selectivity.
 *
 * This is used to keep enough context to refine a summary as a query is
 * refined, and to go back to a wider summary if the query is relaxed.
 *
 * Ideally, there could be several levels in the stack, so that each subquery
 * takes the previous one as a starting point, and the process gets faster and
 * faster; however, a summary may support a query (in the sense that it knows
 * that that query selects no data at all) while a more general summary may
 * support it and return data (e.g. ask first for rep_memo=synop, then change
 * one's mind and ask for rep_memo=temp.
 *
 * Supporting such scenarios would require implementing nontrivial logic for an
 * optimization that it is still unclear to me if it would be required. At the
 * moment, I simplify implementation by just supporting two levels, and having
 * each query always start from the topmost summary.
 */
class Stack
{
protected:
    /**
     * Summaries for the current query.
     *
     * summaries[0] is always the summary for the whole database;
     * further summaries are appended as the query is refined.
     */
    std::vector<Summary> summaries;

public:
    /// Check if the stack is empty
    bool is_empty() const { return summaries.empty(); }

    /// Return the stack size. Only really useful for tests.
    unsigned size() const { return summaries.size(); }

    /// Add a new summary to the stack, and return a reference to it
    Summary& push(const Query& query);

    /// Return the topmost summary
    const Summary& top() const { return summaries.back(); }

    /**
     * If the current summary stack can support the given query, append the
     * resulting summary to the stack, else, remove all entries from the stack
     * except the most general one.
     *
     * @returns how the resulting stack supports the query.
     */
    Support query(const Query& query, bool exact, std::function<bool(const Entry&)> match);
};

}

}
}

#endif
