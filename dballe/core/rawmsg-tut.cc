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
#include "core/rawmsg.h"

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("empty", [](Fixture& f) {
        Rawmsg msg;

        wassert(actual(msg.file.empty()).istrue());
        wassert(actual(msg.offset) == 0);
        wassert(actual(msg.index) == 0);
        wassert(actual(msg.size()) == 0);

        /* Resetting an empty message should do anything special */
        msg.clear();
        wassert(actual(msg.file.empty()).istrue());
        wassert(actual(msg.offset) == 0);
        wassert(actual(msg.index) == 0);
        wassert(actual(msg.size()) == 0u);
    }),
};

test_group newtg("core_rawmsg", tests);

}

/* vim:set ts=4 sw=4: */
