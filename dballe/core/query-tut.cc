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

#include "core/test-utils-core.h"
#include "core/query.h"

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("seti", [](Fixture& f) {
        Query q;

        q.seti(DBA_KEY_PRIORITY, 1);
        wassert(actual(q.prio_exact) == 1);
        q.seti(DBA_KEY_PRIOMAX, 2);
        wassert(actual(q.prio_max) == 2);
        q.seti(DBA_KEY_PRIOMIN, 3);
        wassert(actual(q.prio_min) == 3);
        q.seti(DBA_KEY_ANA_ID, 4);
        wassert(actual(q.ana_id) == 4);
        q.seti(DBA_KEY_MOBILE, 0);
        wassert(actual(q.mobile) == 0);
        q.seti(DBA_KEY_LAT, 4412300);
        wassert(actual(q.coords_exact.lat) == 4412300);
        q.seti(DBA_KEY_LON, -1112300);
        wassert(actual(q.coords_exact.lon) == -1112300);
        q.seti(DBA_KEY_LATMAX, 4410000);
        wassert(actual(q.coords_max.lat) == 4410000);
        q.seti(DBA_KEY_LONMAX, 35950000);
        wassert(actual(q.coords_max.lon) == -50000);
        q.seti(DBA_KEY_LATMIN, 4420000);
        wassert(actual(q.coords_min.lat) == 4420000);
        q.seti(DBA_KEY_LONMIN, 1120000);
        wassert(actual(q.coords_min.lon) == 1120000);
        q.seti(DBA_KEY_YEAR, 2000);
        wassert(actual(q.datetime_exact.date.year) == 2000);
        q.seti(DBA_KEY_MONTH, 1);
        wassert(actual(q.datetime_exact.date.month) == 1);
        q.seti(DBA_KEY_DAY, 2);
        wassert(actual(q.datetime_exact.date.day) == 2);
        q.seti(DBA_KEY_HOUR, 12);
        wassert(actual(q.datetime_exact.time.hour) == 12);
        q.seti(DBA_KEY_MIN, 30);
        wassert(actual(q.datetime_exact.time.minute) == 30);
        q.seti(DBA_KEY_SEC, 45);
        wassert(actual(q.datetime_exact.time.second) == 45);

        q.seti(DBA_KEY_YEARMIN, 2001);
        wassert(actual(q.datetime_min.date.year) == 2001);
        q.seti(DBA_KEY_MONTHMIN, 2);
        wassert(actual(q.datetime_min.date.month) == 2);
        q.seti(DBA_KEY_DAYMIN, 3);
        wassert(actual(q.datetime_min.date.day) == 3);
        q.seti(DBA_KEY_HOURMIN, 13);
        wassert(actual(q.datetime_min.time.hour) == 13);
        q.seti(DBA_KEY_MINUMIN, 31);
        wassert(actual(q.datetime_min.time.minute) == 31);
        q.seti(DBA_KEY_SECMIN, 46);
        wassert(actual(q.datetime_min.time.second) == 46);

        q.seti(DBA_KEY_YEARMAX, 2002);
        wassert(actual(q.datetime_max.date.year) == 2002);
        q.seti(DBA_KEY_MONTHMAX, 3);
        wassert(actual(q.datetime_max.date.month) == 3);
        q.seti(DBA_KEY_DAYMAX, 4);
        wassert(actual(q.datetime_max.date.day) == 4);
        q.seti(DBA_KEY_HOURMAX, 14);
        wassert(actual(q.datetime_max.time.hour) == 14);
        q.seti(DBA_KEY_MINUMAX, 32);
        wassert(actual(q.datetime_max.time.minute) == 32);
        q.seti(DBA_KEY_SECMAX, 47);
        wassert(actual(q.datetime_max.time.second) == 47);

        q.seti(DBA_KEY_LEVELTYPE1, 10);
        wassert(actual(q.level.ltype1) == 10);
        q.seti(DBA_KEY_L1, 11);
        wassert(actual(q.level.l1) == 11);
        q.seti(DBA_KEY_LEVELTYPE2, 12);
        wassert(actual(q.level.ltype2) == 12);
        q.seti(DBA_KEY_L2, 13);
        wassert(actual(q.level.l2) == 13);

        q.seti(DBA_KEY_PINDICATOR, 20);
        wassert(actual(q.trange.pind) == 20);
        q.seti(DBA_KEY_P1, 21);
        wassert(actual(q.trange.p1) == 21);
        q.seti(DBA_KEY_P2, 22);
        wassert(actual(q.trange.p2) == 22);

        q.seti(DBA_KEY_VAR, WR_VAR(0, 12, 101));
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));

        q.seti(DBA_KEY_VARLIST, WR_VAR(0, 12, 102));
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 102));

        q.seti(DBA_KEY_LIMIT, 100);
        wassert(actual(q.limit) == 100);
    }),

    Test("setd", [](Fixture& f) {
        Query q;

        q.setd(DBA_KEY_PRIORITY, 1.0);
        wassert(actual(q.prio_exact) == 1);
        q.setd(DBA_KEY_PRIOMAX, 2.5);
        wassert(actual(q.prio_max) == 3);
        q.setd(DBA_KEY_PRIOMIN, 4.1);
        wassert(actual(q.prio_min) == 4);
        q.setd(DBA_KEY_ANA_ID, 4.0);
        wassert(actual(q.ana_id) == 4);
        q.setd(DBA_KEY_MOBILE, 0.0);
        wassert(actual(q.mobile) == 0);
        q.setd(DBA_KEY_LAT, 44.12300);
        wassert(actual(q.coords_exact.lat) == 4412300);
        q.setd(DBA_KEY_LON, -11.12300);
        wassert(actual(q.coords_exact.lon) == -1112300);
        q.setd(DBA_KEY_LATMAX, 44.10000);
        wassert(actual(q.coords_max.lat) == 4410000);
        q.setd(DBA_KEY_LONMAX, 359.50000);
        wassert(actual(q.coords_max.lon) == -50000);
        q.setd(DBA_KEY_LATMIN, 44.20000);
        wassert(actual(q.coords_min.lat) == 4420000);
        q.setd(DBA_KEY_LONMIN, 11.20000);
        wassert(actual(q.coords_min.lon) == 1120000);
        q.setd(DBA_KEY_YEAR, 2000.0);
        wassert(actual(q.datetime_exact.date.year) == 2000);
        q.setd(DBA_KEY_MONTH, 1.0);
        wassert(actual(q.datetime_exact.date.month) == 1);
        q.setd(DBA_KEY_DAY, 2.0);
        wassert(actual(q.datetime_exact.date.day) == 2);
        q.setd(DBA_KEY_HOUR, 12.0);
        wassert(actual(q.datetime_exact.time.hour) == 12);
        q.setd(DBA_KEY_MIN, 30.0);
        wassert(actual(q.datetime_exact.time.minute) == 30);
        q.setd(DBA_KEY_SEC, 45.0);
        wassert(actual(q.datetime_exact.time.second) == 45);

        q.setd(DBA_KEY_YEARMIN, 2001.0);
        wassert(actual(q.datetime_min.date.year) == 2001);
        q.setd(DBA_KEY_MONTHMIN, 2.0);
        wassert(actual(q.datetime_min.date.month) == 2);
        q.setd(DBA_KEY_DAYMIN, 3.0);
        wassert(actual(q.datetime_min.date.day) == 3);
        q.setd(DBA_KEY_HOURMIN, 13.0);
        wassert(actual(q.datetime_min.time.hour) == 13);
        q.setd(DBA_KEY_MINUMIN, 31.0);
        wassert(actual(q.datetime_min.time.minute) == 31);
        q.setd(DBA_KEY_SECMIN, 46.0);
        wassert(actual(q.datetime_min.time.second) == 46);

        q.setd(DBA_KEY_YEARMAX, 2002.0);
        wassert(actual(q.datetime_max.date.year) == 2002);
        q.setd(DBA_KEY_MONTHMAX, 3.0);
        wassert(actual(q.datetime_max.date.month) == 3);
        q.setd(DBA_KEY_DAYMAX, 4.0);
        wassert(actual(q.datetime_max.date.day) == 4);
        q.setd(DBA_KEY_HOURMAX, 14.0);
        wassert(actual(q.datetime_max.time.hour) == 14);
        q.setd(DBA_KEY_MINUMAX, 32.0);
        wassert(actual(q.datetime_max.time.minute) == 32);
        q.setd(DBA_KEY_SECMAX, 47.0);
        wassert(actual(q.datetime_max.time.second) == 47);

        q.setd(DBA_KEY_LEVELTYPE1, 10.0);
        wassert(actual(q.level.ltype1) == 10);
        q.setd(DBA_KEY_L1, 11.0);
        wassert(actual(q.level.l1) == 11);
        q.setd(DBA_KEY_LEVELTYPE2, 12.0);
        wassert(actual(q.level.ltype2) == 12);
        q.setd(DBA_KEY_L2, 13.0);
        wassert(actual(q.level.l2) == 13);

        q.setd(DBA_KEY_PINDICATOR, 20.0);
        wassert(actual(q.trange.pind) == 20);
        q.setd(DBA_KEY_P1, 21.0);
        wassert(actual(q.trange.p1) == 21);
        q.setd(DBA_KEY_P2, 22.0);
        wassert(actual(q.trange.p2) == 22);

        q.setd(DBA_KEY_LIMIT, 100.0);
        wassert(actual(q.limit) == 100);
    }),

    Test("setc_cstring", [](Fixture& f) {
        Query q;

        q.setc(DBA_KEY_PRIORITY, "1");
        wassert(actual(q.prio_exact) == 1);
        q.setc(DBA_KEY_PRIOMAX, "2");
        wassert(actual(q.prio_max) == 2);
        q.setc(DBA_KEY_PRIOMIN, "3");
        wassert(actual(q.prio_min) == 3);
        q.setc(DBA_KEY_REP_MEMO, "synop");
        wassert(actual(q.rep_memo) == "synop");
        q.setc(DBA_KEY_ANA_ID, "4");
        wassert(actual(q.ana_id) == 4);
        q.setc(DBA_KEY_MOBILE, "1");
        wassert(actual(q.mobile) == 1);
        q.setc(DBA_KEY_IDENT, "spc");
        wassert(actual(q.ident) == "spc");
        q.setc(DBA_KEY_LAT, "4412300");
        wassert(actual(q.coords_exact.lat) == 4412300);
        q.setc(DBA_KEY_LON, "-1112300");
        wassert(actual(q.coords_exact.lon) == -1112300);
        q.setc(DBA_KEY_LATMAX, "4410000");
        wassert(actual(q.coords_max.lat) == 4410000);
        q.setc(DBA_KEY_LONMAX, "35950000");
        wassert(actual(q.coords_max.lon) == -50000);
        q.setc(DBA_KEY_LATMIN, "4420000");
        wassert(actual(q.coords_min.lat) == 4420000);
        q.setc(DBA_KEY_LONMIN, "1120000");
        wassert(actual(q.coords_min.lon) == 1120000);

        q.setc(DBA_KEY_YEAR, "2000");
        wassert(actual(q.datetime_exact.date.year) == 2000);
        q.setc(DBA_KEY_MONTH, "1");
        wassert(actual(q.datetime_exact.date.month) == 1);
        q.setc(DBA_KEY_DAY, "2");
        wassert(actual(q.datetime_exact.date.day) == 2);
        q.setc(DBA_KEY_HOUR, "12");
        wassert(actual(q.datetime_exact.time.hour) == 12);
        q.setc(DBA_KEY_MIN, "30");
        wassert(actual(q.datetime_exact.time.minute) == 30);
        q.setc(DBA_KEY_SEC, "45");
        wassert(actual(q.datetime_exact.time.second) == 45);

        q.setc(DBA_KEY_YEARMIN, "2001");
        wassert(actual(q.datetime_min.date.year) == 2001);
        q.setc(DBA_KEY_MONTHMIN, "2");
        wassert(actual(q.datetime_min.date.month) == 2);
        q.setc(DBA_KEY_DAYMIN, "3");
        wassert(actual(q.datetime_min.date.day) == 3);
        q.setc(DBA_KEY_HOURMIN, "13");
        wassert(actual(q.datetime_min.time.hour) == 13);
        q.setc(DBA_KEY_MINUMIN, "31");
        wassert(actual(q.datetime_min.time.minute) == 31);
        q.setc(DBA_KEY_SECMIN, "46");
        wassert(actual(q.datetime_min.time.second) == 46);

        q.setc(DBA_KEY_YEARMAX, "2002");
        wassert(actual(q.datetime_max.date.year) == 2002);
        q.setc(DBA_KEY_MONTHMAX, "3");
        wassert(actual(q.datetime_max.date.month) == 3);
        q.setc(DBA_KEY_DAYMAX, "4");
        wassert(actual(q.datetime_max.date.day) == 4);
        q.setc(DBA_KEY_HOURMAX, "14");
        wassert(actual(q.datetime_max.time.hour) == 14);
        q.setc(DBA_KEY_MINUMAX, "32");
        wassert(actual(q.datetime_max.time.minute) == 32);
        q.setc(DBA_KEY_SECMAX, "47");
        wassert(actual(q.datetime_max.time.second) == 47);

        q.setc(DBA_KEY_LEVELTYPE1, "10");
        wassert(actual(q.level.ltype1) == 10);
        q.setc(DBA_KEY_L1, "11");
        wassert(actual(q.level.l1) == 11);
        q.setc(DBA_KEY_LEVELTYPE2, "12");
        wassert(actual(q.level.ltype2) == 12);
        q.setc(DBA_KEY_L2, "13");
        wassert(actual(q.level.l2) == 13);

        q.setc(DBA_KEY_PINDICATOR, "20");
        wassert(actual(q.trange.pind) == 20);
        q.setc(DBA_KEY_P1, "21");
        wassert(actual(q.trange.p1) == 21);
        q.setc(DBA_KEY_P2, "22");
        wassert(actual(q.trange.p2) == 22);

        q.setc(DBA_KEY_LIMIT, "100");
        wassert(actual(q.limit) == 100);

        q.setc(DBA_KEY_VAR, "B12101");
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));

        q.setc(DBA_KEY_VARLIST, "B12102,v");
        wassert(actual(q.varcodes.size()) == 2);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 11, 4));
        wassert(actual(*q.varcodes.rbegin()) == WR_VAR(0, 12, 102));

        q.setc(DBA_KEY_QUERY, "best");
        wassert(actual(q.query) == "best");
        q.setc(DBA_KEY_ANA_FILTER, "B01001=1");
        wassert(actual(q.ana_filter) == "B01001=1");
        q.setc(DBA_KEY_DATA_FILTER, "B12101>260");
        wassert(actual(q.data_filter) == "B12101>260");
        q.setc(DBA_KEY_ATTR_FILTER, "B33007>50");
        wassert(actual(q.attr_filter) == "B33007>50");
    }),

    Test("setc_string", [](Fixture& f) {
        Query q;

        // Force setting to a string
        auto do_set = [&](dba_keyword key, const std::string& s) {
            q.setc(key, s);
        };

        do_set(DBA_KEY_PRIORITY, "1");
        wassert(actual(q.prio_exact) == 1);
        do_set(DBA_KEY_PRIOMAX, "2");
        wassert(actual(q.prio_max) == 2);
        do_set(DBA_KEY_PRIOMIN, "3");
        wassert(actual(q.prio_min) == 3);
        do_set(DBA_KEY_REP_MEMO, "synop");
        wassert(actual(q.rep_memo) == "synop");
        do_set(DBA_KEY_ANA_ID, "4");
        wassert(actual(q.ana_id) == 4);
        do_set(DBA_KEY_MOBILE, "1");
        wassert(actual(q.mobile) == 1);
        do_set(DBA_KEY_IDENT, "spc");
        wassert(actual(q.ident) == "spc");
        do_set(DBA_KEY_LAT, "4412300");
        wassert(actual(q.coords_exact.lat) == 4412300);
        do_set(DBA_KEY_LON, "-1112300");
        wassert(actual(q.coords_exact.lon) == -1112300);
        do_set(DBA_KEY_LATMAX, "4410000");
        wassert(actual(q.coords_max.lat) == 4410000);
        do_set(DBA_KEY_LONMAX, "35950000");
        wassert(actual(q.coords_max.lon) == -50000);
        do_set(DBA_KEY_LATMIN, "4420000");
        wassert(actual(q.coords_min.lat) == 4420000);
        do_set(DBA_KEY_LONMIN, "1120000");
        wassert(actual(q.coords_min.lon) == 1120000);

        do_set(DBA_KEY_YEAR, "2000");
        wassert(actual(q.datetime_exact.date.year) == 2000);
        do_set(DBA_KEY_MONTH, "1");
        wassert(actual(q.datetime_exact.date.month) == 1);
        do_set(DBA_KEY_DAY, "2");
        wassert(actual(q.datetime_exact.date.day) == 2);
        do_set(DBA_KEY_HOUR, "12");
        wassert(actual(q.datetime_exact.time.hour) == 12);
        do_set(DBA_KEY_MIN, "30");
        wassert(actual(q.datetime_exact.time.minute) == 30);
        do_set(DBA_KEY_SEC, "45");
        wassert(actual(q.datetime_exact.time.second) == 45);

        do_set(DBA_KEY_YEARMIN, "2001");
        wassert(actual(q.datetime_min.date.year) == 2001);
        do_set(DBA_KEY_MONTHMIN, "2");
        wassert(actual(q.datetime_min.date.month) == 2);
        do_set(DBA_KEY_DAYMIN, "3");
        wassert(actual(q.datetime_min.date.day) == 3);
        do_set(DBA_KEY_HOURMIN, "13");
        wassert(actual(q.datetime_min.time.hour) == 13);
        do_set(DBA_KEY_MINUMIN, "31");
        wassert(actual(q.datetime_min.time.minute) == 31);
        do_set(DBA_KEY_SECMIN, "46");
        wassert(actual(q.datetime_min.time.second) == 46);

        do_set(DBA_KEY_YEARMAX, "2002");
        wassert(actual(q.datetime_max.date.year) == 2002);
        do_set(DBA_KEY_MONTHMAX, "3");
        wassert(actual(q.datetime_max.date.month) == 3);
        do_set(DBA_KEY_DAYMAX, "4");
        wassert(actual(q.datetime_max.date.day) == 4);
        do_set(DBA_KEY_HOURMAX, "14");
        wassert(actual(q.datetime_max.time.hour) == 14);
        do_set(DBA_KEY_MINUMAX, "32");
        wassert(actual(q.datetime_max.time.minute) == 32);
        do_set(DBA_KEY_SECMAX, "47");
        wassert(actual(q.datetime_max.time.second) == 47);

        do_set(DBA_KEY_LEVELTYPE1, "10");
        wassert(actual(q.level.ltype1) == 10);
        do_set(DBA_KEY_L1, "11");
        wassert(actual(q.level.l1) == 11);
        do_set(DBA_KEY_LEVELTYPE2, "12");
        wassert(actual(q.level.ltype2) == 12);
        do_set(DBA_KEY_L2, "13");
        wassert(actual(q.level.l2) == 13);

        do_set(DBA_KEY_PINDICATOR, "20");
        wassert(actual(q.trange.pind) == 20);
        do_set(DBA_KEY_P1, "21");
        wassert(actual(q.trange.p1) == 21);
        do_set(DBA_KEY_P2, "22");
        wassert(actual(q.trange.p2) == 22);

        do_set(DBA_KEY_LIMIT, "100");
        wassert(actual(q.limit) == 100);

        do_set(DBA_KEY_VAR, "B12101");
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));

        do_set(DBA_KEY_VARLIST, "B12102,v");
        wassert(actual(q.varcodes.size()) == 2);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 11, 4));
        wassert(actual(*q.varcodes.rbegin()) == WR_VAR(0, 12, 102));

        do_set(DBA_KEY_QUERY, "best");
        wassert(actual(q.query) == "best");
        do_set(DBA_KEY_ANA_FILTER, "B01001=1");
        wassert(actual(q.ana_filter) == "B01001=1");
        do_set(DBA_KEY_DATA_FILTER, "B12101>260");
        wassert(actual(q.data_filter) == "B12101>260");
        do_set(DBA_KEY_ATTR_FILTER, "B33007>50");
        wassert(actual(q.attr_filter) == "B33007>50");
    }),
};

test_group newtg("core_query", tests);

}
