#include "config.h"
#include "dballe/db/tests.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include "v7/db.h"
#include "v7/transaction.h"
#include <set>
#include <wreport/notes.h>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

static impl::DBImportOptions default_opts;

unsigned diff_msg(std::shared_ptr<Message> first,
                  std::shared_ptr<Message> second, const char* tag)
{
    notes::Collect c(cerr);
    int diffs = first->diff(*second);
    if (diffs)
        dballe::tests::track_different_msgs(*first, *second, tag);
    return diffs;
}

template <typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override
    {
        default_opts.import_attributes = true;
        default_opts.update_station    = true;

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
                if (blacklist.find(files[i]) != blacklist.end())
                    continue;
                try
                {
                    impl::Messages inmsgs = read_msgs(files[i], Encoding::CREX);
                    auto msg              = impl::Message::downcast(inmsgs[0]);

                    f.tr->remove_all();
                    f.tr->import_message(*msg, default_opts);

                    // Explicitly set the rep_memo variable that is added during
                    // export
                    msg->set_rep_memo(
                        impl::Message::repmemo_from_type(msg->get_type()));

                    query.clear();
                    query.report =
                        impl::Message::repmemo_from_type(msg->get_type());

                    impl::Messages msgs = wcallchecked(
                        dballe::tests::messages_from_db(f.tr, query));
                    wassert(actual(msgs.size()) == 1u);
                    wassert(actual(diff_msg(msg, msgs[0], "crex")) == 0);
                }
                catch (std::exception& e)
                {
                    wassert(throw TestFailed(string("[") + files[i] + "] " +
                                             e.what()));
                }
            }
        });
        this->add_method("bufr", [](Fixture& f) {
            // Test import/export with all BUFR samples
            core::Query query;
            const char** files = dballe::tests::bufr_files;
            for (int i = 0; files[i] != NULL; i++)
            {
                try
                {
                    impl::Messages inmsgs = read_msgs(files[i], Encoding::BUFR);
                    auto msg              = impl::Message::downcast(inmsgs[0]);

                    f.tr->remove_all();
                    wassert(f.tr->import_message(*msg, default_opts));

                    query.clear();
                    query.report = impl::Message::repmemo_from_type(msg->type);

                    impl::Messages msgs =
                        dballe::tests::messages_from_db(f.tr, query);
                    wassert(actual(msgs.size()) == 1u);

                    // Explicitly set the rep_memo variable that is added during
                    // export
                    msg->set_rep_memo(
                        impl::Message::repmemo_from_type(msg->type));

                    wassert(actual(diff_msg(msg, msgs[0], "bufr")) == 0);
                }
                catch (std::exception& e)
                {
                    wassert(throw TestFailed(string("[") + files[i] + "] " +
                                             e.what()));
                }
            }
        });
        this->add_method("multi", [](Fixture& f) {
            // Check that multiple messages are correctly identified during
            // export
            core::Query query;

            // msg1 has latitude 33.88
            // msg2 has latitude 46.22
            impl::Messages msgs1 =
                read_msgs("bufr/obs0-1.22.bufr", Encoding::BUFR);
            impl::Messages msgs2 =
                read_msgs("bufr/obs0-3.504.bufr", Encoding::BUFR);
            auto msg1 = impl::Message::downcast(msgs1[0]);
            auto msg2 = impl::Message::downcast(msgs2[0]);

            f.tr->remove_all();
            f.tr->import_message(*msg1, default_opts);
            f.tr->import_message(*msg2, default_opts);

            // Explicitly set the rep_memo variable that is added during export
            msg1->set_rep_memo(impl::Message::repmemo_from_type(msg1->type));
            msg2->set_rep_memo(impl::Message::repmemo_from_type(msg2->type));

            query.clear();
            query.report = impl::Message::repmemo_from_type(msg1->type);

            // Warning: this test used to fail with older versions of MySQL.
            // See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=397597
            impl::Messages msgs = dballe::tests::messages_from_db(f.tr, query);
            wassert(actual(msgs.size()) == 2u);

            // Compare the two dba_msg
            wassert(actual(diff_msg(msg1, msgs[0], "synop1")) == 0);
            wassert(actual(diff_msg(msg2, msgs[1], "synop2")) == 0);
        });
        this->add_method("double", [](Fixture& f) {
            // Check that importing the same message twice works
            // msg1 has latitude 33.88
            // msg2 has latitude 46.22
            impl::Messages msgs1 =
                read_msgs("bufr/obs0-1.22.bufr", Encoding::BUFR);
            auto msg1 = impl::Message::downcast(msgs1[0]);

            f.tr->remove_all();
            // auto t = db->transaction();
            f.tr->import_message(*msg1, default_opts);
            f.tr->import_message(*msg1, default_opts);
            // t->commit();

            // Explicitly set the rep_memo variable that is added during export
            msg1->set_rep_memo(impl::Message::repmemo_from_type(msg1->type));

            core::Query query;
            query.report = impl::Message::repmemo_from_type(msg1->type);

            impl::Messages msgs = dballe::tests::messages_from_db(f.tr, query);
            wassert(actual(msgs.size()) == 1u);

            // Compare the two dba_msg
            wassert(actual(diff_msg(msg1, msgs[0], "synop1")) == 0);
        });
        this->add_method("auto_repinfo", [](Fixture& f) {
            // Check automatic repinfo allocation
            core::Query query;
            impl::Messages msgs =
                read_msgs("bufr/generic-new-repmemo.bufr", Encoding::BUFR);
            auto msg = impl::Message::downcast(msgs[0]);

            f.tr->remove_all();
            f.tr->import_message(*msg, default_opts);

            query.clear();
            query.report = "enrico";

            impl::Messages outmsgs =
                dballe::tests::messages_from_db(f.tr, query);
            wassert(actual(outmsgs.size()) == 1u);
            // Compare the two dba_msg
            wassert(actual(diff_msg(msg, outmsgs[0], "enrico")) == 0);
        });
        this->add_method("station_only", [](Fixture& f) {
            // Check that a message that only contains station variables does
            // get imported
            impl::Messages msgs =
                read_msgs("bufr/generic-onlystation.bufr", Encoding::BUFR);

            f.tr->remove_all();
            f.tr->import_message(*msgs[0], default_opts);

            auto cur = f.tr->query_stations(core::Query());
            wassert(actual(cur->remaining()) == 1);
            wassert(actual(cur->next()).istrue());

            auto vars = cur->get_values();
            wassert(actual(vars.size()) == 5);
            wassert(actual(vars.var(WR_VAR(0, 1, 19)).format()) ==
                    "My beautifull station");
            wassert(actual(vars.var(WR_VAR(0, 1, 194)).format()) == "generic");
            wassert(actual(vars.var(WR_VAR(0, 5, 1)).format()) == "45.00000");
            wassert(actual(vars.var(WR_VAR(0, 6, 1)).format()) == "10.00000");
            wassert(actual(vars.var(WR_VAR(0, 7, 30)).format()) == "22.3");
        });
        this->add_method("station_only_no_vars", [](Fixture& f) {
            // Check that a message that only contains station variables does
            // get imported
            impl::Messages msgs =
                read_msgs("bufr/arpa-station.bufr", Encoding::BUFR);
            f.tr->remove_all();
            wassert(f.tr->import_message(*msgs[0], default_opts));

            // Redo it with manually generated messages, this should not get
            // imported
            {
                f.tr->remove_all();
                impl::Message msg;
                msg.type = MessageType::GENERIC;
                msg.set_rep_memo("synop");
                msg.set_latitude(44.53000);
                msg.set_longitude(11.30000);
                wassert(f.tr->import_message(msg, default_opts));
            }

            // Same but with a datetime set. This should not get imported, but
            // it currently does because of a bug. I need to preserve the bug
            // until the software that relies on it has been migrated to use
            // standard DB-All.e features.
            {
                f.tr->remove_all();
                impl::Message msg;
                msg.type = MessageType::GENERIC;
                msg.set_rep_memo("synop");
                msg.set_latitude(44.53000);
                msg.set_longitude(11.30000);
                msg.set_datetime(Datetime(1000, 1, 1, 0, 0, 0));
#warning TODO: fix this test to give an error once we do not need to support this bug anymore
                // try {
                f.tr->import_message(msg, default_opts);
                // wassert(actual(false).istrue());
                //} catch (error_notfound& e) {
                // ok.
                //}
            }
        });
        this->add_method("import_dirty", [](Fixture& f) {
            auto opts            = DBImportOptions::create();
            opts->update_station = true;
            opts->overwrite      = true;

            // Try importing into a dirty database, no attributes involved
            auto add_common = [](impl::Message& msg) {
                msg.type = MessageType::SYNOP;
                msg.set_rep_memo("synop");
                msg.set_latitude(45.4);
                msg.set_longitude(11.2);
                msg.set_datetime(Datetime(2015, 4, 25, 12, 30, 45));
            };

            // Build test messages
            auto first = make_shared<impl::Message>();
            add_common(*first);
            first->set_block(1);           // Station variable
            first->set_station(2);         // Station variable
            first->set_temp_2m(280.1);     // Data variable
            first->set_wet_temp_2m(275.8); // Data variable

            auto second = make_shared<impl::Message>();
            add_common(*second);
            second->set_block(5);   // Station variable, different value
            second->set_station(2); // Station variable, same value
            second->set_height_station(101.0); // Station variable, new value
            second->set_temp_2m(281.1);        // Data variable, different value
            second->set_wet_temp_2m(275.8);    // Data variable, same value
            second->set_humidity(55.6);        // Data variable, new value

            auto third = make_shared<impl::Message>();
            add_common(*third);
            third->set_block(6);   // Station variable, different value
            third->set_station(2); // Station variable, same value
            third->set_height_station(
                101.0); // Station variable, same value as the second
            third->set_height_baro(102.0); // Station variable, new value
            third->set_temp_2m(282.1);     // Data variable, different value
            third->set_wet_temp_2m(275.8); // Data variable, same value
            third->set_humidity(
                55.6); // Data variable, same value as the second
            third->set_dewpoint_2m(55.6); // Data variable, new value

            // Import the first message
            f.tr->remove_all();
            f.tr->import_message(*first, *opts);

            // Export and check
            impl::Messages export_first =
                dballe::tests::messages_from_db(f.tr, "rep_memo=synop");
            wassert(actual(export_first.size()) == 1);
            wassert(actual(diff_msg(first, export_first[0], "first")) == 0);

            // Import the second message
            f.tr->import_message(*second, *opts);

            // Export and check
            impl::Messages export_second =
                dballe::tests::messages_from_db(f.tr, "rep_memo=synop");
            wassert(actual(export_second.size()) == 1);
            wassert(actual(diff_msg(second, export_second[0], "second")) == 0);

            // Try again with empty caches
            f.tr->clear_cached_state();

            // Import the third message
            f.tr->import_message(*third, *opts);

            // Export and check
            impl::Messages export_third =
                dballe::tests::messages_from_db(f.tr, "rep_memo=synop");
            wassert(actual(export_third.size()) == 1);
            wassert(actual(diff_msg(third, export_third[0], "third")) == 0);
        });
        this->add_method("varlist", [](Fixture& f) {
            // Import filtering by varlist. See: #149
            auto opts = DBImportOptions::create();
            opts->varlist.push_back(WR_VAR(0, 12, 101));
            opts->varlist.push_back(WR_VAR(0, 12, 103));
            opts->varlist.push_back(WR_VAR(0, 13, 23));
            impl::Messages inmsgs =
                read_msgs("bufr/gts-synop-linate.bufr", Encoding::BUFR);
            wassert(f.tr->import_messages(inmsgs, *opts));

            core::Query query;
            auto cur       = f.tr->query_data(query);
            unsigned count = 0;
            while (cur->next())
            {
                wassert_true(
                    std::find(opts->varlist.begin(), opts->varlist.end(),
                              cur->get_varcode()) != opts->varlist.end());
                ++count;
            }
            wassert(actual(count) == 3);
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

} // namespace
