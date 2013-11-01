/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/querybuf.h"
#include "db/v5/db.h"
#include "db/v5/cursor.h"
#include "db/internals.h"
#include "db/modifiers.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace std;

namespace tut {

struct db_v5_shar : public dballe::tests::DB_test_base
{
	db_v5_shar() : dballe::tests::DB_test_base(db::V5)
	{
	}

	~db_v5_shar()
	{
	}
};
TESTGRP(db_v5);

// Ensure that reset will work on an empty database
template<> template<>
void to::test<1>()
{
    use_db();
    v5::DB& db = v5();

    db.delete_tables();
    db.reset();
    // Run twice to see if it is idempotent
    db.reset();
}

// This query caused problems
template<> template<>
void to::test<2>()
{
    // This query makes no sense: a station query will have a MODIFIER_DISTINCT.
    // querying only ana_id without distinct could predictably have
    // unpredictable effects ( :P ) and is not really useful in real life
#if 0
    use_db();
    wruntest(populate_database);

    query.clear();
    query.set(DBA_KEY_ANA_FILTER, "B07030>1");

    // Perform the query, limited to level values
    v5::DB& db = v5();
    auto_ptr<db::Cursor> cur(db.query(query, DBA_DB_WANT_ANA_ID, 0));
    ensure_equals(cur->remaining(), 2);

    ensure(cur->next());
    ensure(cur->next());
    ensure(!cur->next());
#endif
}

// Insert with undef leveltype2 and l2
template<> template<>
void to::test<3>()
{
        use_db();

        dballe::tests::TestRecord dataset = dataset0;
        dataset.data.unset(WR_VAR(0, 1, 11));
        dataset.data.set(DBA_KEY_LEVELTYPE1, 44);
        dataset.data.set(DBA_KEY_L1, 55);
        dataset.data.unset(DBA_KEY_LEVELTYPE2);
        dataset.data.unset(DBA_KEY_L2);
        wruntest(dataset.insert, *db);

        // Query it back
        query.clear();
        query.set(DBA_KEY_LEVELTYPE1, 44);
        query.set(DBA_KEY_L1, 55);

        v5::DB& db = v5();
        auto_ptr<db::Cursor> cur(db.query(query, DBA_DB_WANT_VAR_VALUE | DBA_DB_WANT_LEVEL, 0));
        ensure_equals(cur->remaining(), 1);

        ensure(cur->next());
        result.clear();
        cur->to_record(result);

        ensure(result.key_peek(DBA_KEY_LEVELTYPE1) != NULL);
        ensure_equals(result[DBA_KEY_LEVELTYPE1].enqi(), 44);
        ensure(result.key_peek(DBA_KEY_L1) != NULL);
        ensure_equals(result[DBA_KEY_L1].enqi(), 55);
        ensure(result.key_peek(DBA_KEY_LEVELTYPE2) != NULL);
        ensure_equals(result[DBA_KEY_LEVELTYPE2].enqi(), MISSING_INT);
        ensure(result.key_peek(DBA_KEY_L2) != NULL);
        ensure_equals(result[DBA_KEY_L2].enqi(), MISSING_INT);

        ensure(!cur->next());
}

// Query with undef leveltype2 and l2
template<> template<>
void to::test<4>()
{
        use_db();
        wruntest(populate_database);

        query.clear();
        query.set(DBA_KEY_LEVELTYPE1, 10);
        query.set(DBA_KEY_L1, 11);

        v5::DB& db = v5();
        auto_ptr<db::Cursor> cur(db.query(query, DBA_DB_WANT_VAR_VALUE, 0));
        ensure_equals(cur->remaining(), 4);
        cur->discard_rest();
}

// Query with an incorrect attr_filter
template<> template<>
void to::test<5>()
{
        use_db();
        wruntest(populate_database);

        query.clear();
        query.set(DBA_KEY_ATTR_FILTER, "B12001");

    try {
        v5::DB& db = v5();
        db.query(query, DBA_DB_WANT_VAR_VALUE, 0);
    } catch (error_consistency& e) {
        ensure_contains(e.what(), "B12001 is not a valid filter");
    }
}

/* Test querying priomax together with query=best */
template<> template<>
void to::test<6>()
{
        use_db();
        // Start with an empty database
        db->reset();

        // Prepare the common parts of some data
        insert.clear();
        insert.set(DBA_KEY_LAT, 1);
        insert.set(DBA_KEY_LON, 1);
        insert.set(DBA_KEY_LEVELTYPE1, 1);
        insert.set(DBA_KEY_L1, 0);
        insert.set(DBA_KEY_PINDICATOR, 254);
        insert.set(DBA_KEY_P1, 0);
        insert.set(DBA_KEY_P2, 0);
        insert.set(DBA_KEY_YEAR, 2009);
        insert.set(DBA_KEY_MONTH, 11);
        insert.set(DBA_KEY_DAY, 11);
        insert.set(DBA_KEY_HOUR, 0);
        insert.set(DBA_KEY_MIN, 0);
        insert.set(DBA_KEY_SEC, 0);

        //  1,synop,synop,101,oss,0
        //  2,metar,metar,81,oss,0
        //  3,temp,sounding,98,oss,2
        //  4,pilot,wind profile,80,oss,2
        //  9,buoy,buoy,50,oss,31
        // 10,ship,synop ship,99,oss,1
        // 11,tempship,temp ship,100,oss,2
        // 12,airep,airep,82,oss,4
        // 13,amdar,amdar,97,oss,4
        // 14,acars,acars,96,oss,4
        // 42,pollution,pollution,199,oss,8
        // 200,satellite,NOAA satellites,41,oss,255
        // 255,generic,generic data,1000,?,255
        static int rep_cods[] = { 1, 2, 3, 4, 9, 10, 11, 12, 13, 14, 42, 200, 255, -1 };

        for (int* i = rep_cods; *i != -1; ++i)
        {
                insert.set(DBA_KEY_REP_COD, *i);
                insert.set(WR_VAR(0, 12, 101), *i);
                db->insert(insert, false, true);
                insert.unset(DBA_KEY_CONTEXT_ID);
        }

        // Query with querybest only
        {
            query.clear();
            query.set(DBA_KEY_QUERY, "best");
            query.set(DBA_KEY_YEAR, 2009);
            query.set(DBA_KEY_MONTH, 11);
            query.set(DBA_KEY_DAY, 11);
            query.set(DBA_KEY_HOUR, 0);
            query.set(DBA_KEY_MIN, 0);
            query.set(DBA_KEY_SEC, 0);
            query.set(DBA_KEY_VAR, "B12101");

        v5::DB& db = v5();
        auto_ptr<db::Cursor> cur(db.query(query, DBA_DB_WANT_REPCOD | DBA_DB_WANT_VAR_VALUE, 0));
        ensure_equals(cur->remaining(), 1);

            ensure(cur->next());
            result.clear();
            cur->to_record(result);

            ensure(result.key_peek(DBA_KEY_REP_COD) != NULL);
            ensure_equals(result[DBA_KEY_REP_COD].enqi(), 255);

            cur->discard_rest();
        }

    // Query with querybest and priomax
    {
        query.clear();
        query.set(DBA_KEY_PRIOMAX, 100);
        query.set(DBA_KEY_QUERY, "best");
        query.set(DBA_KEY_YEAR, 2009);
        query.set(DBA_KEY_MONTH, 11);
        query.set(DBA_KEY_DAY, 11);
        query.set(DBA_KEY_HOUR, 0);
        query.set(DBA_KEY_MIN, 0);
        query.set(DBA_KEY_SEC, 0);
        query.set(DBA_KEY_VAR, "B12101");
        v5::DB& db = v5();
        auto_ptr<db::Cursor> cur(db.query(query, DBA_DB_WANT_REPCOD | DBA_DB_WANT_VAR_VALUE, 0));
        ensure_equals(cur->remaining(), 1);

            ensure(cur->next());
            result.clear();
            cur->to_record(result);

            ensure(result.key_peek(DBA_KEY_REP_COD) != NULL);
            ensure_equals(result[DBA_KEY_REP_COD].enqi(), 11);

            cur->discard_rest();
        }
}

/* Test querying priomax together with query=best */
template<> template<>
void to::test<7>()
{
        use_db();
        wruntest(populate_database);

    Record res;
    Record rec;
    v5::DB& db = v5();
    auto_ptr<db::Cursor> cur = db.query(rec, DBA_DB_WANT_REPCOD, DBA_DB_MODIFIER_DISTINCT);
    while (cur->next())
    {
        cur->to_record(res);
        ensure(res.key_peek_value(DBA_KEY_REP_MEMO) != 0);
    }
}

}

/* vim:set ts=4 sw=4: */
