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
#include "msg/msgs.h"
#include "msg/context.h"
#include <wreport/notes.h>
#include <set>

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct MsgCollector : public vector<Msg*>, public MsgConsumer
{
    ~MsgCollector()
    {
        for (iterator i = begin(); i != end(); ++i)
            delete *i;
    }
    void operator()(unique_ptr<Msg> msg) override
    {
        push_back(msg.release());
    }

    void from_db(DB& db, const char* query)
    {
        Query q;
        q.set_from_string(query);
        db.export_msgs(q, *this);
    }
};

unsigned diff_msg(Msg& first, Msg& second, const char* tag)
{
    notes::Collect c(cerr);
    int diffs = first.diff(second);
    if (diffs) dballe::tests::track_different_msgs(first, second, tag);
    return diffs;
}

static void normalise_datetime(Msg& msg)
{
    // Strip datetime attrs
    static int vars[] = { DBA_MSG_YEAR, DBA_MSG_MONTH, DBA_MSG_DAY, DBA_MSG_HOUR, DBA_MSG_MINUTE, DBA_MSG_SECOND };
    for (unsigned i = 0; i < 6; ++i)
    {
        Var* v = msg.edit_by_id(vars[i]);
        if (!v) continue;
        v->clear_attrs();
    }

    // Add a second of 0 if missing, since db export will always add
    // seconds
    if (!msg.get_second_var())
        msg.set_second(0, -1);
}


typedef dballe::tests::DBFixture Fixture;
typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("crex", [](Fixture& f) {
        auto& db = f.db;
        Query query;
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
                std::unique_ptr<Msgs> inmsgs = read_msgs(files[i], CREX);
                Msg& msg = *(*inmsgs)[0];
                normalise_datetime(msg);

                db->remove_all();
                db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

                // Explicitly set the rep_memo variable that is added during export
                msg.set_rep_memo(Msg::repmemo_from_type(msg.type));

                query.clear();
                query.rep_memo = Msg::repmemo_from_type(msg.type);

                MsgCollector msgs;
                db->export_msgs(query, msgs);
                ensure_equals(msgs.size(), 1u);
                ensure(msgs[0] != NULL);

                wassert(actual(diff_msg(msg, *msgs[0], "crex")) == 0);
            } catch (std::exception& e) {
                throw tut::failure(string("[") + files[i] + "] " + e.what());
            }
        }
    }),
    Test("bufr", [](Fixture& f) {
        // Test import/export with all BUFR samples
        auto& db = f.db;
        Query query;
        const char** files = dballe::tests::bufr_files;
        for (int i = 0; files[i] != NULL; i++)
        {
            try {
                std::unique_ptr<Msgs> inmsgs = read_msgs(files[i], BUFR);
                Msg& msg = *(*inmsgs)[0];
                normalise_datetime(msg);

                db->remove_all();
                db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

                query.clear();
                query.rep_memo = Msg::repmemo_from_type(msg.type);

                MsgCollector msgs;
                db->export_msgs(query, msgs);
                ensure_equals(msgs.size(), 1u);
                ensure(msgs[0] != NULL);

                // Explicitly set the rep_memo variable that is added during export
                msg.set_rep_memo(Msg::repmemo_from_type(msg.type));

                wassert(actual(diff_msg(msg, *msgs[0], "bufr")) == 0);
            } catch (std::exception& e) {
                throw tut::failure(string("[") + files[i] + "] " + e.what());
            }
        }
    }),
    Test("aof", [](Fixture& f) {
        // Test import/export with all AOF samples
        auto& db = f.db;
        Query query;
        const char** files = dballe::tests::aof_files;
        for (int i = 0; files[i] != NULL; i++)
        {
            try {
                std::unique_ptr<Msgs> inmsgs = read_msgs(files[i], AOF);
                Msg& msg = *(*inmsgs)[0];
                normalise_datetime(msg);

                db->remove_all();
                db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

                // Explicitly set the rep_memo variable that is added during export
                msg.set_rep_memo(Msg::repmemo_from_type(msg.type));

                // db->dump(stderr);

                query.clear();
                query.rep_memo = Msg::repmemo_from_type(msg.type);

                MsgCollector msgs;
                db->export_msgs(query, msgs);
                ensure_equals(msgs.size(), 1u);
                ensure(msgs[0] != NULL);

                wassert(actual(diff_msg(msg, *msgs[0], "bufr")) == 0);
            } catch (std::exception& e) {
                throw tut::failure(string("[") + files[i] + "] " + e.what());
            }
        }
    }),
    Test("multi", [](Fixture& f) {
        // Check that multiple messages are correctly identified during export
        auto& db = f.db;
        Query query;

        // msg1 has latitude 33.88
        // msg2 has latitude 46.22
        std::unique_ptr<Msgs> msgs1 = read_msgs("bufr/obs0-1.22.bufr", BUFR);
        std::unique_ptr<Msgs> msgs2 = read_msgs("bufr/obs0-3.504.bufr", BUFR);
        Msg& msg1 = *(*msgs1)[0];
        Msg& msg2 = *(*msgs2)[0];

        normalise_datetime(msg1);
        normalise_datetime(msg2);

        db->remove_all();
        db->import_msg(msg1, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
        db->import_msg(msg2, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

        // Explicitly set the rep_memo variable that is added during export
        msg1.set_rep_memo(Msg::repmemo_from_type(msg1.type));
        msg2.set_rep_memo(Msg::repmemo_from_type(msg2.type));

        query.clear();
        query.rep_memo = Msg::repmemo_from_type(msg1.type);

        // Warning: this test used to fail with older versions of MySQL.
        // See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=397597
        MsgCollector msgs;
        db->export_msgs(query, msgs);
        ensure_equals(msgs.size(), 2u);
        ensure(msgs[0] != NULL);
        ensure(msgs[1] != NULL);

        // Compare the two dba_msg
        wassert(actual(diff_msg(msg1, *msgs[0], "synop1")) == 0);
        wassert(actual(diff_msg(msg2, *msgs[1], "synop2")) == 0);
    }),
    Test("auto_repinfo", [](Fixture& f) {
        // Check automatic repinfo allocation
        auto& db = f.db;
        Query query;
        std::unique_ptr<Msgs> msgs = read_msgs("bufr/generic-new-repmemo.bufr", BUFR);
        Msg& msg = *(*msgs)[0];

        db->remove_all();
        db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

        query.clear();
        query.rep_memo = "enrico";

        MsgCollector outmsgs;
        db->export_msgs(query, outmsgs);
        wassert(actual(outmsgs.size()) == 1u);
        ensure(outmsgs[0] != NULL);

        // Compare the two dba_msg
        wassert(actual(diff_msg(msg, *outmsgs[0], "enrico")) == 0);
    }),
    Test("station_only", [](Fixture& f) {
        // Check that a message that only contains station variables does get imported
        auto& db = f.db;
        Query query;
        std::unique_ptr<Msgs> msgs = read_msgs("bufr/generic-onlystation.bufr", BUFR);
        Msg& msg = *(*msgs)[0];

        db->remove_all();
        db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);

        query.clear();
        std::unique_ptr<db::Cursor> cur = db->query_stations(query);
        wassert(actual(cur->remaining()) == 1);
        wassert(actual(cur->next()).istrue());

        Record result;
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
    }),
    Test("station_only_no_vars", [](Fixture& f) {
        // Check that a message that only contains station variables does get imported
        auto& db = f.db;
        Record query;
        std::unique_ptr<Msgs> msgs = read_msgs("bufr/arpa-station.bufr", BUFR);
        Msg& msg = *(*msgs)[0];

        db->remove_all();
        try {
            db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
            wassert(actual(false).istrue());
        } catch (error_notfound& e) {
            // ok.
        }

        // Redo it with manually generated messages, this should not get imported
        {
            db->remove_all();
            Msg msg;
            msg.type = MSG_GENERIC;
            msg.set_rep_memo("synop");
            msg.set_latitude(44.53000);
            msg.set_longitude(11.30000);
            try {
                db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
                wassert(actual(false).istrue());
            } catch (error_notfound& e) {
                // ok.
            }
        }

        // Same but with a datetime set. This should not get imported, but it
        // currently does because of a bug. I need to preserve the bug until
        // the software that relies on it has been migrated to use standard
        // DB-All.e features.
        {
            db->remove_all();
            Msg msg;
            msg.type = MSG_GENERIC;
            msg.set_rep_memo("synop");
            msg.set_latitude(44.53000);
            msg.set_longitude(11.30000);
            msg.set_year(1000);
            msg.set_month(1);
            msg.set_day(1);
            msg.set_hour(0);
            msg.set_minute(0);
            msg.set_second(0);
#warning TODO: fix this test to give an error once we do not need to support this bug anymore
            //try {
                db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
                //wassert(actual(false).istrue());
            //} catch (error_notfound& e) {
                // ok.
            //}
        }
    }),
    Test("import_dirty", [](Fixture& f) {
        // Try importing into a dirty database, no attributes involved
        auto& db = f.db;
        Record query;
        auto add_common = [](Msg& msg) {
            msg.type = MSG_SYNOP;
            msg.set_rep_memo("synop");
            msg.set_latitude(45.4);
            msg.set_longitude(11.2);
            msg.set_year(2015);
            msg.set_month(4);
            msg.set_day(25);
            msg.set_hour(12);
            msg.set_minute(30);
            msg.set_second(45);
        };

        // Build test messages
        Msg first;
        add_common(first);
        first.set_block(1);               // Station variable
        first.set_station(2);             // Station variable
        first.set_temp_2m(280.1);         // Data variable
        first.set_wet_temp_2m(275.8);     // Data variable

        Msg second;
        add_common(second);
        second.set_block(5);              // Station variable, different value
        second.set_station(2);            // Station variable, same value
        second.set_height_station(101.0); // Station variable, new value
        second.set_temp_2m(281.1);        // Data variable, different value
        second.set_wet_temp_2m(275.8);    // Data variable, same value
        second.set_humidity(55.6);        // Data variable, new value

        // Import the first message
        db->remove_all();
        db->import_msg(first, NULL, DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE);

        // Export and check
        MsgCollector export_first;
        export_first.from_db(*db, "rep_memo=synop");
        wassert(actual(export_first.size()) == 1);
        wassert(actual(diff_msg(first, *export_first[0], "first")) == 0);

        // Import the second message
        db->import_msg(second, NULL, DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE);

        // Export and check
        MsgCollector export_second;
        export_second.from_db(*db, "rep_memo=synop");
        wassert(actual(export_second.size()) == 1);
        wassert(actual(diff_msg(second, *export_second[0], "second")) == 0);
    }),
};

test_group tg1("db_import_mem", nullptr, db::MEM, tests);
test_group tg2("db_import_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_import_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_import_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_import_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
