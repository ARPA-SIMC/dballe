#include "config.h"
#include "db/tests.h"
#include "cmdline/dbadb.h"
#include "core/arrayfile.h"
#include "msg/codec.h"
#include "msg/msg.h"
#include "config.h"

using namespace dballe;
using namespace dballe::cmdline;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public FixtureTestCase<DBFixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("import", [](Fixture& f) {
            Dbadb dbadb(*f.db);

            // Import a synop
            cmdline::Reader reader;
            wassert(actual(dbadb.do_import(dballe::tests::datafile("bufr/obs0-1.22.bufr"), reader)) == 0);

            // Export forcing report as temp
            core::Query query;
            core::ArrayFile file(File::BUFR);
            wassert(actual(dbadb.do_export(query, file, "generic", "ship")) == 0);

            wassert(actual(file.msgs.size()) == 1u);

            // Decode results
            auto importer = msg::Importer::create(File::BUFR);
            Messages msgs = importer->from_binary(file.msgs[0]);
            wassert(actual(msgs.size()) == 1u);
            Msg& msg = Msg::downcast(msgs[0]);

            // Ensure they're ships
            wassert(actual(msg.type) == MSG_SHIP);

            // Check 001194 [SIM] Report mnemonic(CCITTIA5), too
            const Var* var = msg.get_rep_memo_var();
            wassert(actual(var).istrue());
            wassert(actual(var->enqc()).istrue());
            wassert(actual(var->enq<std::string>()) == "ship");
        });
    }
};

Tests tg1("cmdline_dbadb_mem", nullptr, db::MEM);
Tests tg2("cmdline_dbadb_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests tg4("cmdline_dbadb_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg6("cmdline_dbadb_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg8("cmdline_dbadb_v6_mysql", "MYSQL", db::V6);
#endif

}
