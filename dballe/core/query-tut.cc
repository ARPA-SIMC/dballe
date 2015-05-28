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
void test_set_prio(WIBBLE_TEST_LOCPRM, core::Query& q, T exact1, T min2, T max3)
{
    q.set("priority", exact1);
    wassert(actual(q.prio_min) == 1);
    wassert(actual(q.prio_max) == 1);
    q.set("priomin", min2);
    wassert(actual(q.prio_min) == 2);
    wassert(actual(q.prio_max) == 1);
    q.set("priomax", max3);
    wassert(actual(q.prio_min) == 2);
    wassert(actual(q.prio_max) == 3);
}

template<typename T>
void test_set_lat(WIBBLE_TEST_LOCPRM, core::Query& q, T lat, T latmin, T latmax)
{
    q.set("lat", lat);
    wassert(actual(q.coords_min.lat) == 4412300);
    wassert(actual(q.coords_max.lat) == 4412300);
    q.set("latmin", latmin);
    wassert(actual(q.coords_min.lat) == 4410000);
    wassert(actual(q.coords_max.lat) == 4412300);
    q.set("latmax", latmax);
    wassert(actual(q.coords_min.lat) == 4410000);
    wassert(actual(q.coords_max.lat) == 4420000);
}

template<typename T>
void test_set_lon(WIBBLE_TEST_LOCPRM, core::Query& q, T lon, T lonmin, T lonmax)
{
    q.set("lon", lon);
    wassert(actual(q.coords_min.lon) == -1112300);
    wassert(actual(q.coords_max.lon) == -1112300);
    q.set("lonmin", lonmin);
    wassert(actual(q.coords_min.lon) == -50000);
    wassert(actual(q.coords_max.lon) == -1112300);
    q.set("lonmax", lonmax);
    wassert(actual(q.coords_min.lon) == -50000);
    wassert(actual(q.coords_max.lon) == 1120000);
}

template<typename T>
void test_set_datetime(WIBBLE_TEST_LOCPRM, core::Query& q,
        T ye, T mo, T da, T ho, T mi, T se,
        T yemin, T momin, T damin, T homin, T mimin, T semin,
        T yemax, T momax, T damax, T homax, T mimax, T semax)
{
    q.set("year", ye);
    wassert(actual((unsigned)q.datetime_min.year) == 2000);
    wassert(actual((unsigned)q.datetime_max.year) == 2000);
    q.set("yearmin", yemin);
    wassert(actual((unsigned)q.datetime_min.year) == 2001);
    wassert(actual((unsigned)q.datetime_max.year) == 2000);
    q.set("yearmax", yemax);
    wassert(actual((unsigned)q.datetime_min.year) == 2001);
    wassert(actual((unsigned)q.datetime_max.year) == 2002);
    q.set("month", mo);
    wassert(actual((unsigned)q.datetime_min.month) == 1);
    wassert(actual((unsigned)q.datetime_max.month) == 1);
    q.set("monthmin", momin);
    wassert(actual((unsigned)q.datetime_min.month) == 2);
    wassert(actual((unsigned)q.datetime_max.month) == 1);
    q.set("monthmax", momax);
    wassert(actual((unsigned)q.datetime_min.month) == 2);
    wassert(actual((unsigned)q.datetime_max.month) == 3);
    q.set("day", da);
    wassert(actual((unsigned)q.datetime_min.day) == 2);
    wassert(actual((unsigned)q.datetime_max.day) == 2);
    q.set("daymin", damin);
    wassert(actual((unsigned)q.datetime_min.day) == 3);
    wassert(actual((unsigned)q.datetime_max.day) == 2);
    q.set("daymax", damax);
    wassert(actual((unsigned)q.datetime_min.day) == 3);
    wassert(actual((unsigned)q.datetime_max.day) == 4);
    q.set("hour", ho);
    wassert(actual((unsigned)q.datetime_min.hour) == 12);
    wassert(actual((unsigned)q.datetime_max.hour) == 12);
    q.set("hourmin", homin);
    wassert(actual((unsigned)q.datetime_min.hour) == 13);
    wassert(actual((unsigned)q.datetime_max.hour) == 12);
    q.set("hourmax", homax);
    wassert(actual((unsigned)q.datetime_min.hour) == 13);
    wassert(actual((unsigned)q.datetime_max.hour) == 14);
    q.set("min", mi);
    wassert(actual((unsigned)q.datetime_min.minute) == 30);
    wassert(actual((unsigned)q.datetime_max.minute) == 30);
    q.set("minumin", mimin);
    wassert(actual((unsigned)q.datetime_min.minute) == 31);
    wassert(actual((unsigned)q.datetime_max.minute) == 30);
    q.set("minumax", mimax);
    wassert(actual((unsigned)q.datetime_min.minute) == 31);
    wassert(actual((unsigned)q.datetime_max.minute) == 32);
    q.set("sec", se);
    wassert(actual((unsigned)q.datetime_min.second) == 45);
    wassert(actual((unsigned)q.datetime_max.second) == 45);
    q.set("secmin", semin);
    wassert(actual((unsigned)q.datetime_min.second) == 46);
    wassert(actual((unsigned)q.datetime_max.second) == 45);
    q.set("secmax", semax);
    wassert(actual((unsigned)q.datetime_min.second) == 46);
    wassert(actual((unsigned)q.datetime_max.second) == 47);
}


std::vector<Test> tests {
    Test("seti", [](Fixture& f) {
        core::Query q;

        wruntest(test_set_prio, q, 1, 2, 3);

        q.seti_keyword(DBA_KEY_ANA_ID, 4);
        wassert(actual(q.ana_id) == 4);
        q.seti_keyword(DBA_KEY_MOBILE, 0);
        wassert(actual(q.mobile) == 0);

        wruntest(test_set_lat, q, 4412300, 4410000, 4420000);
        wruntest(test_set_lon, q, -1112300, 35950000, 1120000);
        wruntest(test_set_datetime, q,
                2000, 1, 2, 12, 30, 45,
                2001, 2, 3, 13, 31, 46,
                2002, 3, 4, 14, 32, 47);

        q.seti_keyword(DBA_KEY_LEVELTYPE1, 10);
        wassert(actual(q.level.ltype1) == 10);
        q.seti_keyword(DBA_KEY_L1, 11);
        wassert(actual(q.level.l1) == 11);
        q.seti_keyword(DBA_KEY_LEVELTYPE2, 12);
        wassert(actual(q.level.ltype2) == 12);
        q.seti_keyword(DBA_KEY_L2, 13);
        wassert(actual(q.level.l2) == 13);

        q.seti_keyword(DBA_KEY_PINDICATOR, 20);
        wassert(actual(q.trange.pind) == 20);
        q.seti_keyword(DBA_KEY_P1, 21);
        wassert(actual(q.trange.p1) == 21);
        q.seti_keyword(DBA_KEY_P2, 22);
        wassert(actual(q.trange.p2) == 22);

        q.seti_keyword(DBA_KEY_VAR, WR_VAR(0, 12, 101));
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));

        q.seti_keyword(DBA_KEY_VARLIST, WR_VAR(0, 12, 102));
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 102));

        q.seti_keyword(DBA_KEY_CONTEXT_ID, 42);
        wassert(actual(q.data_id) == 42);

        q.seti_keyword(DBA_KEY_LIMIT, 100);
        wassert(actual(q.limit) == 100);

        wassert(actual(q.has_ident).isfalse());
    }),

    Test("setd", [](Fixture& f) {
        core::Query q;

        wruntest(test_set_prio, q, 1.0, 1.5, 3.4);

        q.setd_keyword(DBA_KEY_ANA_ID, 4.0);
        wassert(actual(q.ana_id) == 4);
        q.setd_keyword(DBA_KEY_MOBILE, 0.0);
        wassert(actual(q.mobile) == 0);

        wruntest(test_set_lat, q, 44.123, 44.10, 44.20);
        wruntest(test_set_lon, q, -11.123, 359.50, 11.20);
        wruntest(test_set_datetime, q,
                2000.0, 1.0, 2.0, 12.0, 30.0, 45.0,
                2001.0, 2.0, 3.0, 13.0, 31.0, 46.0,
                2002.0, 3.0, 4.0, 14.0, 32.0, 47.0);

        q.setd_keyword(DBA_KEY_LEVELTYPE1, 10.0);
        wassert(actual(q.level.ltype1) == 10);
        q.setd_keyword(DBA_KEY_L1, 11.0);
        wassert(actual(q.level.l1) == 11);
        q.setd_keyword(DBA_KEY_LEVELTYPE2, 12.0);
        wassert(actual(q.level.ltype2) == 12);
        q.setd_keyword(DBA_KEY_L2, 13.0);
        wassert(actual(q.level.l2) == 13);

        q.setd_keyword(DBA_KEY_PINDICATOR, 20.0);
        wassert(actual(q.trange.pind) == 20);
        q.setd_keyword(DBA_KEY_P1, 21.0);
        wassert(actual(q.trange.p1) == 21);
        q.setd_keyword(DBA_KEY_P2, 22.0);
        wassert(actual(q.trange.p2) == 22);

        q.setd_keyword(DBA_KEY_CONTEXT_ID, 42.0);
        wassert(actual(q.data_id) == 42);

        q.setd_keyword(DBA_KEY_LIMIT, 100.0);
        wassert(actual(q.limit) == 100);

        wassert(actual(q.has_ident).isfalse());
    }),

    Test("setc_cstring", [](Fixture& f) {
        core::Query q;

        wruntest(test_set_prio, q, "1", "2", "3");

        q.setc_keyword(DBA_KEY_REP_MEMO, "synop");
        wassert(actual(q.rep_memo) == "synop");
        q.setc_keyword(DBA_KEY_ANA_ID, "4");
        wassert(actual(q.ana_id) == 4);
        q.setc_keyword(DBA_KEY_MOBILE, "1");
        wassert(actual(q.mobile) == 1);

        wassert(actual(q.has_ident).isfalse());
        q.setc_keyword(DBA_KEY_IDENT, "spc");
        wassert(actual(q.ident) == "spc");
        wassert(actual(q.has_ident).istrue());

        wruntest(test_set_lat, q, "4412300", "4410000", "4420000");
        wruntest(test_set_lon, q, "-1112300", "35950000", "1120000");
        wruntest(test_set_datetime, q,
                "2000", "1", "2", "12", "30", "45",
                "2001", "2", "3", "13", "31", "46",
                "2002", "3", "4", "14", "32", "47");

        q.setc_keyword(DBA_KEY_LEVELTYPE1, "10");
        wassert(actual(q.level.ltype1) == 10);
        q.setc_keyword(DBA_KEY_L1, "11");
        wassert(actual(q.level.l1) == 11);
        q.setc_keyword(DBA_KEY_LEVELTYPE2, "12");
        wassert(actual(q.level.ltype2) == 12);
        q.setc_keyword(DBA_KEY_L2, "13");
        wassert(actual(q.level.l2) == 13);

        q.setc_keyword(DBA_KEY_PINDICATOR, "20");
        wassert(actual(q.trange.pind) == 20);
        q.setc_keyword(DBA_KEY_P1, "21");
        wassert(actual(q.trange.p1) == 21);
        q.setc_keyword(DBA_KEY_P2, "22");
        wassert(actual(q.trange.p2) == 22);

        q.setc_keyword(DBA_KEY_LIMIT, "100");
        wassert(actual(q.limit) == 100);

        q.setc_keyword(DBA_KEY_CONTEXT_ID, "42");
        wassert(actual(q.data_id) == 42);

        q.setc_keyword(DBA_KEY_VAR, "B12101");
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));

        q.setc_keyword(DBA_KEY_VARLIST, "B12102,v");
        wassert(actual(q.varcodes.size()) == 2);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 11, 4));
        wassert(actual(*q.varcodes.rbegin()) == WR_VAR(0, 12, 102));

        q.setc_keyword(DBA_KEY_QUERY, "best");
        wassert(actual(q.query) == "best");
        q.setc_keyword(DBA_KEY_ANA_FILTER, "B01001=1");
        wassert(actual(q.ana_filter) == "B01001=1");
        q.setc_keyword(DBA_KEY_DATA_FILTER, "B12101>260");
        wassert(actual(q.data_filter) == "B12101>260");
        q.setc_keyword(DBA_KEY_ATTR_FILTER, "B33007>50");
        wassert(actual(q.attr_filter) == "B33007>50");
    }),
    Test("setc_string", [](Fixture& f) {
        core::Query q;

        // Force setting to a string
        auto do_set = [&](dba_keyword key, const std::string& s) {
            q.sets_keyword(key, s);
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
        wassert(actual(core::Query::parse_modifiers("best")) == DBA_DB_MODIFIER_BEST);
        wassert(actual(core::Query::parse_modifiers("details")) == DBA_DB_MODIFIER_SUMMARY_DETAILS);
    }),
};

test_group newtg("core_query", tests);

}
