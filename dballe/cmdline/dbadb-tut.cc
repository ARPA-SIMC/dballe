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
#include "cmdline/dbadb.h"
#include "core/arrayfile.h"
#include "msg/codec.h"
#include "config.h"

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

typedef dballe::tests::DBFixture Fixture;
typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    // Test simple queries
    Test("utils", [](Fixture& f) {
        // Resolve NULL and the empty string
        ensure_equals(dbadb::parse_op_report(*f.db, NULL), (const char*)NULL);
        ensure_equals(dbadb::parse_op_report(*f.db, ""), (const char*)NULL);

        // Resolve from names
        wassert(actual(dbadb::parse_op_report(*f.db, "synop")) == "synop");

        // Resolve from nonexisting numbers
        try {
            dbadb::parse_op_report(*f.db, "11234");
            ensure(false);
        } catch (...) {}

        // Resolve from nonexisting names
        try {
            dbadb::parse_op_report(*f.db, "lolcats");
            ensure(false);
        } catch (...) {}
    }),
    Test("import", [](Fixture& f) {
        Dbadb dbadb(*f.db);

        // Import a synop
        cmdline::Reader reader;
        ensure_equals(dbadb.do_import(dballe::tests::datafile("bufr/obs0-1.22.bufr"), reader), 0);

        // Export forcing report as temp
        core::Query query;
        ArrayFile file(BUFR);
        ensure_equals(dbadb.do_export(query, file, "generic", "ship"), 0);

        ensure_equals(file.msgs.size(), 1u);

        // Decode results
        auto importer = msg::Importer::create(BUFR);
        Msgs msgs;
        importer->from_rawmsg(file.msgs[0], msgs);
        ensure_equals(msgs.size(), 1u);

        // Ensure they're ships
        ensure_equals(msgs[0]->type, MSG_SHIP);

        // Check 001194 [SIM] Report mnemonic(CCITTIA5), too
        const Var* var = msgs[0]->get_rep_memo_var();
        ensure(var != 0);
        ensure(var->enqc() != 0);
        ensure_equals(var->enq<std::string>(), "ship");
    }),
};

test_group tg1("cmdline_dbadb_mem", nullptr, db::MEM, tests);
test_group tg2("cmdline_dbadb_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("cmdline_dbadb_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("cmdline_dbadb_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("cmdline_dbadb_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
