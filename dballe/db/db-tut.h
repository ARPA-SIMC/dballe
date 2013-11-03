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

namespace dballe {
namespace tests {

struct db_tests : public dballe::tests::DB_test_base
{
    void test_simple_insert();
    void test_insert();
    void test_double_station_insert();
    void test_ana_query();
    void test_misc_queries();
    void test_querybest();
    void test_deletion();
    void test_datetime_queries();
    void test_qc();
    void test_ana_queries();
    void test_vacuum();
    void test_attrs();
    void test_wrap_longitude();
    void test_ana_filter();
    void test_datetime_extremes();
    void test_invalid_sql_querybest();
    void test_bug_querybest();
    void test_bug_query_stations_by_level();
    void test_bug_query_levels_by_station();
    void test_connect_leaks();
    void test_query_stations();
    void test_summary_queries();
    void test_value_update();
    void test_query_step_by_step();
    void test_double_stationinfo_insert();
    void test_double_stationinfo_insert1();
    void test_insert_undef_lev2();
    void test_query_undef_lev2();
    void test_query_bad_attr_filter();
    void test_querybest_priomax();
    void test_repmemo_in_output();

    dballe::tests::TestStation st1;
    dballe::tests::TestStation st2;

    db_tests(dballe::db::Format format) : dballe::tests::DB_test_base(format)
    {
        st1.lat = 12.34560;
        st1.lon = 76.54320;
        st1.info["synop"].set("block", 1);
        st1.info["synop"].set("station", 2);
        st1.info["synop"].set("B07030", 42.0); // height
        st1.info["metar"].set("B07030", 50.0); // height

        st2.lat = 23.45670;
        st2.lon = 65.43210;
        st2.info["temp"].set("block", 3);
        st2.info["temp"].set("station", 4);
        st2.info["temp"].set("B07030", 100.0); // height
        st2.info["metar"].set("B07030", 110.0); // height
    }
};

}
}

/* vim:set ts=4 sw=4: */

