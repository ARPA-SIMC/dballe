/*
 * Copyright (C) 2009--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "test-utils-core.h"
#include "matcher.h"
#include "record.h"

#include <sstream>
#include <iostream>

using namespace std;
using namespace dballe;
using namespace dballe::core;
using namespace wibble::tests;

namespace tut {

struct matcher_shar {
    std::unique_ptr<Matcher> get_matcher(const char* q)
    {
        return Matcher::create(*dballe::tests::query_from_string(q));
    }
};
TESTGRP(matcher);

// Test var_id matcher
template<> template<>
void to::test<1>()
{
    auto m = get_matcher("context_id=1");

    core::Record matched;
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set("data_id", 2);
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set("data_id", 1);
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<2>()
{
    auto m = get_matcher("ana_id=1");

    core::Record matched;
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set("ana_id", 2);
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set("ana_id", 1);
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
}

// Test station WMO matcher
template<> template<>
void to::test<3>()
{
    {
        auto m = get_matcher("block=11");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("block", 1);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("block", 11);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);

        matched.set("station", 222);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }

    {
        auto m = get_matcher("block=11, station=222");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("block", 1);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("block", 11);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("station", 22);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("station", 222);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);

        matched.set("block", 1);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.unset("block");
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);
    }
}

// Test date matcher
template<> template<>
void to::test<4>()
{
    {
        auto m = get_matcher("yearmin=2000");

        core::Record matched;
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.set("year", 1999);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.set("year", 2000);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmax=2000");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2001);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmin=2000, yearmax=2010");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 1999);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2011);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);

        matched.set("year", 2005);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);

        matched.set("year", 2010);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
}

// Test coordinates matcher
template<> template<>
void to::test<5>()
{
    {
        auto m = get_matcher("latmin=45.00");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 43.0);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 45.0);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set("lat", 46.0);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmax=45.00");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 4600000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 4500000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set("lat", 4400000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=45.00, lonmax=180.0");

        core::Record matched;
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.set("lon", 4300000);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.set("lon", 4500000);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        matched.set("lon", 4500000);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=-180, lonmax=45.0");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 4600000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 4500000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set("lon", 4400000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

        core::Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 4550000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 1300000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 1100000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
}

// Test rep_memo matcher
template<> template<>
void to::test<6>()
{
    auto m = get_matcher("rep_memo=synop");

    core::Record matched;
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set("rep_memo", "temp");
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set("rep_memo", "synop");
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
}

// Test empty matcher
template<> template<>
void to::test<7>()
{
    auto query = dballe::Query::create();
    std::unique_ptr<Matcher> m = Matcher::create(*query);

    core::Record matched;
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
}

}

// vim:set ts=4 sw=4:
