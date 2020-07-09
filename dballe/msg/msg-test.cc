#include "tests.h"
#include "msg.h"
#include "context.h"
#include "dballe/core/csv.h"
#include "dballe/cursor.h"
#include <wreport/notes.h>

using namespace std;
using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace dballe::impl;

namespace {

std::unique_ptr<Matcher> get_matcher(const char* q)
{
    return Matcher::create(dballe::tests::core_query_from_string(q));
}

void init(Messages& m)
{
    m.emplace_back(make_shared<impl::Message>());
}

// Ensure that the context vector inside the message is in strict ascending order
void msg_is_sorted(const impl::Message& msg)
{
    if (msg.data.size() < 2)
        return;
    for (auto i = msg.data.begin() + 1; i != msg.data.end(); ++i)
        wassert(actual((i-1)->compare(*i)) < 0);
}

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("msg_msg");

void Tests::register_tests() {

add_method("ordering", []() {
    // Test dba_msg internal ordering
    impl::Message msg;
    Level lev1(1, 1, 1, 1);
    Level lev2(2, 2, 2, 2);
    Trange tr1(1, 1, 1);
    Trange tr2(2, 2, 2);
    unique_ptr<Var> av1 = newvar(WR_VAR(0, 1, 1));
    unique_ptr<Var> av2 = newvar(WR_VAR(0, 1, 1));
    unique_ptr<Var> av3 = newvar(WR_VAR(0, 1, 1));
    unique_ptr<Var> av4 = newvar(WR_VAR(0, 1, 1));
    // Var* v1 = av1.get();
    Var* v2 = av2.get();
    Var* v3 = av3.get();
    Var* v4 = av4.get();

    msg.set(lev2, tr1, move(av1)); wassert(actual(msg.data.size()) == 1);
    msg.set(lev2, tr1, move(av2)); wassert(actual(msg.data.size()) == 1);
    msg.set(lev1, tr1, move(av3)); wassert(actual(msg.data.size()) == 2);
    msg.set(lev1, tr2, move(av4)); wassert(actual(msg.data.size()) == 3);

    wassert(msg_is_sorted(msg));

    wassert(actual(msg.get(lev1, tr1, WR_VAR(0, 1, 1))).istrue());
    wassert(actual(msg.get(lev1, tr1, WR_VAR(0, 1, 1))) == v3);

    wassert(actual(msg.get(lev1, tr2, WR_VAR(0, 1, 1))).istrue());
    wassert(actual(msg.get(lev1, tr2, WR_VAR(0, 1, 1))) == v4);

    wassert(actual(msg.get(lev2, tr1, WR_VAR(0, 1, 1))).istrue());
    wassert(actual(msg.get(lev2, tr1, WR_VAR(0, 1, 1))) == v2);

    wassert(actual(msg.get(lev1, tr2, WR_VAR(0, 1, 2))) == (Var*)0);
    wassert(actual(msg.get(Level(0, 0, 0, 0), tr1, WR_VAR(0, 1, 1))) == (Var*)0);
    wassert(actual(msg.get(Level(3, 3, 3, 3), tr1, WR_VAR(0, 1, 1))) == (Var*)0);
    wassert(actual(msg.get(lev1, Trange(3, 3, 3), WR_VAR(0, 1, 1))) == (Var*)0);
});

add_method("compose", []() {
    // Try to write a generic message from scratch
    auto msg = make_shared<impl::Message>();
    msg->type = MessageType::GENERIC;
    //msg->type = MessageType::SYNOP;

    // Fill in the dba_msg
    msg->station_data.set(newvar(WR_VAR(0, 4, 1), 2008));
    msg->station_data.set(newvar(WR_VAR(0, 4, 2),    5));
    msg->station_data.set(newvar(WR_VAR(0, 4, 3),    7));
    // ...
    msg->station_data.set(newvar(WR_VAR(0, 5, 1),   45.0));
    msg->station_data.set(newvar(WR_VAR(0, 6, 1),   11.0));
    // ...
    auto var = newvar(WR_VAR(0,12, 101),  273.0); var->seta(newvar(WR_VAR(0, 33, 7), 75));
    msg->set(Level(102, 2000), Trange::instant(), std::move(var));

    // Append the dba_msg to a dba_msgs
    Messages msgs;
    msgs.emplace_back(msg);

    // FIXME: missing an actual test?
});

add_method("repmemo", []() {
    // Test repmemo handling
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::SYNOP))) == MessageType::SYNOP);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::METAR))) == MessageType::METAR);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::SHIP))) == MessageType::SHIP);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::BUOY))) == MessageType::BUOY);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::AIREP))) == MessageType::AIREP);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::AMDAR))) == MessageType::AMDAR);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::ACARS))) == MessageType::ACARS);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::PILOT))) == MessageType::PILOT);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::TEMP))) == MessageType::TEMP);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::TEMP_SHIP))) == MessageType::TEMP_SHIP);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::SAT))) == MessageType::SAT);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::POLLUTION))) == MessageType::POLLUTION);
    wassert(actual(impl::Message::type_from_repmemo(impl::Message::repmemo_from_type(MessageType::GENERIC))) == MessageType::GENERIC);

    wassert(actual(impl::Message::type_from_repmemo("synop")) == MessageType::SYNOP);
    wassert(actual(impl::Message::type_from_repmemo("SYNOP")) == MessageType::SYNOP); // Case insensitive
    wassert(actual(impl::Message::type_from_repmemo("metar")) == MessageType::METAR);
    wassert(actual(impl::Message::type_from_repmemo("ship")) == MessageType::SHIP);
    wassert(actual(impl::Message::type_from_repmemo("buoy")) == MessageType::BUOY);
    wassert(actual(impl::Message::type_from_repmemo("airep")) == MessageType::AIREP);
    wassert(actual(impl::Message::type_from_repmemo("amdar")) == MessageType::AMDAR);
    wassert(actual(impl::Message::type_from_repmemo("acars")) == MessageType::ACARS);
    wassert(actual(impl::Message::type_from_repmemo("pilot")) == MessageType::PILOT);
    wassert(actual(impl::Message::type_from_repmemo("temp")) == MessageType::TEMP);
    wassert(actual(impl::Message::type_from_repmemo("tempship")) == MessageType::TEMP_SHIP);
    wassert(actual(impl::Message::type_from_repmemo("satellite")) == MessageType::SAT);
    wassert(actual(impl::Message::type_from_repmemo("pollution")) == MessageType::POLLUTION);
    wassert(actual(impl::Message::type_from_repmemo("generic")) == MessageType::GENERIC);
    wassert(actual(impl::Message::type_from_repmemo("antani")) == MessageType::GENERIC);
    wassert(actual(impl::Message::type_from_repmemo("")) == MessageType::GENERIC);
    wassert(actual(impl::Message::type_from_repmemo(NULL)) == MessageType::GENERIC);
});

add_method("msg_match_stationid", []() {
    // Test station_id matcher
    auto m = get_matcher("ana_id=1");

    impl::Message matched;
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

    matched.station_data.set(newvar(WR_VAR(0, 1, 192), 2));
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

    matched.station_data.set(newvar(WR_VAR(0, 1, 192), 1));
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
});

add_method("msg_match_blockstation", []() {
    // Test station WMO matcher
    {
        auto m = get_matcher("block=11");

        impl::Message matched;
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

        impl::Message matched;
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

        impl::Message matched;
        wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

        matched.set_datetime(Datetime(1999));
        wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

        matched.set_datetime(Datetime(2000));
        wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmax=2000");

        impl::Message matched;
        wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

        matched.set_datetime(Datetime(2001));
        wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

        matched.set_datetime(Datetime(2000));
        wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmin=2000, yearmax=2010");

        impl::Message matched;
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

        impl::Message matched;
        matched.set_longitude(11.0);
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

        impl::Message matched;
        matched.set_longitude(11.0);
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

        impl::Message matched;
        matched.set_latitude(11.0);
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

        impl::Message matched;
        matched.set_latitude(11.0);
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

        impl::Message matched;
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

    impl::Message matched;
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

    matched.set_rep_memo("temp");
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_NO);

    matched.set_rep_memo("synop");
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
});

add_method("msg_match_empty", []() {
    // Test empty matcher
    std::unique_ptr<Matcher> m = Matcher::create(core::Query());

    impl::Message matched;
    wassert(actual_matcher_result(m->match(MatchedMsg(matched))) == matcher::MATCH_YES);
});

add_method("msg_csv", []() {
    // Test CSV encoding/decoding
    impl::Message msg;
    msg.type = MessageType::TEMP;
    //msg->type = MessageType::SYNOP;

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

    impl::Message msg1;
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

    impl::Messages matched; init(matched);
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

    impl::Message::downcast(matched[0])->station_data.set(var(WR_VAR(0, 1, 192), 2));
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

    impl::Message::downcast(matched[0])->station_data.set(var(WR_VAR(0, 1, 192), 1));
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
});

add_method("msgs_match_blockstation", []() {
    // Test station WMO matcher
    {
        auto m = get_matcher("block=11");

        impl::Messages matched; init(matched);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_block(1);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_block(11);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

        impl::Message::downcast(matched[0])->set_station(222);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }

    {
        auto m = get_matcher("block=11, station=222");

        impl::Messages matched; init(matched);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_block(1);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_block(11);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_station(22);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_station(222);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

        impl::Message::downcast(matched[0])->set_block(1);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);
    }
});

add_method("msgs_match_date", []() {
    // Test date matcher
    {
        auto m = get_matcher("yearmin=2000");

        impl::Messages matched; init(matched);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(1999));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2000));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmax=2000");

        impl::Messages matched; init(matched);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2001));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2000));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmin=2000, yearmax=2010");

        impl::Messages matched; init(matched);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(1999));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2011));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2000));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2005));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);

        impl::Message::downcast(matched[0])->set_datetime(Datetime(2010));
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
});

add_method("msgs_match_coords", []() {
    // Test coordinates matcher
    {
        auto m = get_matcher("latmin=45.00");

        impl::Messages matched; init(matched);
        impl::Message::downcast(matched[0])->set_longitude(11.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_latitude(43.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_latitude(45.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        impl::Message::downcast(matched[0])->set_latitude(46.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmax=45.00");

        impl::Messages matched; init(matched);
        impl::Message::downcast(matched[0])->set_longitude(11.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_latitude(46.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_latitude(45.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        impl::Message::downcast(matched[0])->set_latitude(44.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=45.00, lonmax=180.0");

        impl::Messages matched; init(matched);
        impl::Message::downcast(matched[0])->set_latitude(11.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_longitude(43.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_longitude(45.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        impl::Message::downcast(matched[0])->set_longitude(45.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=-180, lonmax=45.0");

        impl::Messages matched; init(matched);
        impl::Message::downcast(matched[0])->set_latitude(11.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_longitude(46.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_longitude(45.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
        impl::Message::downcast(matched[0])->set_longitude(44.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

        impl::Messages matched; init(matched);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_latitude(45.5);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_longitude(13.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

        impl::Message::downcast(matched[0])->set_longitude(11.0);
        wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
    }
});

add_method("msgs_match_repmemo", []() {
    // Test rep_memo matcher
    auto m = get_matcher("rep_memo=synop");

    impl::Messages matched; init(matched);
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

    impl::Message::downcast(matched[0])->set_rep_memo("temp");
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_NO);

    impl::Message::downcast(matched[0])->set_rep_memo("synop");
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
});

add_method("msgs_match_empty", []() {
    // Test empty matcher
    std::unique_ptr<Matcher> m = Matcher::create(core::Query());

    impl::Messages matched; init(matched);
    wassert(actual_matcher_result(m->match(MatchedMessages(matched))) == matcher::MATCH_YES);
});

add_method("msgs_csv", []() {
    // Test CSV encoding/decoding
    impl::Messages msgs = read_msgs("bufr/synop-evapo.bufr", Encoding::BUFR);

    // Serialise to CSV
    MemoryCSVWriter csv;
    wassert(msg::messages_to_csv(msgs, csv));

    // Read back
    csv.buf.seekg(0);
    CSVReader in(csv.buf);
    wassert(actual(in.next()).istrue());
    impl::Messages msgs1 = wcallchecked(msg::messages_from_csv(in));

    // Normalise before compare
    for (auto& i: msgs)
    {
        wassert(impl::Message::downcast(i)->set_rep_memo("synop"));
        //m.set_second(0);
    }

    notes::Collect c(cerr);
    wassert(actual(msg::messages_diff(msgs, msgs1)) == 0u);
});

add_method("msgs_copy", []() {
    Messages msgs = read_msgs("bufr/synop-evapo.bufr", Encoding::BUFR);
    Messages msgs1(msgs);
    Messages msgs2;
    msgs2 = msgs;
    msgs1 = msgs1;
});

add_method("iterate_no_data", [] {
    // Check that a message that only contains station variables does get imported
    impl::Messages msgs = read_msgs("bufr/generic-onlystation.bufr", Encoding::BUFR);
    auto cur = msgs[0]->query_data(core::Query());

    wassert(actual(cur->remaining()) == 0);
    wassert(actual(cur->next()).isfalse());
});

}

}
