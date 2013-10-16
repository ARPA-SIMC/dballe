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
#include <wibble/string.h>

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct db1_shar : public dballe::tests::DB_test_base
{
    void test_invalid_sql_querybest();
    void test_bug_querybest();
    void test_bug_query_stations_by_level();
    void test_bug_query_levels_by_station();
    void test_connect_leaks();
};
TESTGRP(db1);

template<> template<> void to::test<1>() { use_db(V5); test_invalid_sql_querybest(); }
template<> template<> void to::test<2>() { use_db(V6); test_invalid_sql_querybest(); }
void db1_shar::test_invalid_sql_querybest()
{
// Reproduce a querybest scenario which produced invalid SQL
    wruntest(populate_database);
    // SELECT pa.lat, pa.lon, pa.ident,
    //        d.datetime, d.id_report, d.id_var, d.value,
    //        ri.prio, pa.id, d.id, d.id_lev_tr
    //   FROM data d
    //   JOIN station pa ON d.id_station = pa.id
    //   JOIN repinfo ri ON ri.id=d.id_report
    //  WHERE pa.lat>=? AND pa.lat<=? AND pa.lon>=? AND pa.lon<=? AND pa.ident IS NULL
    //    AND d.datetime=?
    //    AND d.id_var IN (1822)
    //    AND ri.prio=(SELECT MAX(sri.prio) FROM repinfo sri JOIN data sd ON sri.id=sd.id_report WHERE sd.id_station=d.id_station AND sd.id_lev_tr=d.id_lev_tr AND sd.datetime=d.datetime AND sd.id_var=d.id_var)
    //    AND d.id_lev_tr IS NULLORDER BY d.id_station, d.datetime
    Record rec;
    rec.set(DBA_KEY_YEAR,  1000);
    rec.set(DBA_KEY_MONTH,    1);
    rec.set(DBA_KEY_DAY,      1);
    rec.set(DBA_KEY_HOUR,     0);
    rec.set(DBA_KEY_MIN,     0);
    rec.set(DBA_KEY_QUERY,    "best");
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    while (cur->next())
    {
    }
}

template<> template<> void to::test<3>() { use_db(V5); test_bug_querybest(); }
template<> template<> void to::test<4>() { use_db(V6); test_bug_querybest(); }
void db1_shar::test_bug_querybest()
{
    // Reproduce a querybest scenario which produced always the same data record

    // Import lots
    const char** files = dballe::tests::bufr_files;
    for (int i = 0; files[i] != NULL; i++)
    {
        std::auto_ptr<Msgs> inmsgs = read_msgs(files[i], BUFR);
        Msg& msg = *(*inmsgs)[0];
        wrunchecked(db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS | DBA_IMPORT_OVERWRITE));
    }

    // Query all with best
    Record rec;
    rec.set(DBA_KEY_VAR,   "B12101");
    rec.set(DBA_KEY_QUERY, "best");
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    unsigned orig_count = cur->remaining();
    unsigned count = 0;
    int id_data = 0;
    unsigned id_data_changes = 0;
    while (cur->next())
    {
        ++count;
        if (cur->attr_reference_id() != id_data)
        {
            id_data = cur->attr_reference_id();
            ++id_data_changes;
        }
    }

    ensure(count > 1);
    ensure_equals(id_data_changes, count);
    ensure_equals(count, orig_count);
}

template<> template<> void to::test<5>() { use_db(V5); test_bug_query_stations_by_level(); }
template<> template<> void to::test<6>() { use_db(V6); test_bug_query_stations_by_level(); }
void db1_shar::test_bug_query_stations_by_level()
{
    // Reproduce a query that generated invalid SQL on V6
    wruntest(populate_database);

    // All DB
    query.clear();
    query.set(DBA_KEY_LEVELTYPE1, 103);
    query.set(DBA_KEY_L1, 2000);
    db->query_stations(query);
}

template<> template<> void to::test<7>() { use_db(V5); test_bug_query_levels_by_station(); }
template<> template<> void to::test<8>() { use_db(V6); test_bug_query_levels_by_station(); }
void db1_shar::test_bug_query_levels_by_station()
{
    // Reproduce a query that generated invalid SQL on V6
    wruntest(populate_database);

    // All DB
    query.clear();
    query.set(DBA_KEY_ANA_ID, 1);
    //db->query_levels(query);
    //db->query_tranges(query);
#warning currently disabled
}

template<> template<> void to::test<9>() { use_db(V5); test_connect_leaks(); }
template<> template<> void to::test<10>() { use_db(V6); test_connect_leaks(); }
void db1_shar::test_connect_leaks()
{
    insert.clear();
    insert.set_ana_context();
    insert.set(DBA_KEY_LAT, 12.34560);
    insert.set(DBA_KEY_LON, 76.54320);
    insert.set(DBA_KEY_MOBILE, 0);
    insert.set(DBA_KEY_REP_MEMO, "synop");
    insert.set(WR_VAR(0, 7, 30), 42.0); // Height

    // Assume a max open file limit of 1100
    for (unsigned i = 0; i < 1100; ++i)
    {
        std::auto_ptr<DB> db = DB::connect_test();
        wrunchecked(db->insert(insert, true, true));
    }
}


}

/* vim:set ts=4 sw=4: */

