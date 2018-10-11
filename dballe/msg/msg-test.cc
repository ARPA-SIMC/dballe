#include "tests.h"
#include "msg.h"
#include "context.h"
#include "dballe/core/csv.h"
#include <wreport/notes.h>

using namespace std;
using namespace wreport;
using namespace dballe;
using namespace dballe::tests;

namespace {

std::unique_ptr<Matcher> get_matcher(const char* q)
{
    return Matcher::create(dballe::tests::core_query_from_string(q));
}

void init(Messages& m)
{
    m.emplace_back(make_shared<Msg>());
}

// Ensure that the context vector inside the message is in strict ascending order
void msg_is_sorted(const Msg& msg)
{
    if (msg.data.size() < 2)
        return;
    for (unsigned i = 0; i < msg.data.size() - 1; ++i)
        wassert(actual(msg.data[i]->compare(*msg.data[i + 1])) < 0);
}

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("ordering", []() {
            // Test dba_msg internal ordering
            Msg msg;
            Level lev1(1, 1, 1, 1);
            Level lev2(2, 2, 2, 2);
            Trange tr1(1, 1, 1);
            Trange tr2(2, 2, 2);
            unique_ptr<Var> av1 = newvar(WR_VAR(0, 1, 1));
            unique_ptr<Var> av2 = newvar(WR_VAR(0, 1, 1));
            unique_ptr<Var> av3 = newvar(WR_VAR(0, 1, 1));
            unique_ptr<Var> av4 = newvar(WR_VAR(0, 1, 1));
            Var* v1 = av1.get();
            Var* v2 = av2.get();
            Var* v3 = av3.get();
            Var* v4 = av4.get();

            msg.set(move(av1), lev2, tr1); wassert(actual(msg.data.size()) == 1);
            msg.set(move(av2), lev2, tr1); wassert(actual(msg.data.size()) == 1);
            msg.set(move(av3), lev1, tr1); wassert(actual(msg.data.size()) == 2);
            msg.set(move(av4), lev1, tr2); wassert(actual(msg.data.size()) == 3);

            wassert(msg_is_sorted(msg));

            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev1, tr1)).istrue());
            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev1, tr1)) == v3);

            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev1, tr2)).istrue());
            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev1, tr2)) == v4);

            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev2, tr1)).istrue());
            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev2, tr1)) == v2);

            wassert(actual(msg.get(WR_VAR(0, 1, 2), lev1, tr2)) == (Var*)0);
            wassert(actual(msg.get(WR_VAR(0, 1, 1), Level(0, 0, 0, 0), tr1)) == (Var*)0);
            wassert(actual(msg.get(WR_VAR(0, 1, 1), Level(3, 3, 3, 3), tr1)) == (Var*)0);
            wassert(actual(msg.get(WR_VAR(0, 1, 1), lev1, Trange(3, 3, 3))) == (Var*)0);
        });
        add_method("compose", []() {
            // Try to write a generic message from scratch
            auto msg = make_shared<Msg>();
            msg->type = MSG_GENERIC;
            //msg->type = MSG_SYNOP;

            // Fill in the dba_msg
            msg->seti(WR_VAR(0, 4, 1), 2008,   -1, Level(), Trange());
            msg->seti(WR_VAR(0, 4, 2),    5,   -1, Level(), Trange());
            msg->seti(WR_VAR(0, 4, 3),    7,   -1, Level(), Trange());
            // ...
            msg->setd(WR_VAR(0, 5, 1),   45.0, -1, Level(), Trange());
            msg->setd(WR_VAR(0, 6, 1),   11.0, -1, Level(), Trange());
            // ...
            msg->setd(WR_VAR(0,12, 101),  273.0, 75, Level(102, 2000), Trange::instant());

            // Append the dba_msg to a dba_msgs
            Messages msgs;
            msgs.emplace_back(msg);

            // FIXME: missing an actual test?
        });
        add_method("repmemo", []() {
            // Test repmemo handling
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_SYNOP))) == MSG_SYNOP);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_METAR))) == MSG_METAR);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_SHIP))) == MSG_SHIP);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_BUOY))) == MSG_BUOY);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_AIREP))) == MSG_AIREP);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_AMDAR))) == MSG_AMDAR);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_ACARS))) == MSG_ACARS);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_PILOT))) == MSG_PILOT);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_TEMP))) == MSG_TEMP);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_TEMP_SHIP))) == MSG_TEMP_SHIP);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_SAT))) == MSG_SAT);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_POLLUTION))) == MSG_POLLUTION);
            wassert(actual(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_GENERIC))) == MSG_GENERIC);

            wassert(actual(Msg::type_from_repmemo("synop")) == MSG_SYNOP);
            wassert(actual(Msg::type_from_repmemo("SYNOP")) == MSG_SYNOP); // Case insensitive
            wassert(actual(Msg::type_from_repmemo("metar")) == MSG_METAR);
            wassert(actual(Msg::type_from_repmemo("ship")) == MSG_SHIP);
            wassert(actual(Msg::type_from_repmemo("buoy")) == MSG_BUOY);
            wassert(actual(Msg::type_from_repmemo("airep")) == MSG_AIREP);
            wassert(actual(Msg::type_from_repmemo("amdar")) == MSG_AMDAR);
            wassert(actual(Msg::type_from_repmemo("acars")) == MSG_ACARS);
            wassert(actual(Msg::type_from_repmemo("pilot")) == MSG_PILOT);
            wassert(actual(Msg::type_from_repmemo("temp")) == MSG_TEMP);
            wassert(actual(Msg::type_from_repmemo("tempship")) == MSG_TEMP_SHIP);
            wassert(actual(Msg::type_from_repmemo("satellite")) == MSG_SAT);
            wassert(actual(Msg::type_from_repmemo("pollution")) == MSG_POLLUTION);
            wassert(actual(Msg::type_from_repmemo("generic")) == MSG_GENERIC);
            wassert(actual(Msg::type_from_repmemo("antani")) == MSG_GENERIC);
            wassert(actual(Msg::type_from_repmemo("")) == MSG_GENERIC);
            wassert(actual(Msg::type_from_repmemo(NULL)) == MSG_GENERIC);
        });
        add_method("msg_match_stationid", []() {
            // Test station_id matcher
            auto m = get_matcher("ana_id=1");

            Msg matched;
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

            matched.seti(WR_VAR(0, 1, 192), 2, -1, Level(), Trange());
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

            matched.seti(WR_VAR(0, 1, 192), 1, -1, Level(), Trange());
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
        });
        add_method("msg_match_blockstation", []() {
            // Test station WMO matcher
            {
                auto m = get_matcher("block=11");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_block(1);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_block(11);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);

                matched.set_station(222);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }

            {
                auto m = get_matcher("block=11, station=222");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_block(1);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_block(11);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_station(22);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_station(222);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);

                matched.set_block(1);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);
            }
        });
        add_method("msg_match_date", []() {
            // Test date matcher
            {
                auto m = get_matcher("yearmin=2000");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(1999));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(2000));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmax=2000");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(2001));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(2000));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmin=2000, yearmax=2010");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(1999));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(2011));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_datetime(Datetime(2000));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);

                matched.set_datetime(Datetime(2005));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);

                matched.set_datetime(Datetime(2010));
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
        });
        add_method("msg_match_coords", []() {
            // Test coordinates matcher
            {
                auto m = get_matcher("latmin=45.00");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_latitude(43.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_latitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
                matched.set_latitude(46.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmax=45.00");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_latitude(46.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_latitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
                matched.set_latitude(44.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=45.00, lonmax=180.0");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_longitude(43.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_longitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
                matched.set_longitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=-180, lonmax=45.0");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_longitude(46.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_longitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
                matched.set_longitude(44.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

                Msg matched;
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_latitude(45.5);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_longitude(13.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

                matched.set_longitude(11.0);
                wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
            }
        });
        add_method("msg_match_repmemo", []() {
            // Test rep_memo matcher
            auto m = get_matcher("rep_memo=synop");

            Msg matched;
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

            matched.set_rep_memo("temp");
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

            matched.set_rep_memo("synop");
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
        });
        add_method("msg_match_empty", []() {
            // Test empty matcher
            std::unique_ptr<Matcher> m = Matcher::create(core::Query());

            Msg matched;
            wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
        });
        add_method("msg_csv", []() {
            // Test CSV encoding/decoding
            Msg msg;
            msg.type = MSG_TEMP;
            //msg->type = MSG_SYNOP;

            // Fill in the dba_msg
            msg.set_datetime(Datetime(2011, 5, 3, 12, 30, 45));
            msg.set_latitude(45.0);
            msg.set_longitude(11.0);
            msg.set_temp_2m(273.0, 75);
            msg.set_height_station(1230.0);
            msg.set_st_name("antani");
            msg.set_rep_memo("temp");

            MemoryCSVWriter csv;
            msg.to_csv(csv);

            Msg msg1;
            csv.buf.seekg(0);
            CSVReader in(csv.buf);
            wassert(actual(in.next()).istrue());
            msg1.from_csv(in);

            notes::Collect c(cerr);
            wassert(actual(msg.diff(msg1)) == 0u);
        });
        add_method("msgs_match_stationid", []() {
            // Test station_id matcher
            auto m = get_matcher("ana_id=1");

            Messages matched; init(matched);
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

            Msg::downcast(matched[0])->seti(WR_VAR(0, 1, 192), 2, -1, Level(), Trange());
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

            Msg::downcast(matched[0])->seti(WR_VAR(0, 1, 192), 1, -1, Level(), Trange());
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        });
        add_method("msgs_match_blockstation", []() {
            // Test station WMO matcher
            {
                auto m = get_matcher("block=11");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_block(1);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_block(11);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

                Msg::downcast(matched[0])->set_station(222);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }

            {
                auto m = get_matcher("block=11, station=222");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_block(1);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_block(11);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_station(22);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_station(222);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

                Msg::downcast(matched[0])->set_block(1);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);
            }
        });
        add_method("msgs_match_date", []() {
            // Test date matcher
            {
                auto m = get_matcher("yearmin=2000");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(1999));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(2000));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmax=2000");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(2001));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(2000));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmin=2000, yearmax=2010");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(1999));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(2011));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_datetime(Datetime(2000));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

                Msg::downcast(matched[0])->set_datetime(Datetime(2005));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

                Msg::downcast(matched[0])->set_datetime(Datetime(2010));
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
        });
        add_method("msgs_match_coords", []() {
            // Test coordinates matcher
            {
                auto m = get_matcher("latmin=45.00");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_latitude(43.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_latitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
                Msg::downcast(matched[0])->set_latitude(46.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmax=45.00");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_latitude(46.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_latitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
                Msg::downcast(matched[0])->set_latitude(44.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=45.00, lonmax=180.0");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_longitude(43.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_longitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
                Msg::downcast(matched[0])->set_longitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=-180, lonmax=45.0");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_longitude(46.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_longitude(45.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
                Msg::downcast(matched[0])->set_longitude(44.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

                Messages matched; init(matched);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_latitude(45.5);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_longitude(13.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

                Msg::downcast(matched[0])->set_longitude(11.0);
                wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
            }
        });
        add_method("msgs_match_repmemo", []() {
            // Test rep_memo matcher
            auto m = get_matcher("rep_memo=synop");

            Messages matched; init(matched);
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

            Msg::downcast(matched[0])->set_rep_memo("temp");
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

            Msg::downcast(matched[0])->set_rep_memo("synop");
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        });
        add_method("msgs_match_empty", []() {
            // Test empty matcher
            std::unique_ptr<Matcher> m = Matcher::create(core::Query());

            Messages matched; init(matched);
            wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        });
        add_method("msgs_csv", []() {
            // Test CSV encoding/decoding
            Messages msgs = read_msgs("bufr/synop-evapo.bufr", File::BUFR);

            // Serialise to CSV
            MemoryCSVWriter csv;
            msg::messages_to_csv(msgs, csv);

            // Read back
            csv.buf.seekg(0);
            CSVReader in(csv.buf);
            wassert(actual(in.next()).istrue());
            Messages msgs1 = msg::messages_from_csv(in);

            // Normalise before compare
            for (auto& i: msgs)
            {
                Msg::downcast(i)->set_rep_memo("synop");
                //m.set_second(0);
            }

            notes::Collect c(cerr);
            wassert(actual(msg::messages_diff(msgs, msgs1)) == 0u);
        });
        add_method("msgs_copy", []() {
            Messages msgs = read_msgs("bufr/synop-evapo.bufr", File::BUFR);
            Messages msgs1(msgs);
            Messages msgs2;
            msgs2 = msgs;
            msgs1 = msgs1;
        });
    }
} test("msg_msg");

}
