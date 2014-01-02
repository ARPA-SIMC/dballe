/*
 * Copyright (C) 2010--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "msg/test-utils-msg.h"
#include "core/csv.h"
#include "msg/msgs.h"
#include "msg/msg.h"
#include "msg/context.h"
#include <wreport/notes.h>

using namespace dballe;
using namespace wreport;
using namespace std;

namespace tut {

struct msgs_shar
{
    msgs_shar()
    {
    }

    ~msgs_shar()
    {
    }

    void init(Msgs& m)
    {
        auto_ptr<Msg> msg(new Msg);
        m.acquire(msg);
    }
};
TESTGRP(msgs);

// Test var_id matcher
template<> template<>
void to::test<1>()
{
    Record matcher;
    matcher.set("data_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Msgs matched; init(matched);
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

    matched[0]->set(newvar(WR_VAR(0, 12, 101), 21.5), Level(1), Trange::instant());
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

    std::auto_ptr<wreport::Var> var = newvar(WR_VAR(0, 12, 103), 18.5);
    var->seta(newvar(WR_VAR(0, 33, 195), 1));
    matched[0]->set(var, Level(1), Trange::instant());
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<2>()
{
    Record matcher;
    matcher.set("ana_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Msgs matched; init(matched);
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

    matched[0]->seti(WR_VAR(0, 1, 192), 2, -1, Level::ana(), Trange::ana());
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

    matched[0]->seti(WR_VAR(0, 1, 192), 1, -1, Level::ana(), Trange::ana());
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
}

// Test station WMO matcher
template<> template<>
void to::test<3>()
{
    {
        Record matcher;
        matcher.set("block", 11);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_block(1);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_block(11);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);

        matched[0]->set_station(222);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }

    {
        Record matcher;
        matcher.set("block", 11);
        matcher.set("station", 222);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_block(1);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_block(11);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_station(22);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_station(222);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);

        matched[0]->set_block(1);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);
    }
}

// Test date matcher
template<> template<>
void to::test<4>()
{
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(1999);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(2000);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmax", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(2001);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(2000);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        matcher.set("yearmax", 2010);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(1999);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(2011);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_year(2000);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);

        matched[0]->set_year(2005);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);

        matched[0]->set_year(2010);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
}

// Test coordinates matcher
template<> template<>
void to::test<5>()
{
    {
        Record matcher;
        matcher.set("latmin", 45.0);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_latitude(43.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_latitude(45.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
        matched[0]->set_latitude(46.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmax", 45.0);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_latitude(46.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_latitude(45.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
        matched[0]->set_latitude(44.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmin", 45.0);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_longitude(43.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_longitude(45.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
        matched[0]->set_longitude(45.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmax", 45.0);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_longitude(46.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_longitude(45.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
        matched[0]->set_longitude(44.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmin", 45.0);
        matcher.set("latmax", 46.0);
        matcher.set("lonmin", 10.0);
        matcher.set("lonmax", 12.0);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Msgs matched; init(matched);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_latitude(45.5);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_longitude(13.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

        matched[0]->set_longitude(11.0);
        ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
    }
}

// Test rep_memo matcher
template<> template<>
void to::test<6>()
{
    Record matcher;
    matcher.set(DBA_KEY_REP_MEMO, "synop");
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Msgs matched; init(matched);
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

    matched[0]->set_rep_memo("temp");
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_NO);

    matched[0]->set_rep_memo("synop");
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
}

// Test empty matcher
template<> template<>
void to::test<7>()
{
    Record matcher;
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Msgs matched; init(matched);
    ensure(m->match(MatchedMsgs(matched)) == matcher::MATCH_YES);
}

// Test CSV encoding/decoding
template<> template<>
void to::test<8>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/synop-evapo.bufr", BUFR);

    // Serialise to CSV
    stringstream str;
    msgs->to_csv(str);

    // Read back
    Msgs msgs1;
    str.seekg(0);
    CSVReader in(str);
    ensure(in.next());
    msgs1.from_csv(in);

    // Normalise before compare
    for (Msgs::iterator i = msgs->begin(); i != msgs->end(); ++i)
    {
        Msg& m = **i;
        m.set_rep_memo("synop");
        m.set_second(0);
    }

    notes::Collect c(cerr);
    ensure_equals(msgs->diff(msgs1), 0u);
}

// Test copy
template<> template<>
void to::test<9>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/synop-evapo.bufr", BUFR);
    Msgs msgs1(*msgs);
    Msgs msgs2;
    msgs2 = *msgs;
    msgs1 = msgs1;
}

}

/* vim:set ts=4 sw=4: */
