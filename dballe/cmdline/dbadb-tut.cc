#include "db/test-utils-db.h"
#include "cmdline/dbadb.h"
#include "core/arrayfile.h"
#include "msg/codec.h"

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace std;

namespace tut {

struct dbadb_shar : public dballe::tests::db_test
{
    void test_parse_op_report()
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

    void test_temp_ships()
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
        std::auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR);
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
};
TESTGRP(dbadb);

template<> template<> void to::test<1>() { use_db(db::V5); test_parse_op_report(); }
template<> template<> void to::test<2>() { use_db(db::V5); test_temp_ships(); }
template<> template<> void to::test<3>() { use_db(db::V6); test_parse_op_report(); }
template<> template<> void to::test<4>() { use_db(db::V6); test_temp_ships(); }

}
