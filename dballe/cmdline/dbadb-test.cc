#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/cmdline/dbadb.h"
#include "dballe/core/arrayfile.h"
#include "dballe/msg/codec.h"
#include "dballe/msg/msg.h"
#include "config.h"

using namespace dballe;
using namespace dballe::cmdline;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

template<typename DB>
class Tests : public FixtureTestCase<DBFixture<DB>>
{
    typedef DBFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg2a("cmdline_dbadb_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg6a("cmdline_dbadb_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg8a("cmdline_dbadb_v7_mysql", "MYSQL");
#endif


template<typename DB>
void Tests<DB>::register_tests() {

this->add_method("import", [](Fixture& f) {
    Dbadb dbadb(*f.db);

    // Import a synop
    cmdline::ReaderOptions opts;
    cmdline::Reader reader(opts);
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
    auto msg = Msg::downcast(msgs[0]);

    // Ensure they're ships
    wassert(actual(msg->type) == MSG_SHIP);

    // Check 001194 [SIM] Report mnemonic(CCITTIA5), too
    const Var* var = msg->get_rep_memo_var();
    wassert(actual(var).istrue());
    wassert(actual(var->enqc()).istrue());
    wassert(actual(var->enq<std::string>()) == "ship");
});

this->add_method("issue62", [](Fixture& f) {
    // https://github.com/ARPA-SIMC/dballe/issues/62
    Dbadb dbadb(*f.db);

    // Import a synop
    cmdline::ReaderOptions opts;
    cmdline::Reader reader(opts);
    wassert(actual(dbadb.do_import(dballe::tests::datafile("bufr/issue62.bufr"), reader)) == 0);

    // Export
    core::Query query;
    core::ArrayFile file(File::BUFR);
    wassert(actual(dbadb.do_export(query, file, "", nullptr)) == 0);
    wassert(actual(file.msgs.size()) == 2u);

    // Decode results
    auto importer = msg::Importer::create(File::BUFR);
    Messages msgs = importer->from_binary(file.msgs[0]);
    wassert(actual(msgs.size()) == 1u);
    auto msg = Msg::downcast(msgs[0]);

    wassert(actual(msg->get_datetime()) == Datetime(2016, 3, 14, 23, 0, 4));
});

}

}
