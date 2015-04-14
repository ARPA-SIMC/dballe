/*
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
#include "db/test-utils-db.h"
#include "summary.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct db_summary_shar: public dballe::tests::db_test
{
    db_summary_shar() : db_test(V6)
    {
    }

    ~db_summary_shar()
    {
    }
};
TESTGRP(db_summary);

// Test building a summary and checking if it supports queries
template<> template<>
void to::test<1>()
{
    wruntest(populate<OldDballeTestFixture>);

    Query query;
    query.set(DBA_KEY_QUERY, "details");
    Summary s(query);
    wassert(actual(s.is_valid()).isfalse());

    // Build the whole db summary
    auto cur = db->query_summary(query);
    while (cur->next())
        s.add_summary(*cur, true);

    // Check its contents
    wassert(actual(s.is_valid()).istrue());
    wassert(actual(s.all_stations.size()) == 1);
    wassert(actual(s.all_levels.size()) == 1);
    wassert(actual(s.all_tranges.size()) == 2);
    wassert(actual(s.all_varcodes.size()) == 2);
    wassert(actual(s.datetime_min()) == Datetime(1945, 4, 25, 8));
    wassert(actual(s.datetime_max()) == Datetime(1945, 4, 25, 8, 30));
    wassert(actual(s.data_count()) == 4);

    // Check what it can support

    // An existing station is ok: we know we have it
    query.clear(); query.set_from_string("ana_id=1");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);

    // A non-existing station is also ok: we know we don't have it
    query.clear(); query.set_from_string("ana_id=2");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);

    query.clear(); query.set_from_string("ana_id=1, leveltype=10");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);

    query.clear(); query.set_from_string("ana_id=1, leveltype=10, pindicator=20");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);

    query.clear(); query.set_from_string("ana_id=1, leveltype=10, pindicator=20");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);

    // Still exact, because the query matches the entire summary
    query.clear(); query.set_from_string("yearmin=1945");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);

    // Still exact, because although the query partially matches the summary,
    // each summary entry is entier included completely or excluded completely
    query.clear(); query.set_from_string("yearmin=1945, monthmin=4, daymin=25, hourmin=8, yearmax=1945, monthmax=4, daymax=25, hourmax=8, minumax=10");
    wassert(actual(s.supports(query)) == summary::Support::EXACT);
}

// Test summary::Stack
template<> template<>
void to::test<2>()
{
    using namespace summary;

    wruntest(populate<OldDballeTestFixture>);

    Query query;
    query.set(DBA_KEY_QUERY, "details");
    Summary s(query);
    wassert(actual(s.is_valid()).isfalse());

    Stack stack;

    // Build the whole db summary
    Summary& general = stack.push(Query());
    auto cur = db->query_summary(query);
    while (cur->next())
        general.add_summary(*cur, true);

    wassert(actual(stack.size()) == 1);
    wassert(actual(stack.top().data_count()) == 4);

    // Query the stack
    query.clear(); query.set("rep_memo", "synop");
    Support res = stack.query(query, true, [](const Entry& e) { return e.rep_memo == "synop"; });
    wassert(actual(res) == EXACT);

    wassert(actual(stack.size()) == 2);
    wassert(actual(stack.top().data_count()) == 2);

    // Query further
    query.clear(); query.set("rep_memo", "synop"); query.set("var", "B01011");
    res = stack.query(query, true, [](const Entry& e) { return e.rep_memo == "synop" && e.varcode == WR_VAR(0, 1, 11); });
    wassert(actual(res) == EXACT);

    wassert(actual(stack.size()) == 2);
    wassert(actual(stack.top().data_count()) == 1);

    // Query the same var but a different rep_memo
    query.clear(); query.set("rep_memo", "metar"); query.set("var", "B01011");
    res = stack.query(query, true, [](const Entry& e) { return e.rep_memo == "metar" && e.varcode == WR_VAR(0, 1, 11); });
    wassert(actual(res) == EXACT);

    wassert(actual(stack.size()) == 2);
    wassert(actual(stack.top().data_count()) == 1);
}

}

