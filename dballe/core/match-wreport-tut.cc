/*
 * Copyright (C) 2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "match-wreport.h"
#include <wreport/subset.h>
#include <wreport/bulletin.h>

#include <sstream>
#include <iostream>

using namespace std;
using namespace dballe;
using namespace wreport;

namespace tut {

struct match_wreport_shar {
    void init(BufrBulletin& b)
    {
        b.edition = 4;
        b.centre = 200;
        b.subcentre = 0;
        b.master_table = 14;
        b.local_table = 0;
        b.load_tables();
    }
};
TESTGRP(match_wreport);

// Test var_id matcher
template<> template<>
void to::test<1>()
{
    Record matcher;
    matcher.set("data_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Subset s(Vartable::get("dballe"));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.store_variable_i(WR_VAR(0, 1, 1), 1);
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.back().seta(newvar(WR_VAR(0, 33, 195), 1));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<2>()
{
    Record matcher;
    matcher.set("ana_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Subset s(Vartable::get("dballe"));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.store_variable_i(WR_VAR(0, 1, 1), 1);
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.store_variable_i(WR_VAR(0, 1, 192), 1);
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
}

// Test station WMO matcher
template<> template<>
void to::test<3>()
{
    {
        Record matcher;
        matcher.set("block", 11);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 1, 1), 1);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.back().seti(11);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);

        s.store_variable_i(WR_VAR(0, 1, 2), 222);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }

    {
        Record matcher;
        matcher.set("block", 11);
        matcher.set("station", 222);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 1, 1), 1);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.back().seti(11);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 1, 2), 22);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.back().seti(222);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);

        s[0].seti(1);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0] = var(WR_VAR(0, 1, 192));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);
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

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 4, 1), 1999);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].seti(2000);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmax", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 4, 1), 2001);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].seti(2000);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        matcher.set("yearmax", 2010);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 4, 1), 1999);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].seti(2011);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].seti(2000);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);

        s[0].seti(2005);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);

        s[0].seti(2010);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
}

// Test coordinates matcher
template<> template<>
void to::test<5>()
{
    {
        Record matcher;
        matcher.set("latmin", 4500000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_d(WR_VAR(0, 5, 1), 43.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].setd(45.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
        s[0].setd(46.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmax", 4500000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_d(WR_VAR(0, 5, 1), 46.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].setd(45.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
        s[0].setd(44.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmin", 1100000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_d(WR_VAR(0, 6, 1), 10.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].setd(11.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
        s[0].setd(12.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmax", 1100000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_d(WR_VAR(0, 6, 1), 12.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].setd(11.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
        s[0].setd(10.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmin", 4500000);
        matcher.set("latmax", 4600000);
        matcher.set("lonmin", 1000000);
        matcher.set("lonmax", 1200000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_d(WR_VAR(0, 5, 1), 45.5);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_d(WR_VAR(0, 6, 1), 13.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[1].setd(11.0);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
}

// Test rep_memo matcher
template<> template<>
void to::test<6>()
{
    Record matcher;
    matcher.set(DBA_KEY_REP_MEMO, "synop");
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Subset s(Vartable::get("dballe"));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.store_variable_c(WR_VAR(0, 1, 194), "temp");
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s[0].setc("synop");
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
}

// Test empty matcher
template<> template<>
void to::test<7>()
{
    Record matcher;
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    Subset s(Vartable::get("dballe"));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
}

// Test var_id matcher
template<> template<>
void to::test<8>()
{
    Record matcher;
    matcher.set("data_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    BufrBulletin b; init(b);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(1).back().seta(newvar(WR_VAR(0, 33, 195), 1));
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<9>()
{
    Record matcher;
    matcher.set("ana_id", 1);
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    BufrBulletin b; init(b);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 192), 1);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
}

// Test station WMO matcher
template<> template<>
void to::test<10>()
{
    {
        Record matcher;
        matcher.set("block", 11);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).back().seti(11);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);

        b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 2), 222);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }

    {
        Record matcher;
        matcher.set("block", 11);
        matcher.set("station", 222);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 1), 1);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).back().seti(11);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 2), 22);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).back().seti(222);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);

        b.obtain_subset(0)[0].seti(1);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0] = var(WR_VAR(0, 1, 192));
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);
    }

    // Valid block and station must be in the same subset
    {
        Record matcher;
        matcher.set("block", 11);
        matcher.set("station", 222);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 1), 11);
        b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 2), 222);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 2), 222);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
}

// Test date matcher
template<> template<>
void to::test<11>()
{
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 1999);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].seti(2000);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmax", 2000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 2001);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].seti(2000);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        matcher.set("yearmax", 2010);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 1999);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].seti(2011);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].seti(2000);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);

        b.obtain_subset(0)[0].seti(2005);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);

        b.obtain_subset(0)[0].seti(2010);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
}

// Test coordinates matcher
template<> template<>
void to::test<12>()
{
    {
        Record matcher;
        matcher.set("latmin", 4500000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).store_variable_d(WR_VAR(0, 5, 1), 43.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1)[0].setd(45.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(1)[0].setd(46.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmax", 4500000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).store_variable_d(WR_VAR(0, 5, 1), 46.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1)[0].setd(45.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(1)[0].setd(44.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmin", 1100000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 10.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].setd(11.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(0)[0].setd(12.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("lonmax", 1100000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 12.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].setd(11.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(0)[0].setd(10.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("latmin", 4500000);
        matcher.set("latmax", 4600000);
        matcher.set("lonmin", 1000000);
        matcher.set("lonmax", 1200000);
        std::auto_ptr<Matcher> m = Matcher::create(matcher);

        BufrBulletin b; init(b);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_d(WR_VAR(0, 5, 1), 45.5);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 13.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[1].setd(11.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
}

// Test rep_memo matcher
template<> template<>
void to::test<13>()
{
    Record matcher;
    matcher.set(DBA_KEY_REP_MEMO, "synop");
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    BufrBulletin b; init(b);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(0).store_variable_c(WR_VAR(0, 1, 194), "temp");
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(0)[0].setc("synop");
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
}

// Test empty matcher
template<> template<>
void to::test<14>()
{
    Record matcher;
    std::auto_ptr<Matcher> m = Matcher::create(matcher);

    BufrBulletin b; init(b);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
}

}

// vim:set ts=4 sw=4:
