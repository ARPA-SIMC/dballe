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

#include "config.h"
#include "db/test-utils-db.h"
#include "cmdline/dbadb.h"
#include "core/arrayfile.h"
#include "msg/codec.h"

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace std;

namespace dballe {
namespace tests {

struct test_dbadb : public db_test
{
};

}
}

namespace tut {

using namespace dballe::tests;
typedef db_tg<test_dbadb> tg;
typedef tg::object to;

template<> template<> void to::test<1>()
{
    // Resolve NULL and the empty string
    ensure_equals(dbadb::parse_op_report(*db, NULL), (const char*)NULL);
    ensure_equals(dbadb::parse_op_report(*db, ""), (const char*)NULL);

    // Resolve from names
    ensure_equals(string(dbadb::parse_op_report(*db, "synop")), "synop");

    // Resolve from nonexisting numbers
    try {
        dbadb::parse_op_report(*db, "11234");
        ensure(false);
    } catch (...) {}

    // Resolve from nonexisting names
    try {
        dbadb::parse_op_report(*db, "lolcats");
        ensure(false);
    } catch (...) {}
}

template<> template<> void to::test<2>()
{
    Dbadb dbadb(*db);

    // Import a synop
    cmdline::Reader reader;
    ensure_equals(dbadb.do_import(dballe::tests::datafile("bufr/obs0-1.22.bufr"), reader), 0);

    // Export forcing report as temp
    Record query;
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
}

}

namespace {

tut::tg tests_dbadb_mem_tg("cmdline_dbadb_mem", db::MEM);
#ifdef HAVE_ODBC
tut::tg tests_dbadb_v5_tg("cmdline_dbadb_v5", db::V5);
tut::tg tests_dbadb_v6_tg("cmdline_dbadb_v6", db::V6);
#endif

}

