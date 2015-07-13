#include "tests.h"
#include "msg.h"
#include "context.h"
#include "core/csv.h"
#include <wreport/notes.h>

using namespace dballe;
using namespace wreport;
using namespace std;

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

std::unique_ptr<Matcher> get_matcher(const char* q)
{
    return Matcher::create(dballe::tests::core_query_from_string(q));
}

void init(Messages& m)
{
    unique_ptr<Msg> msg(new Msg);
    m.append(move(msg));
}

// Ensure that the context vector inside the message is in strict ascending order
void _ensure_msg_is_sorted(const wibble::tests::Location& loc, const Msg& msg)
{
	if (msg.data.size() < 2)
		return;
	for (unsigned i = 0; i < msg.data.size() - 1; ++i)
		inner_ensure(msg.data[i]->compare(*msg.data[i + 1]) < 0);
}
#define ensure_msg_is_sorted(x) _ensure_msg_is_sorted(wibble::tests::Location(__FILE__, __LINE__, "msg is sorted in " #x), (x))


typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("ordering", [](Fixture& f) {
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

        msg.set(move(av1), lev2, tr1); ensure_equals(msg.data.size(), 1);
        msg.set(move(av2), lev2, tr1); ensure_equals(msg.data.size(), 1);
        msg.set(move(av3), lev1, tr1); ensure_equals(msg.data.size(), 2);
        msg.set(move(av4), lev1, tr2); ensure_equals(msg.data.size(), 3);

        ensure_msg_is_sorted(msg);

        ensure(msg.get(WR_VAR(0, 1, 1), lev1, tr1) != NULL);
        ensure_equals(msg.get(WR_VAR(0, 1, 1), lev1, tr1), v3);

        ensure(msg.get(WR_VAR(0, 1, 1), lev1, tr2) != NULL);
        ensure_equals(msg.get(WR_VAR(0, 1, 1), lev1, tr2), v4);

        ensure(msg.get(WR_VAR(0, 1, 1), lev2, tr1) != NULL);
        ensure_equals(msg.get(WR_VAR(0, 1, 1), lev2, tr1), v2);

        ensure_equals(msg.get(WR_VAR(0, 1, 2), lev1, tr2), (Var*)0);
        ensure_equals(msg.get(WR_VAR(0, 1, 1), Level(0, 0, 0, 0), tr1), (Var*)0);
        ensure_equals(msg.get(WR_VAR(0, 1, 1), Level(3, 3, 3, 3), tr1), (Var*)0);
        ensure_equals(msg.get(WR_VAR(0, 1, 1), lev1, Trange(3, 3, 3)), (Var*)0);
    }),
    Test("compose", [](Fixture& f) {
        // Try to write a generic message from scratch
        Msg msg;
        msg.type = MSG_GENERIC;
        //msg->type = MSG_SYNOP;

        // Fill in the dba_msg
        msg.seti(WR_VAR(0, 4, 1), 2008,   -1, Level(), Trange());
        msg.seti(WR_VAR(0, 4, 2),    5,   -1, Level(), Trange());
        msg.seti(WR_VAR(0, 4, 3),    7,   -1, Level(), Trange());
        // ...
        msg.setd(WR_VAR(0, 5, 1),   45.0, -1, Level(), Trange());
        msg.setd(WR_VAR(0, 6, 1),   11.0, -1, Level(), Trange());
        // ...
        msg.setd(WR_VAR(0,12, 101),  273.0, 75, Level(102, 2000), Trange::instant());

        // Append the dba_msg to a dba_msgs
        Messages msgs;
        msgs.append(msg);
    }),
    Test("repmemo", [](Fixture& f) {
        // Test repmemo handling
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_SYNOP)), MSG_SYNOP);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_METAR)), MSG_METAR);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_SHIP)), MSG_SHIP);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_BUOY)), MSG_BUOY);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_AIREP)), MSG_AIREP);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_AMDAR)), MSG_AMDAR);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_ACARS)), MSG_ACARS);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_PILOT)), MSG_PILOT);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_TEMP)), MSG_TEMP);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_TEMP_SHIP)), MSG_TEMP_SHIP);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_SAT)), MSG_SAT);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_POLLUTION)), MSG_POLLUTION);
        ensure_equals(Msg::type_from_repmemo(Msg::repmemo_from_type(MSG_GENERIC)), MSG_GENERIC);

        ensure_equals(Msg::type_from_repmemo("synop"), MSG_SYNOP);
        ensure_equals(Msg::type_from_repmemo("SYNOP"), MSG_SYNOP); // Case insensitive
        ensure_equals(Msg::type_from_repmemo("metar"), MSG_METAR);
        ensure_equals(Msg::type_from_repmemo("ship"), MSG_SHIP);
        ensure_equals(Msg::type_from_repmemo("buoy"), MSG_BUOY);
        ensure_equals(Msg::type_from_repmemo("airep"), MSG_AIREP);
        ensure_equals(Msg::type_from_repmemo("amdar"), MSG_AMDAR);
        ensure_equals(Msg::type_from_repmemo("acars"), MSG_ACARS);
        ensure_equals(Msg::type_from_repmemo("pilot"), MSG_PILOT);
        ensure_equals(Msg::type_from_repmemo("temp"), MSG_TEMP);
        ensure_equals(Msg::type_from_repmemo("tempship"), MSG_TEMP_SHIP);
        ensure_equals(Msg::type_from_repmemo("satellite"), MSG_SAT);
        ensure_equals(Msg::type_from_repmemo("pollution"), MSG_POLLUTION);
        ensure_equals(Msg::type_from_repmemo("generic"), MSG_GENERIC);
        ensure_equals(Msg::type_from_repmemo("antani"), MSG_GENERIC);
        ensure_equals(Msg::type_from_repmemo(""), MSG_GENERIC);
        ensure_equals(Msg::type_from_repmemo(NULL), MSG_GENERIC);
    }),
    Test("msg_match_varid", [](Fixture& f) {
        // Test var_id matcher
        auto m = get_matcher("context_id=1");

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set(newvar(WR_VAR(0, 12, 101), 21.5), Level(1), Trange::instant());
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        std::unique_ptr<wreport::Var> var = newvar(WR_VAR(0, 12, 103), 18.5);
        var->seta(newvar(WR_VAR(0, 33, 195), 1));
        matched.set(move(var), Level(1), Trange::instant());
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }),
    Test("msg_match_stationid", [](Fixture& f) {
        // Test station_id matcher
        auto m = get_matcher("ana_id=1");

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.seti(WR_VAR(0, 1, 192), 2, -1, Level(), Trange());
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.seti(WR_VAR(0, 1, 192), 1, -1, Level(), Trange());
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }),
    Test("msg_match_blockstation", [](Fixture& f) {
        // Test station WMO matcher
        {
            auto m = get_matcher("block=11");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_block(1);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_block(11);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);

            matched.set_station(222);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }

        {
            auto m = get_matcher("block=11, station=222");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_block(1);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_block(11);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_station(22);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_station(222);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);

            matched.set_block(1);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);
        }
    }),
    Test("msg_match_date", [](Fixture& f) {
        // Test date matcher
        {
            auto m = get_matcher("yearmin=2000");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(1999));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(2000));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("yearmax=2000");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(2001));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(2000));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("yearmin=2000, yearmax=2010");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(1999));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(2011));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_datetime(Datetime(2000));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);

            matched.set_datetime(Datetime(2005));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);

            matched.set_datetime(Datetime(2010));
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
    }),
    Test("msg_match_coords", [](Fixture& f) {
        // Test coordinates matcher
        {
            auto m = get_matcher("latmin=45.00");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_latitude(43.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_latitude(45.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
            matched.set_latitude(46.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("latmax=45.00");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_latitude(46.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_latitude(45.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
            matched.set_latitude(44.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("lonmin=45.00, lonmax=180.0");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_longitude(43.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_longitude(45.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
            matched.set_longitude(45.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("lonmin=-180, lonmax=45.0");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_longitude(46.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_longitude(45.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
            matched.set_longitude(44.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

            Msg matched;
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_latitude(45.5);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_longitude(13.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

            matched.set_longitude(11.0);
            ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
        }
    }),
    Test("msg_match_repmemo", [](Fixture& f) {
        // Test rep_memo matcher
        auto m = get_matcher("rep_memo=synop");

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_rep_memo("temp");
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_rep_memo("synop");
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }),
    Test("msg_match_empty", [](Fixture& f) {
        // Test empty matcher
        std::unique_ptr<Matcher> m = Matcher::create(core::Query());

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }),
    Test("msg_csv", [](Fixture& f) {
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

        stringstream str;
        msg.to_csv(str);

        Msg msg1;
        str.seekg(0);
        CSVReader in(str);
        ensure(in.next());
        msg1.from_csv(in);

        notes::Collect c(cerr);
        ensure_equals(msg.diff(msg1), 0u);
    }),
    Test("msgs_match_varid", [](Fixture& f) {
        // Test var_id matcher
        auto m = get_matcher("context_id=1");

        Messages matched; init(matched);
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

        Msg::downcast(matched[0]).set(newvar(WR_VAR(0, 12, 101), 21.5), Level(1), Trange::instant());
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

        std::unique_ptr<wreport::Var> var = newvar(WR_VAR(0, 12, 103), 18.5);
        var->seta(newvar(WR_VAR(0, 33, 195), 1));
        Msg::downcast(matched[0]).set(move(var), Level(1), Trange::instant());
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
    }),
    Test("msgs_match_stationid", [](Fixture& f) {
        // Test station_id matcher
        auto m = get_matcher("ana_id=1");

        Messages matched; init(matched);
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

        Msg::downcast(matched[0]).seti(WR_VAR(0, 1, 192), 2, -1, Level(), Trange());
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

        Msg::downcast(matched[0]).seti(WR_VAR(0, 1, 192), 1, -1, Level(), Trange());
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
    }),
    Test("msgs_match_blockstation", [](Fixture& f) {
        // Test station WMO matcher
        {
            auto m = get_matcher("block=11");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_block(1);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_block(11);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);

            Msg::downcast(matched[0]).set_station(222);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }

        {
            auto m = get_matcher("block=11, station=222");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_block(1);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_block(11);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_station(22);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_station(222);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);

            Msg::downcast(matched[0]).set_block(1);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);
        }
    }),
    Test("msgs_match_date", [](Fixture& f) {
        // Test date matcher
        {
            auto m = get_matcher("yearmin=2000");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(1999));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(2000));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("yearmax=2000");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(2001));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(2000));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("yearmin=2000, yearmax=2010");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(1999));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(2011));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_datetime(Datetime(2000));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);

            Msg::downcast(matched[0]).set_datetime(Datetime(2005));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);

            Msg::downcast(matched[0]).set_datetime(Datetime(2010));
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
    }),
    Test("msgs_match_coords", [](Fixture& f) {
        // Test coordinates matcher
        {
            auto m = get_matcher("latmin=45.00");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_latitude(43.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_latitude(45.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
            Msg::downcast(matched[0]).set_latitude(46.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("latmax=45.00");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_latitude(46.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_latitude(45.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
            Msg::downcast(matched[0]).set_latitude(44.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("lonmin=45.00, lonmax=180.0");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_longitude(43.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_longitude(45.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
            Msg::downcast(matched[0]).set_longitude(45.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("lonmin=-180, lonmax=45.0");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_longitude(46.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_longitude(45.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
            Msg::downcast(matched[0]).set_longitude(44.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
        {
            auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

            Messages matched; init(matched);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_latitude(45.5);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_longitude(13.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

            Msg::downcast(matched[0]).set_longitude(11.0);
            ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
        }
    }),
    Test("msgs_match_repmemo", [](Fixture& f) {
        // Test rep_memo matcher
        auto m = get_matcher("rep_memo=synop");

        Messages matched; init(matched);
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

        Msg::downcast(matched[0]).set_rep_memo("temp");
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_NO);

        Msg::downcast(matched[0]).set_rep_memo("synop");
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
    }),
    Test("msgs_match_empty", [](Fixture& f) {
        // Test empty matcher
        std::unique_ptr<Matcher> m = Matcher::create(core::Query());

        Messages matched; init(matched);
        ensure(m->match(MatchedMessages(matched)) == matcher::MATCH_YES);
    }),
    Test("msgs_csv", [](Fixture& f) {
        // Test CSV encoding/decoding
        Messages msgs = read_msgs("bufr/synop-evapo.bufr", File::BUFR);

        // Serialise to CSV
        stringstream str;
        msg::messages_to_csv(msgs, str);

        // Read back
        str.seekg(0);
        CSVReader in(str);
        ensure(in.next());
        Messages msgs1 = msg::messages_from_csv(in);

        // Normalise before compare
        for (auto& i: msgs)
        {
            Msg& m = Msg::downcast(i);
            m.set_rep_memo("synop");
            //m.set_second(0);
        }

        notes::Collect c(cerr);
        ensure_equals(msgs.diff(msgs1), 0u);
    }),
    Test("msgs_copy", [](Fixture& f) {
        Messages msgs = read_msgs("bufr/synop-evapo.bufr", File::BUFR);
        Messages msgs1(msgs);
        Messages msgs2;
        msgs2 = msgs;
        msgs1 = msgs1;
    }),
};

test_group newtg("msg_msg", tests);

}
