/*
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "test-utils-core.h"
#include "json.h"

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("null", [](Fixture& f) {
        // null value
        string out;
        JSONWriter writer(out);
        writer.add_null();
        wassert(actual(out) == "null");
    }),
    Test("bool", [](Fixture& f) {
        // bool value
        string out;
        JSONWriter writer(out);

        writer.add(true);
        wassert(actual(out) == "true");

        out.clear();
        writer.add(false);
        wassert(actual(out) == "false");
    }),
    Test("int", [](Fixture& f) {
        // int value
        string out;
        JSONWriter writer(out);

        writer.add(1);
        wassert(actual(out) == "1");

        out.clear();
        writer.add(-1234567);
        wassert(actual(out) == "-1234567");
    }),
    Test("double", [](Fixture& f) {
        // double value
        string out;
        JSONWriter writer(out);
        writer.add(1.1);
        wassert(actual(out) == "1.100000");

        out.clear();
        writer.add(-1.1);
        wassert(actual(out) == "-1.100000");

        out.clear();
        writer.add(1.0);
        wassert(actual(out) == "1.0");

        out.clear();
        writer.add(-1.0);
        wassert(actual(out) == "-1.0");
    }),
    Test("string", [](Fixture& f) {
        // string value
        string out;
        JSONWriter writer(out);
        writer.add("");
        wassert(actual(out) == "\"\"");

        out.clear();
        writer.add("antani");
        wassert(actual(out) == "\"antani\"");

        out.clear();
        writer.add("\n");
        wassert(actual(out) == "\"\\n\"");
    }),
    Test("list", [](Fixture& f) {
        // list value
        string out;
        JSONWriter writer(out);
        writer.start_list();
        writer.add("");
        writer.add(1);
        writer.add(1.0);
        writer.end_list();
        wassert(actual(out) == "[\"\",1,1.0]");
    }),
    Test("mapping", [](Fixture& f) {
        // mapping value
        string out;
        JSONWriter writer(out);
        writer.start_mapping();
        writer.add("", 1);
        writer.add("antani", 1.0);
        writer.end_mapping();
        wassert(actual(out) == "{\"\":1,\"antani\":1.0}");
    }),
};

test_group newtg("core_json", tests);

}
