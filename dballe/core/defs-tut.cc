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

#include "core/test-utils-core.h"
#include "core/defs.h"

using namespace dballe;
using namespace wibble::tests;
using namespace std;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("coords", [](Fixture& f) {
        wassert(actual(Coords(44.0, 11.0)) == Coords(44.0, 360.0+11.0));
    }),
    Test("ident", [](Fixture& f) {
        wassert(actual(Ident()) == Ident());
        wassert(actual(Ident("foo")) == Ident("foo"));
        wassert(actual(Ident().is_missing()).istrue());
        wassert(actual(Ident("foo").is_missing()).isfalse());
        Ident test;
        wassert(actual((const char*)test == nullptr).istrue());
        test = "antani";
        wassert(actual((const char*)test) == "antani");
        Ident test1 = test;
        wassert(actual(test) == Ident("antani"));
        wassert(actual(test1) == Ident("antani"));
        test1 = test1;
        wassert(actual(test) == Ident("antani"));
        wassert(actual(test1) == Ident("antani"));
        test1 = Ident("blinda");
        wassert(actual(test) == Ident("antani"));
        wassert(actual(test1) == Ident("blinda"));
        test = move(test1);
        wassert(actual(test) == Ident("blinda"));
        wassert(actual(test1.is_missing()).istrue());
        Ident test2(move(test));
        wassert(actual(test.is_missing()).istrue());
        wassert(actual(test2) == Ident("blinda"));
        test = string("foo");
        wassert(actual(test) == Ident("foo"));
        wassert(actual(test2) == Ident("blinda"));
        wassert(actual(Ident("foo") == Ident("foo")).istrue());
        wassert(actual(Ident("foo") <= Ident("foo")).istrue());
        wassert(actual(Ident("foo") >= Ident("foo")).istrue());
        wassert(actual(Ident("foo") == Ident("bar")).isfalse());
        wassert(actual(Ident("foo") != Ident("foo")).isfalse());
        wassert(actual(Ident("foo") != Ident("bar")).istrue());
        wassert(actual(Ident("antani") <  Ident("blinda")).istrue());
        wassert(actual(Ident("antani") <= Ident("blinda")).istrue());
        wassert(actual(Ident("antani") >  Ident("blinda")).isfalse());
        wassert(actual(Ident("antani") >= Ident("blinda")).isfalse());
        wassert(actual(Ident("blinda") <  Ident("antani")).isfalse());
        wassert(actual(Ident("blinda") <= Ident("antani")).isfalse());
        wassert(actual(Ident("blinda") >  Ident("antani")).istrue());
        wassert(actual(Ident("blinda") >= Ident("antani")).istrue());
    }),
};

test_group newtg("dballe_core_defs", tests);

}
