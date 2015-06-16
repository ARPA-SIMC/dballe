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

#include "db/tests.h"
#include "db/querybuf.h"

using namespace wibble::tests;
using namespace wreport;
using namespace dballe;
using namespace std;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("append", [](Fixture& f) {
        Querybuf buf(10);

        // A new querybuf contains the empty string
        ensure_equals(buf, string());

        // Clearing should still be empty
        buf.clear();
        ensure_equals(buf, string());

        buf.append("ciao");
        ensure_equals(buf, "ciao");
        ensure_equals(buf.size(), 4);

        buf.appendf("%d %s", 42, "--");
        ensure_equals(buf, "ciao42 --");
        ensure_equals(buf.size(), 9);

        buf.clear();
        ensure_equals(buf, string());

        buf.append("123456789");
        ensure_equals(buf, "123456789");

        buf.clear();
        buf.start_list(", ");
        buf.append_list("1");
        buf.append_list("2");
        buf.append_listf("%d", 3);
        ensure_equals(buf, "1, 2, 3");
    }),
    Test("varlist", [](Fixture& f) {
        Querybuf buf(50);
        buf.append_varlist("B12101,B12103,block");
        wassert(actual((string)buf) == "3173,3175,257");
    }),
};

test_group tg("db_querybuf", tests);

}
