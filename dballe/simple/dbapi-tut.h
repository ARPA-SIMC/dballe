/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dbapi.h"

namespace dballe {
namespace tests {

struct dbapi_tests : public dballe::tests::DB_test_base
{
    dbapi_tests(dballe::db::Format format) : dballe::tests::DB_test_base(format) {}

    void populate_variables(fortran::DbAPI& api)
    {
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.settimerange(254, 0, 0);
        api.setdate(2013, 4, 25, 12, 0, 0);

        // Instant temperature, 2 meters above ground
        api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
        api.setd("B12101", 21.5);
        api.prendilo();

        // Instant wind speed, 10 meters above ground
        api.unsetb();
        api.setlevel(103, 10000, MISSING_INT, MISSING_INT);
        api.setd("B11002", 2.4);
        api.prendilo();

        api.unsetall();
    }

    void test_vars();
    void test_attrs();
    void test_attrs_prendilo();
    void test_prendilo_anaid();
};

}
}

