/*
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

#include "test-utils-db.h"
#include <dballe/db/internals.h>
#include "dballe/db/v5/db.h"
#include "dballe/db/v6/db.h"
#include "dballe/msg/vars.h"
#include <wreport/error.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace wreport;
using namespace std;
using namespace wibble::tests;

namespace dballe {
namespace tests {

void TestStation::set_latlonident_into(Record& rec) const
{
    rec.set(DBA_KEY_LAT, lat);
    rec.set(DBA_KEY_LON, lon);
    if (!ident.empty())
        rec.set(DBA_KEY_IDENT, ident);
    else
        rec.unset(DBA_KEY_IDENT);
}

Record TestStation::merged_info_with_highest_prio(DB& db) const
{
    Record res;
    map<string, int> prios = db.get_repinfo_priorities();
    map<wreport::Varcode, int> cur_prios;
    for (std::map<std::string, Record>::const_iterator i = info.begin();
            i != info.end(); ++i)
    {
        int prio = prios[i->first];
        const vector<Var*>& vars = i->second.vars();
        for (vector<Var*>::const_iterator v = vars.begin(); v != vars.end(); ++v)
        {
            map<wreport::Varcode, int>::const_iterator cur_prio = cur_prios.find((*v)->code());
            if (cur_prio == cur_prios.end() || cur_prio->second < prio)
            {
                res.set(**v);
                cur_prios[(*v)->code()] = prio;
            }
        }
    }
    return res;
}

void TestStation::insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace)
{
    for (std::map<std::string, Record>::const_iterator i = info.begin();
            i != info.end(); ++i)
    {
        if (!i->second.vars().empty())
        {
            WIBBLE_TEST_INFO(locinfo);
            Record insert(i->second);
            set_latlonident_into(insert);
            insert.set(DBA_KEY_REP_MEMO, i->first);
            insert.set_ana_context();
            locinfo() << insert.to_string();
            wrunchecked(db.insert(insert, can_replace, true));
        }
    }
}

void TestRecord::insert(WIBBLE_TEST_LOCPRM, DB& db, bool can_replace)
{
    // Insert the station info
    wruntest(station.insert, db, can_replace);

    // Insert variables
    if (!data.vars().empty())
    {
        WIBBLE_TEST_INFO(locinfo);
        Record insert(data);
        wrunchecked(station.set_latlonident_into(insert));
        locinfo() << insert.to_string();
        wrunchecked(db.insert(insert, can_replace, true));
    }

    // Insert attributes
    for (std::map<wreport::Varcode, Record>::const_iterator i = attrs.begin();
            i != attrs.end(); ++i)
    {
        WIBBLE_TEST_INFO(locinfo);
        locinfo() << wreport::varcode_format(i->first) << ": " << i->second.to_string();
        wrunchecked(db.attr_insert(i->first, i->second, can_replace));
    }
}

void TestRecord::set_var(const char* msgvarname, float val, int conf)
{
    int msgvarid = resolve_var(msgvarname);
    const MsgVarShortcut& v = shortcutTable[msgvarid];
    data.set(Level(v.ltype1, v.l1, v.ltype2, v.l2));
    data.set(Trange(v.pind, v.p1, v.p2));
    data.set(v.code, val);
    if (conf != -1)
        attrs[v.code].set(WR_VAR(0, 33, 7), conf);
}

void TestCursorStationKeys::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur.get_lat()) == ds.lat);
    wassert(actual(cur.get_lon()) == ds.lon);
    wassert(actual(cur.get_ident()) == ds.ident);
}

void TestCursorStationVars::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).station_keys_match(ds));

    Record expected = ds.merged_info_with_highest_prio(cur.get_db());

    Record rec;
    cur.to_record(rec);
    wassert(actual(rec).vars_equal(expected));
}

void TestCursorDataContext::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).station_keys_match(ds.station));
    wassert(actual(cur.get_rep_memo()) == ds.data.get(DBA_KEY_REP_MEMO, ""));
    wassert(actual(cur.get_level()) == ds.data.get_level());
    wassert(actual(cur.get_trange()) == ds.data.get_trange());
    int dt[6];
    cur.get_datetime(dt);
    wassert(actual(dt[0]) == ds.data.get(DBA_KEY_YEAR, MISSING_INT));
    wassert(actual(dt[1]) == ds.data.get(DBA_KEY_MONTH, MISSING_INT));
    wassert(actual(dt[2]) == ds.data.get(DBA_KEY_DAY, MISSING_INT));
    wassert(actual(dt[3]) == ds.data.get(DBA_KEY_HOUR, MISSING_INT));
    wassert(actual(dt[4]) == ds.data.get(DBA_KEY_MIN, MISSING_INT));
    wassert(actual(dt[5]) == ds.data.get(DBA_KEY_SEC, MISSING_INT));

    Record a;
    cur.to_record(a);
    wassert(actual(a).equals(ds.data, DBA_KEY_REP_MEMO));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_LEVELTYPE1));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_L1));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_LEVELTYPE2));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_L2));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_PINDICATOR));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_P1));
    wassert(actual(a).equals_with_missing_int(ds.data, DBA_KEY_P2));
    wassert(actual(a).equals(ds.data, DBA_KEY_YEAR));
    wassert(actual(a).equals(ds.data, DBA_KEY_MONTH));
    wassert(actual(a).equals(ds.data, DBA_KEY_DAY));
    wassert(actual(a).equals(ds.data, DBA_KEY_HOUR));
    wassert(actual(a).equals(ds.data, DBA_KEY_MIN));
    wassert(actual(a).equals(ds.data, DBA_KEY_SEC));
}

void TestCursorDataVar::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur.get_varcode()) == code);
    wassert(actual(cur.get_var()) == ds.data[code]);
    Record a;
    cur.to_record(a);
    wassert(actual(a).equals(ds.data, code));
}

void TestCursorDataMatch::check(WIBBLE_TEST_LOCPRM) const
{
    wassert(actual(cur).data_context_matches(ds));
    wassert(actual(cur).data_var_matches(ds, code));
}


db_test::db_test()
{
    orig_format = DB::get_default_format();
}

db_test::db_test(db::Format format, bool reset)
{
    orig_format = DB::get_default_format();
    if (reset) disappear();
    DB::set_default_format(format);
    db = DB::connect_test();
    if (reset) db->reset();
}

db_test::~db_test()
{
    DB::set_default_format(orig_format);
}

void db_test::disappear()
{
    auto_ptr<DB> db(DB::connect_test());
    db->disappear();
}

void db_test::use_db()
{
    if (!has_db()) throw tut::no_such_test();
}

void db_test::use_db(db::Format format, bool reset)
{
    if (getenv("DBALLE_SKIP_DB_TESTS")) throw tut::no_such_test();

    if (!db.get())
    {
        if (reset) disappear();
        DB::set_default_format(format);
        db = DB::connect_test();
    }
    if (db->format() != format)
        throw error_consistency("DB format mismatch in test init");
    use_db();
    if (reset) db->reset();
}

db::v5::DB& db_test::v5()
{
    if (db::v5::DB* d = dynamic_cast<db::v5::DB*>(db.get()))
        return *d;
    else
        throw error_consistency("test DB is not a v5 DB");
}

db::v6::DB& db_test::v6()
{
    if (db::v6::DB* d = dynamic_cast<db::v6::DB*>(db.get()))
        return *d;
    else
        throw error_consistency("test DB is not a v6 DB");
}

DB_test_base::DB_test_base()
{
    init_records();
}
DB_test_base::DB_test_base(db::Format format) : db_test(format)
{
    init_records();
}

void DB_test_base::init_records()
{
    ds_st_oldtests.lat = 12.34560;
    ds_st_oldtests.lon = 76.54320;
    ds_st_oldtests.info["synop"].set(WR_VAR(0, 7, 30), 42);     // Height
    ds_st_oldtests.info["synop"].set(WR_VAR(0, 7, 31), 234);    // Heightbaro
    ds_st_oldtests.info["synop"].set(WR_VAR(0, 1,  1), 1);      // Block
    ds_st_oldtests.info["synop"].set(WR_VAR(0, 1,  2), 52);     // Station
    ds_st_oldtests.info["synop"].set(WR_VAR(0, 1, 19), "Cippo Lippo");  // Name

    dataset0.station = ds_st_oldtests;
    dataset0.data.set(DBA_KEY_REP_MEMO, "synop");
    dataset0.data.set(Level(10, 11, 15, 22));
    dataset0.data.set(Trange(20, 111, 122));
    dataset0.data.set_datetime(1945, 4, 25, 8);

    dataset1 = dataset0;
    dataset1.data.set(DBA_KEY_REP_MEMO, "metar");
    dataset1.data.set(DBA_KEY_MIN, 30);
    dataset1.data.set(DBA_KEY_P2, 123);

    dataset0.data.set(WR_VAR(0, 1, 11), "DB-All.e!");
    dataset0.data.set(WR_VAR(0, 1, 12), 300);
    dataset1.data.set(WR_VAR(0, 1, 11), "Arpa-Sim!");
    dataset1.data.set(WR_VAR(0, 1, 12), 400);

    ds_st_navile.lat = 44.5008;
    ds_st_navile.lon = 11.3288;
    ds_st_navile.info["synop"].set(WR_VAR(0, 7, 30), 78); // Height
}

void DB_test_base::populate_database(WIBBLE_TEST_LOCPRM)
{
    /* Start with an empty database */
    db->reset();

    wruntest(dataset0.insert, *db, false);
    // replace=true to replace existing station info data
    wruntest(dataset1.insert, *db, true);
}

} // namespace tests
} // namespace dballe
