/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"
#include "db/test-utils-db.h"
#include "db/db.h"
#include "core/record.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct MsgCollector : public vector<Msg*>, public MsgConsumer
{
    ~MsgCollector()
    {
        for (iterator i = begin(); i != end(); ++i)
            delete *i;
    }
    void operator()(unique_ptr<Msg> msg) override
    {
        push_back(msg.release());
    }
};

struct Fixture : public dballe::tests::DBFixture
{
    Fixture()
    {
    }

    void populate_database()
    {
        using namespace dballe::tests;

        TestStation st1;
        st1.lat = 12.34560;
        st1.lon = 76.54321;
        //st1.info["synop"]

        TestStation st2(st1);
        st2.ident = "ciao";

        TestRecord ds0;
        ds0.station = st1;
        ds0.data.set_datetime(1945, 4, 25, 8, 0);
        ds0.data.set(Level(1, 2, 0, 3));
        ds0.data.set(Trange(4, 5, 6));
        ds0.data.set(DBA_KEY_REP_MEMO, "synop");
        ds0.data.set(WR_VAR(0, 1, 12), 500);

        TestRecord ds1(ds0);
        ds1.data.set_datetime(1945, 4, 26, 8, 0);
        ds1.data.set(WR_VAR(0, 1, 12), 400);

        TestRecord ds2(ds1);
        ds2.station = st2;
        ds2.data.set(WR_VAR(0, 1, 12), 300);

        TestRecord ds3(ds2);
        ds3.station = st2;
        ds3.data.set(DBA_KEY_REP_MEMO, "metar");
        ds3.data.set(WR_VAR(0, 1, 12), 200);

        wruntest(ds0.insert, *db, false);
        wruntest(ds1.insert, *db, false);
        wruntest(ds2.insert, *db, false);
        wruntest(ds3.insert, *db, false);
    }
};


typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("export", [](Fixture& f) {
        // Simple export
        auto& db = f.db;
        f.populate_database();

        // Put some data in the database and check that it gets exported properly
        // Query back the data
        Query query;

        MsgCollector msgs;
        db->export_msgs(query, msgs);
        ensure_equals(msgs.size(), 4u);

        if (msgs[2]->type == MSG_METAR)
        {
            // Since the order here is not determined, enforce one
            Msg* tmp = msgs[2];
            msgs[2] = msgs[3];
            msgs[3] = tmp;
        }

        ensure_equals(msgs[0]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LONGITUDE), 76.54321);
        ensure_msg_undef(*msgs[0], DBA_MSG_IDENT);
        wassert(actual(msgs[0]->datetime()) == Datetime(1945, 4, 25, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[0], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 500);

        ensure_equals(msgs[1]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[1], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[1], DBA_MSG_LONGITUDE), 76.54321);
        ensure_msg_undef(*msgs[1], DBA_MSG_IDENT);
        wassert(actual(msgs[1]->datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[1], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 400);

        ensure_equals(msgs[2]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[2], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[2], DBA_MSG_LONGITUDE), 76.54321);
        ensure_var_equals(want_var(*msgs[2], DBA_MSG_IDENT), "ciao");
        wassert(actual(msgs[2]->datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[2], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 300);

        ensure_equals(msgs[3]->type, MSG_METAR);
        ensure_var_equals(want_var(*msgs[3], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[3], DBA_MSG_LONGITUDE), 76.54321);
        ensure_var_equals(want_var(*msgs[3], DBA_MSG_IDENT), "ciao");
        wassert(actual(msgs[3]->datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[3], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 200);
    }),
    Test("export", [](Fixture& f) {
        // Text exporting of extra station information
        auto& db = f.db;

        // Import some data in the station extra information context
        Record in;
        in.set(DBA_KEY_LAT, 45.0);
        in.set(DBA_KEY_LON, 11.0);
        in.set(DBA_KEY_REP_MEMO, "synop");
        in.set_ana_context();
        in.set(WR_VAR(0, 1, 1), 10);
        db->insert(in, false, true);

        // Import one real datum
        in.clear();
        in.set(DBA_KEY_LAT, 45.0);
        in.set(DBA_KEY_LON, 11.0);
        in.set(DBA_KEY_REP_MEMO, "synop");
        in.set(DBA_KEY_YEAR, 2000);
        in.set(DBA_KEY_MONTH, 1);
        in.set(DBA_KEY_DAY, 1);
        in.set(DBA_KEY_HOUR, 0);
        in.set(DBA_KEY_MIN, 0);
        in.set(DBA_KEY_SEC, 0);
        in.set(DBA_KEY_LEVELTYPE1, 103);
        in.set(DBA_KEY_L1, 2000);
        in.set(DBA_KEY_PINDICATOR, 254);
        in.set(DBA_KEY_P1, 0);
        in.set(DBA_KEY_P2, 0);
        in.set(WR_VAR(0, 12, 101), 290.0);
        db->insert(in, false, true);

        // Query back the data
        Query query;
        MsgCollector msgs;
        db->export_msgs(query, msgs);
        ensure_equals(msgs.size(), 1u);

        ensure_equals(msgs[0]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LATITUDE), 45.0);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LONGITUDE), 11.0);
        ensure_msg_undef(*msgs[0], DBA_MSG_IDENT);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_BLOCK), 10);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_TEMP_2M), 290.0);
    }),
};

test_group tg1("db_export_mem", nullptr, db::MEM, tests);
test_group tg2("db_export_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_export_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_export_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_export_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
