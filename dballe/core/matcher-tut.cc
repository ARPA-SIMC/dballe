/*
 * Copyright (C) 2009--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

namespace tut {

struct matcher_shar {
};
TESTGRP(matcher);

// Test var_id matcher
template<> template<>
void to::test<1>()
{
    Record matcher;
    matcher.set("data_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Record matched;
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
    Record matcher;
    matcher.set("ana_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Record matched;
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
        Record matcher;
        matcher.set("block", 11);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("block", 1);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("block", 11);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);

        matched.set("station", 222);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }

    {
        Record matcher;
        matcher.set("block", 11);
        matcher.set("station", 222);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
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
        Record matcher;
        matcher.set("yearmin", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 1999);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmax", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2001);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("year", 2000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        matcher.set("yearmax", 2010);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
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
        Record matcher;
        matcher.set("latmin", 450000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set(DBA_KEY_LAT, 430000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set(DBA_KEY_LAT, 450000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set(DBA_KEY_LAT, 450000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmax", 450000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 460000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 450000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set("lat", 440000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmin", 450000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 430000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 450000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set("lon", 450000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmax", 450000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 460000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 450000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
        matched.set("lon", 440000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmin", 450000);
        matcher.set("latmax", 460000);
        matcher.set("lonmin", 100000);
        matcher.set("lonmax", 120000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Record matched;
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lat", 455000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 130000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

        matched.set("lon", 110000);
        ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
    }
}

// Test rep_memo matcher
template<> template<>
void to::test<6>()
{
    Record matcher;
    matcher.set(DBA_KEY_REP_MEMO, "synop");
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Record matched;
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set(DBA_KEY_REP_MEMO, "temp");
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_NO);

    matched.set(DBA_KEY_REP_MEMO, "synop");
    ensure(m->match(MatchedRecord(matched)) == matcher::MATCH_YES);
}

}

// vim:set ts=4 sw=4:
