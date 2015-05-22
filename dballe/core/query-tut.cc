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
using namespace wreport;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

template<typename T>
void test_set_prio(WIBBLE_TEST_LOCPRM, Query& q, T exact1, T min2, T max3)
{
    q.set(DBA_KEY_PRIORITY, exact1);
    wassert(actual(q.prio_min) == 1);
    wassert(actual(q.prio_max) == 1);
    q.set(DBA_KEY_PRIOMIN, min2);
    wassert(actual(q.prio_min) == 2);
    wassert(actual(q.prio_max) == 1);
    q.set(DBA_KEY_PRIOMAX, max3);
    wassert(actual(q.prio_min) == 2);
    wassert(actual(q.prio_max) == 3);
}

template<typename T>
void test_set_lat(WIBBLE_TEST_LOCPRM, Query& q, T lat, T latmin, T latmax)
{
    q.set(DBA_KEY_LAT, lat);
    wassert(actual(q.coords_min.lat) == 4412300);
    wassert(actual(q.coords_max.lat) == 4412300);
    q.set(DBA_KEY_LATMIN, latmin);
    wassert(actual(q.coords_min.lat) == 4410000);
    wassert(actual(q.coords_max.lat) == 4412300);
    q.set(DBA_KEY_LATMAX, latmax);
    wassert(actual(q.coords_min.lat) == 4410000);
    wassert(actual(q.coords_max.lat) == 4420000);
}

template<typename T>
void test_set_lon(WIBBLE_TEST_LOCPRM, Query& q, T lon, T lonmin, T lonmax)
{
    q.set(DBA_KEY_LON, lon);
    wassert(actual(q.coords_min.lon) == -1112300);
    wassert(actual(q.coords_max.lon) == -1112300);
    q.set(DBA_KEY_LONMIN, lonmin);
    wassert(actual(q.coords_min.lon) == -50000);
    wassert(actual(q.coords_max.lon) == -1112300);
    q.set(DBA_KEY_LONMAX, lonmax);
    wassert(actual(q.coords_min.lon) == -50000);
    wassert(actual(q.coords_max.lon) == 1120000);
}

template<typename T>
void test_set_datetime(WIBBLE_TEST_LOCPRM, Query& q,
        T ye, T mo, T da, T ho, T mi, T se,
        T yemin, T momin, T damin, T homin, T mimin, T semin,
        T yemax, T momax, T damax, T homax, T mimax, T semax)
{
    q.set(DBA_KEY_YEAR, ye);
    wassert(actual((unsigned)q.datetime_min.date.year) == 2000);
    wassert(actual((unsigned)q.datetime_max.date.year) == 2000);
    q.set(DBA_KEY_YEARMIN, yemin);
    wassert(actual((unsigned)q.datetime_min.date.year) == 2001);
    wassert(actual((unsigned)q.datetime_max.date.year) == 2000);
    q.set(DBA_KEY_YEARMAX, yemax);
    wassert(actual((unsigned)q.datetime_min.date.year) == 2001);
    wassert(actual((unsigned)q.datetime_max.date.year) == 2002);
    q.set(DBA_KEY_MONTH, mo);
    wassert(actual((unsigned)q.datetime_min.date.month) == 1);
    wassert(actual((unsigned)q.datetime_max.date.month) == 1);
    q.set(DBA_KEY_MONTHMIN, momin);
    wassert(actual((unsigned)q.datetime_min.date.month) == 2);
    wassert(actual((unsigned)q.datetime_max.date.month) == 1);
    q.set(DBA_KEY_MONTHMAX, momax);
    wassert(actual((unsigned)q.datetime_min.date.month) == 2);
    wassert(actual((unsigned)q.datetime_max.date.month) == 3);
    q.set(DBA_KEY_DAY, da);
    wassert(actual((unsigned)q.datetime_min.date.day) == 2);
    wassert(actual((unsigned)q.datetime_max.date.day) == 2);
    q.set(DBA_KEY_DAYMIN, damin);
    wassert(actual((unsigned)q.datetime_min.date.day) == 3);
    wassert(actual((unsigned)q.datetime_max.date.day) == 2);
    q.set(DBA_KEY_DAYMAX, damax);
    wassert(actual((unsigned)q.datetime_min.date.day) == 3);
    wassert(actual((unsigned)q.datetime_max.date.day) == 4);
    q.set(DBA_KEY_HOUR, ho);
    wassert(actual((unsigned)q.datetime_min.time.hour) == 12);
    wassert(actual((unsigned)q.datetime_max.time.hour) == 12);
    q.set(DBA_KEY_HOURMIN, homin);
    wassert(actual((unsigned)q.datetime_min.time.hour) == 13);
    wassert(actual((unsigned)q.datetime_max.time.hour) == 12);
    q.set(DBA_KEY_HOURMAX, homax);
    wassert(actual((unsigned)q.datetime_min.time.hour) == 13);
    wassert(actual((unsigned)q.datetime_max.time.hour) == 14);
    q.set(DBA_KEY_MIN, mi);
    wassert(actual((unsigned)q.datetime_min.time.minute) == 30);
    wassert(actual((unsigned)q.datetime_max.time.minute) == 30);
    q.set(DBA_KEY_MINUMIN, mimin);
    wassert(actual((unsigned)q.datetime_min.time.minute) == 31);
    wassert(actual((unsigned)q.datetime_max.time.minute) == 30);
    q.set(DBA_KEY_MINUMAX, mimax);
    wassert(actual((unsigned)q.datetime_min.time.minute) == 31);
    wassert(actual((unsigned)q.datetime_max.time.minute) == 32);
    q.set(DBA_KEY_SEC, se);
    wassert(actual((unsigned)q.datetime_min.time.second) == 45);
    wassert(actual((unsigned)q.datetime_max.time.second) == 45);
    q.set(DBA_KEY_SECMIN, semin);
    wassert(actual((unsigned)q.datetime_min.time.second) == 46);
    wassert(actual((unsigned)q.datetime_max.time.second) == 45);
    q.set(DBA_KEY_SECMAX, semax);
    wassert(actual((unsigned)q.datetime_min.time.second) == 46);
    wassert(actual((unsigned)q.datetime_max.time.second) == 47);
}


std::vector<Test> tests {
    Test("seti", [](Fixture& f) {
        Query q;

        wruntest(test_set_prio, q, 1, 2, 3);

        q.seti(DBA_KEY_ANA_ID, 4);
        wassert(actual(q.ana_id) == 4);
        q.seti(DBA_KEY_MOBILE, 0);
        wassert(actual(q.mobile) == 0);

        wruntest(test_set_lat, q, 4412300, 4410000, 4420000);
        wruntest(test_set_lon, q, -1112300, 35950000, 1120000);
        wruntest(test_set_datetime, q,
                2000, 1, 2, 12, 30, 45,
                2001, 2, 3, 13, 31, 46,
                2002, 3, 4, 14, 32, 47);

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

        q.seti(DBA_KEY_CONTEXT_ID, 42);
        wassert(actual(q.data_id) == 42);

        q.seti(DBA_KEY_LIMIT, 100);
        wassert(actual(q.limit) == 100);

        wassert(actual(q.has_ident).isfalse());
    }),

    Test("setd", [](Fixture& f) {
        Query q;

        wruntest(test_set_prio, q, 1.0, 1.5, 3.4);

        q.setd(DBA_KEY_ANA_ID, 4.0);
        wassert(actual(q.ana_id) == 4);
        q.setd(DBA_KEY_MOBILE, 0.0);
        wassert(actual(q.mobile) == 0);

        wruntest(test_set_lat, q, 44.123, 44.10, 44.20);
        wruntest(test_set_lon, q, -11.123, 359.50, 11.20);
        wruntest(test_set_datetime, q,
                2000.0, 1.0, 2.0, 12.0, 30.0, 45.0,
                2001.0, 2.0, 3.0, 13.0, 31.0, 46.0,
                2002.0, 3.0, 4.0, 14.0, 32.0, 47.0);

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

        q.setd(DBA_KEY_CONTEXT_ID, 42.0);
        wassert(actual(q.data_id) == 42);

        q.setd(DBA_KEY_LIMIT, 100.0);
        wassert(actual(q.limit) == 100);

        wassert(actual(q.has_ident).isfalse());
    }),

    Test("setc_cstring", [](Fixture& f) {
        Query q;

        wruntest(test_set_prio, q, "1", "2", "3");

        q.setc(DBA_KEY_REP_MEMO, "synop");
        wassert(actual(q.rep_memo) == "synop");
        q.setc(DBA_KEY_ANA_ID, "4");
        wassert(actual(q.ana_id) == 4);
        q.setc(DBA_KEY_MOBILE, "1");
        wassert(actual(q.mobile) == 1);

        wassert(actual(q.has_ident).isfalse());
        q.setc(DBA_KEY_IDENT, "spc");
        wassert(actual(q.ident) == "spc");
        wassert(actual(q.has_ident).istrue());

        wruntest(test_set_lat, q, "4412300", "4410000", "4420000");
        wruntest(test_set_lon, q, "-1112300", "35950000", "1120000");
        wruntest(test_set_datetime, q,
                "2000", "1", "2", "12", "30", "45",
                "2001", "2", "3", "13", "31", "46",
                "2002", "3", "4", "14", "32", "47");

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

        q.setc(DBA_KEY_CONTEXT_ID, "42");
        wassert(actual(q.data_id) == 42);

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

        wruntest(test_set_prio, q, string("1"), string("2"), string("3"));

        do_set(DBA_KEY_REP_MEMO, "synop");
        wassert(actual(q.rep_memo) == "synop");
        do_set(DBA_KEY_ANA_ID, "4");
        wassert(actual(q.ana_id) == 4);
        do_set(DBA_KEY_MOBILE, "1");
        wassert(actual(q.mobile) == 1);

        wassert(actual(q.has_ident).isfalse());
        do_set(DBA_KEY_IDENT, "spc");
        wassert(actual(q.ident) == "spc");
        wassert(actual(q.has_ident).istrue());

        wruntest(test_set_lat, q, string("4412300"), string("4410000"), string("4420000"));
        wruntest(test_set_lon, q, string("-1112300"), string("35950000"), string("1120000"));
        wruntest(test_set_datetime, q,
                string("2000"), string("1"), string("2"), string("12"), string("30"), string("45"),
                string("2001"), string("2"), string("3"), string("13"), string("31"), string("46"),
                string("2002"), string("3"), string("4"), string("14"), string("32"), string("47"));

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

        do_set(DBA_KEY_CONTEXT_ID, "42");
        wassert(actual(q.data_id) == 42);

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
    Test("modifiers", [](Fixture& f) {
        wassert(actual(Query::parse_modifiers("best")) == DBA_DB_MODIFIER_BEST);
        wassert(actual(Query::parse_modifiers("details")) == DBA_DB_MODIFIER_SUMMARY_DETAILS);
    }),
    Test("to_vars", [](Fixture& f) {
        auto to_vars = [](const std::string& test_string) -> std::string {
            std::string res;
            Query q;
            q.set_from_test_string(test_string);
            q.to_vars([&](dba_keyword key, unique_ptr<Var> var) {
                if (!res.empty()) res += ", ";
                res += Record::keyword_name(key);
                res += "=";
                res += var->format("");
            });
            return res;
        };

        wassert(actual(to_vars("")) == "");
        wassert(actual(to_vars("latmin=45.0")) == "latmin=45.00000");
        wassert(actual(to_vars("latmin=45.0, latmax=46.0")) == "latmin=45.00000, latmax=46.00000");
        wassert(actual(to_vars("latmin=45.0, latmax=46.0, lonmin=11.0")) == "latmin=45.00000, latmax=46.00000, lonmin=11.00000");
        wassert(actual(to_vars("latmin=45.0, latmax=46.0, lonmin=11.0, lonmax=11.5")) == "latmin=45.00000, latmax=46.00000, lonmin=11.00000, lonmax=11.50000");

        
    }),
};

test_group newtg("core_query", tests);

}
