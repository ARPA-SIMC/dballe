#include "config.h"
#include "dballe/db/tests.h"
#include "v7/db.h"
#include "v7/transaction.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <wreport/notes.h>
#include <set>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

unsigned diff_msg(std::shared_ptr<Message> first, std::shared_ptr<Message> second, const char* tag)
{
    notes::Collect c(cerr);
    int diffs = first->diff(*second);
    if (diffs) dballe::tests::track_different_msgs(*first, *second, tag);
    return diffs;
}

static void normalise_datetime(std::shared_ptr<Msg> msg)
{
    msg::Context* ctx = msg->edit_context(Level(), Trange());
    if (!ctx) return;

    // Strip datetime variables
    ctx->remove(WR_VAR(0, 4, 1));
    ctx->remove(WR_VAR(0, 4, 2));
    ctx->remove(WR_VAR(0, 4, 3));
    ctx->remove(WR_VAR(0, 4, 4));
    ctx->remove(WR_VAR(0, 4, 5));
    ctx->remove(WR_VAR(0, 4, 6));
}


template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override
    {
        this->add_method("crex", [](Fixture& f) {
            core::Query query;
            // Test import/export with all CREX samples
            const char** files = dballe::tests::crex_files;
            set<string> blacklist;
            // These files have no data to import
            blacklist.insert("crex/test-synop1.crex");
            blacklist.insert("crex/test-synop3.crex");
            for (int i = 0; files[i] != NULL; ++i)
            {
                if (blacklist.find(files[i]) != blacklist.end()) continue;
                try {
                    Messages inmsgs = read_msgs(files[i], File::CREX);
                    auto msg = Msg::downcast(inmsgs[0]);
                    normalise_datetime(msg);

                    f.tr->remove_all();
                    f.tr->import_msg(*msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

                    // Explicitly set the rep_memo variable that is added during export
                    msg->set_rep_memo(Msg::repmemo_from_type(msg->type));

                    query.clear();
                    query.rep_memo = Msg::repmemo_from_type(msg->type);

                    Messages msgs = wcallchecked(dballe::tests::messages_from_db(f.tr, query));
                    wassert(actual(msgs.size()) == 1u);
                    wassert(actual(diff_msg(msg, msgs[0], "crex")) == 0);
                } catch (std::exception& e) {
                    wassert(throw TestFailed(string("[") + files[i] + "] " + e.what()));
                }
            }
        });
        this->add_method("bufr", [](Fixture& f) {
            // Test import/export with all BUFR samples
            core::Query query;
            const char** files = dballe::tests::bufr_files;
            for (int i = 0; files[i] != NULL; i++)
            {
                try {
                    Messages inmsgs = read_msgs(files[i], File::BUFR);
                    auto msg = Msg::downcast(inmsgs[0]);
                    normalise_datetime(msg);

                    f.tr->remove_all();
                    wassert(f.tr->import_msg(*msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA));

                    query.clear();
                    query.rep_memo = Msg::repmemo_from_type(msg->type);

                    Messages msgs = dballe::tests::messages_from_db(f.tr, query);
                    wassert(actual(msgs.size()) == 1u);

                    // Explicitly set the rep_memo variable that is added during export
                    msg->set_rep_memo(Msg::repmemo_from_type(msg->type));

                    wassert(actual(diff_msg(msg, msgs[0], "bufr")) == 0);
                } catch (std::exception& e) {
                    wassert(throw TestFailed(string("[") + files[i] + "] " + e.what()));
                }
            }
        });
        this->add_method("multi", [](Fixture& f) {
            // Check that multiple messages are correctly identified during export
            core::Query query;

            // msg1 has latitude 33.88
            // msg2 has latitude 46.22
            Messages msgs1 = read_msgs("bufr/obs0-1.22.bufr", File::BUFR);
            Messages msgs2 = read_msgs("bufr/obs0-3.504.bufr", File::BUFR);
            auto msg1 = Msg::downcast(msgs1[0]);
            auto msg2 = Msg::downcast(msgs2[0]);

            normalise_datetime(msg1);
            normalise_datetime(msg2);

            f.tr->remove_all();
            f.tr->import_msg(*msg1, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
            f.tr->import_msg(*msg2, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

            // Explicitly set the rep_memo variable that is added during export
            msg1->set_rep_memo(Msg::repmemo_from_type(msg1->type));
            msg2->set_rep_memo(Msg::repmemo_from_type(msg2->type));

            query.clear();
            query.rep_memo = Msg::repmemo_from_type(msg1->type);

            // Warning: this test used to fail with older versions of MySQL.
            // See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=397597
            Messages msgs = dballe::tests::messages_from_db(f.tr, query);
            wassert(actual(msgs.size()) == 2u);

            // Compare the two dba_msg
            wassert(actual(diff_msg(msg1, msgs[0], "synop1")) == 0);
            wassert(actual(diff_msg(msg2, msgs[1], "synop2")) == 0);
        });
        this->add_method("double", [](Fixture& f) {
            // Check that importing the same message twice works
            // msg1 has latitude 33.88
            // msg2 has latitude 46.22
            Messages msgs1 = read_msgs("bufr/obs0-1.22.bufr", File::BUFR);
            auto msg1 = Msg::downcast(msgs1[0]);

            normalise_datetime(msg1);

            f.tr->remove_all();
            //auto t = db->transaction();
            f.tr->import_msg(*msg1, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
            f.tr->import_msg(*msg1, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
            //t->commit();

            // Explicitly set the rep_memo variable that is added during export
            msg1->set_rep_memo(Msg::repmemo_from_type(msg1->type));

            core::Query query;
            query.rep_memo = Msg::repmemo_from_type(msg1->type);

            Messages msgs = dballe::tests::messages_from_db(f.tr, query);
            wassert(actual(msgs.size()) == 1u);

            // Compare the two dba_msg
            wassert(actual(diff_msg(msg1, msgs[0], "synop1")) == 0);
        });
        this->add_method("auto_repinfo", [](Fixture& f) {
            // Check automatic repinfo allocation
            core::Query query;
            Messages msgs = read_msgs("bufr/generic-new-repmemo.bufr", File::BUFR);
            auto msg = Msg::downcast(msgs[0]);

            f.tr->remove_all();
            f.tr->import_msg(*msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

            query.clear();
            query.rep_memo = "enrico";

            Messages outmsgs = dballe::tests::messages_from_db(f.tr, query);
            wassert(actual(outmsgs.size()) == 1u);
            // Compare the two dba_msg
            wassert(actual(diff_msg(msg, outmsgs[0], "enrico")) == 0);
        });
        this->add_method("station_only", [](Fixture& f) {
            // Check that a message that only contains station variables does get imported
            Messages msgs = read_msgs("bufr/generic-onlystation.bufr", File::BUFR);

            f.tr->remove_all();
            f.tr->import_msg(*msgs[0], NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

            std::unique_ptr<db::Cursor> cur = f.tr->query_stations(core::Query());
            wassert(actual(cur->remaining()) == 1);
            wassert(actual(cur->next()).istrue());

            core::Record result;
            cur->to_record(result);

            const std::vector<wreport::Var*>& vars = result.vars();
            wassert(actual(vars.size()) == 5);
            wassert(actual(varcode_format(vars[0]->code())) == "B01019");
            wassert(actual(vars[0]->format()) == "My beautifull station");
            wassert(actual(varcode_format(vars[1]->code())) == "B01194");
            wassert(actual(vars[1]->format()) == "generic");
            wassert(actual(varcode_format(vars[2]->code())) == "B05001");
            wassert(actual(vars[2]->format()) == "45.00000");
            wassert(actual(varcode_format(vars[3]->code())) == "B06001");
            wassert(actual(vars[3]->format()) == "10.00000");
            wassert(actual(varcode_format(vars[4]->code())) == "B07030");
            wassert(actual(vars[4]->format()) == "22.3");
        });
        this->add_method("station_only_no_vars", [](Fixture& f) {
            // Check that a message that only contains station variables does get imported
            core::Record query;
            Messages msgs = read_msgs("bufr/arpa-station.bufr", File::BUFR);
            f.tr->remove_all();
            wassert(f.tr->import_msg(*msgs[0], NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA));

            // Redo it with manually generated messages, this should not get imported
            {
                f.tr->remove_all();
                Msg msg;
                msg.type = MSG_GENERIC;
                msg.set_rep_memo("synop");
                msg.set_latitude(44.53000);
                msg.set_longitude(11.30000);
                wassert(f.tr->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA));
            }

            // Same but with a datetime set. This should not get imported, but it
            // currently does because of a bug. I need to preserve the bug until
            // the software that relies on it has been migrated to use standard
            // DB-All.e features.
            {
                f.tr->remove_all();
                Msg msg;
                msg.type = MSG_GENERIC;
                msg.set_rep_memo("synop");
                msg.set_latitude(44.53000);
                msg.set_longitude(11.30000);
                msg.set_datetime(Datetime(1000, 1, 1, 0, 0, 0));
#warning TODO: fix this test to give an error once we do not need to support this bug anymore
                //try {
                    f.tr->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
                    //wassert(actual(false).istrue());
                //} catch (error_notfound& e) {
                    // ok.
                //}
            }
        });
        this->add_method("import_dirty", [](Fixture& f) {
            // Try importing into a dirty database, no attributes involved
            core::Record query;
            auto add_common = [](Msg& msg) {
                msg.type = MSG_SYNOP;
                msg.set_rep_memo("synop");
                msg.set_latitude(45.4);
                msg.set_longitude(11.2);
                msg.set_datetime(Datetime(2015, 4, 25, 12, 30, 45));
            };

            // Build test messages
            auto first = make_shared<Msg>();
            add_common(*first);
            first->set_block(1);               // Station variable
            first->set_station(2);             // Station variable
            first->set_temp_2m(280.1);         // Data variable
            first->set_wet_temp_2m(275.8);     // Data variable

            auto second = make_shared<Msg>();
            add_common(*second);
            second->set_block(5);              // Station variable, different value
            second->set_station(2);            // Station variable, same value
            second->set_height_station(101.0); // Station variable, new value
            second->set_temp_2m(281.1);        // Data variable, different value
            second->set_wet_temp_2m(275.8);    // Data variable, same value
            second->set_humidity(55.6);        // Data variable, new value

            auto third = make_shared<Msg>();
            add_common(*third);
            third->set_block(6);              // Station variable, different value
            third->set_station(2);            // Station variable, same value
            third->set_height_station(101.0); // Station variable, same value as the second
            third->set_height_baro(102.0);    // Station variable, new value
            third->set_temp_2m(282.1);        // Data variable, different value
            third->set_wet_temp_2m(275.8);    // Data variable, same value
            third->set_humidity(55.6);        // Data variable, same value as the second
            third->set_dewpoint_2m(55.6);     // Data variable, new value

            // Import the first message
            f.tr->remove_all();
            f.tr->import_msg(*first, NULL, DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE);

            // Export and check
            Messages export_first = dballe::tests::messages_from_db(f.tr, "rep_memo=synop");
            wassert(actual(export_first.size()) == 1);
            wassert(actual(diff_msg(first, export_first[0], "first")) == 0);

            // Import the second message
            f.tr->import_msg(*second, NULL, DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE);

            // Export and check
            Messages export_second = dballe::tests::messages_from_db(f.tr, "rep_memo=synop");
            wassert(actual(export_second.size()) == 1);
            wassert(actual(diff_msg(second, export_second[0], "second")) == 0);

            // Try again with empty caches
            f.tr->clear_cached_state();

            // Import the third message
            f.tr->import_msg(*third, NULL, DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE);

            // Export and check
            Messages export_third = dballe::tests::messages_from_db(f.tr, "rep_memo=synop");
            wassert(actual(export_third.size()) == 1);
            wassert(actual(diff_msg(third, export_third[0], "third")) == 0);
        });
    }
};

Tests<V7DB> tg2("db_import_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg4("db_import_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg6("db_import_v7_mysql", "MYSQL");
#endif

}
