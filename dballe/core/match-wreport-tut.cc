/*
 * Copyright (C) 2010--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
    BufrBulletin* bulletin;

    match_wreport_shar()
        : bulletin(0)
    {
    }
    ~match_wreport_shar()
    {
        if (bulletin) delete bulletin;
    }
    BufrBulletin& init()
    {
        if (bulletin) delete bulletin;
        bulletin = BufrBulletin::create().release();
        BufrBulletin& b = *bulletin;
        b.edition = 4;
        b.centre = 200;
        b.subcentre = 0;
        b.master_table = 14;
        b.local_table = 0;
        b.load_tables();
        return b;
    }

    std::unique_ptr<Matcher> get_matcher(const char* q)
    {
        return Matcher::create(*dballe::tests::query_from_string(q));
    }
};
TESTGRP(match_wreport);

// Test var_id matcher
template<> template<>
void to::test<1>()
{
    auto m = get_matcher("context_id=1");

    Subset s(Vartable::get("dballe"));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.store_variable_i(WR_VAR(0, 1, 1), 1);
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

    s.back().seta(ap_newvar(WR_VAR(0, 33, 195), 1));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<2>()
{
    auto m = get_matcher("ana_id=1");

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
        auto m = get_matcher("block=11");

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
        auto m = get_matcher("block=11, station=222");

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
        auto m = get_matcher("yearmin=2000");

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 4, 1), 1999);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].seti(2000);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmax=2000");

        Subset s(Vartable::get("dballe"));
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s.store_variable_i(WR_VAR(0, 4, 1), 2001);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_NO);

        s[0].seti(2000);
        ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmin=2000, yearmax=2010");

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
        auto m = get_matcher("latmin=45.00");

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
        auto m = get_matcher("latmax=45.00");

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
        auto m = get_matcher("lonmin=11.00, lonmax=180.0");

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
        auto m = get_matcher("lonmin=-180, lonmax=11.0");

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
        auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

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
    auto m = get_matcher("rep_memo=synop");

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
    std::unique_ptr<Matcher> m = Matcher::create(*Query::create());

    Subset s(Vartable::get("dballe"));
    ensure(m->match(MatchedSubset(s)) == matcher::MATCH_YES);
}

// Test var_id matcher
template<> template<>
void to::test<8>()
{
    auto m = get_matcher("context_id=1");

    BufrBulletin& b = init();
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

    b.obtain_subset(1).back().seta(ap_newvar(WR_VAR(0, 33, 195), 1));
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<9>()
{
    auto m = get_matcher("ana_id=1");

    BufrBulletin& b = init();
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
        auto m = get_matcher("block=11");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).back().seti(11);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);

        b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 2), 222);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }

    {
        auto m = get_matcher("block=11, station=222");

        BufrBulletin& b = init();
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
        auto m = get_matcher("block=11, station=222");

        BufrBulletin& b = init();
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
        auto m = get_matcher("yearmin=2000");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 1999);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].seti(2000);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmax=2000");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 2001);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].seti(2000);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmin=2000, yearmax=2010");

        BufrBulletin& b = init();
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
        auto m = get_matcher("latmin=45.0");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).store_variable_d(WR_VAR(0, 5, 1), 43.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1)[0].setd(45.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(1)[0].setd(46.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmax=45.0");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1).store_variable_d(WR_VAR(0, 5, 1), 46.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(1)[0].setd(45.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(1)[0].setd(44.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=11.00, lonmax=180.0");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 10.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].setd(11.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(0)[0].setd(12.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=-180, lonmax=11.0");

        BufrBulletin& b = init();
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 12.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_NO);

        b.obtain_subset(0)[0].setd(11.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
        b.obtain_subset(0)[0].setd(10.0);
        ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

        BufrBulletin& b = init();
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
    auto m = get_matcher("rep_memo=synop");

    BufrBulletin& b = init();
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
    std::unique_ptr<Matcher> m = Matcher::create(*Query::create());

    BufrBulletin& b = init();
    ensure(m->match(MatchedBulletin(b)) == matcher::MATCH_YES);
}

}
