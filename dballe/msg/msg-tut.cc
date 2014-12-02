/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

struct msg_shar
{
	msg_shar()
	{
	}

	~msg_shar()
	{
	}
};
TESTGRP(msg);

// Ensure that the context vector inside the message is in strict ascending order
void _ensure_msg_is_sorted(const wibble::tests::Location& loc, const Msg& msg)
{
	if (msg.data.size() < 2)
		return;
	for (unsigned i = 0; i < msg.data.size() - 1; ++i)
		inner_ensure(msg.data[i]->compare(*msg.data[i + 1]) < 0);
}
#define ensure_msg_is_sorted(x) _ensure_msg_is_sorted(wibble::tests::Location(__FILE__, __LINE__, "msg is sorted in " #x), (x))

/* Test dba_msg internal ordering */
template<> template<>
void to::test<1>()
{
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

	ensure(msg.find(WR_VAR(0, 1, 1), lev1, tr1) != NULL);
	ensure_equals(msg.find(WR_VAR(0, 1, 1), lev1, tr1), v3);

	ensure(msg.find(WR_VAR(0, 1, 1), lev1, tr2) != NULL);
	ensure_equals(msg.find(WR_VAR(0, 1, 1), lev1, tr2), v4);

	ensure(msg.find(WR_VAR(0, 1, 1), lev2, tr1) != NULL);
	ensure_equals(msg.find(WR_VAR(0, 1, 1), lev2, tr1), v2);

	ensure_equals(msg.find(WR_VAR(0, 1, 2), lev1, tr2), (Var*)0);
	ensure_equals(msg.find(WR_VAR(0, 1, 1), Level(0, 0, 0, 0), tr1), (Var*)0);
	ensure_equals(msg.find(WR_VAR(0, 1, 1), Level(3, 3, 3, 3), tr1), (Var*)0);
	ensure_equals(msg.find(WR_VAR(0, 1, 1), lev1, Trange(3, 3, 3)), (Var*)0);
}

/* Try to write a generic message from scratch */
template<> template<>
void to::test<2>()
{
	Msg msg;
	msg.type = MSG_GENERIC;
	//msg->type = MSG_SYNOP;

	// Fill in the dba_msg
	msg.seti(WR_VAR(0, 4, 1), 2008,   -1, Level::ana(), Trange::ana());
	msg.seti(WR_VAR(0, 4, 2),    5,   -1, Level::ana(), Trange::ana());
	msg.seti(WR_VAR(0, 4, 3),    7,   -1, Level::ana(), Trange::ana());
	// ...
	msg.setd(WR_VAR(0, 5, 1),   45.0, -1, Level::ana(), Trange::ana());
	msg.setd(WR_VAR(0, 6, 1),   11.0, -1, Level::ana(), Trange::ana());
	// ...
	msg.setd(WR_VAR(0,12, 101),  273.0, 75, Level(102, 2000), Trange::instant());

	// Append the dba_msg to a dba_msgs
	Msgs msgs;
	msgs.acquire(msg);

#if 0
    TODO TODO TODO
	// Encode to BUFR
	dba_rawmsg rmsg;
	CHECKED(dba_marshal_encode(msgs, BUFR, &rmsg));

	// Write it out
	//dba_file file;
	//CHECKED(dba_file_create(BUFR, "/tmp/prova", "wb", &file));
	//CHECKED(dba_file_write(file, rmsg));
	//dba_file_delete(file);
#endif
}

#if 0
	{
		int i;
		/* Check that the elements in tag_defs are properly sorted */
		for (i = 0; i < (sizeof(tag_defs) / sizeof(struct tagdef)) - 1; i++)
			fail_unless(strcmp(tag_defs[i].tag, tag_defs[i + 1].tag) < 0);
	}
#endif

#if 0
	{
		/* Keyword info handling */
		dba_varinfo info;

		gen_ensure(dba_record_keyword_byname("cippo") == DBA_KEY_ERROR);
		gen_ensure(dba_record_keyword_byname("zzzip") == DBA_KEY_ERROR);

		gen_ensure_equals(dba_record_keyword_byname("ana_id"), DBA_KEY_ANA_ID);
		CHECKED(dba_record_keyword_info(DBA_KEY_ANA_ID, &info));
		gen_ensure_equals(string(info->desc), string("Pseudoana database ID"));

		gen_ensure_equals(dba_record_keyword_byname("yearmin"), DBA_KEY_YEARMIN);
		CHECKED(dba_record_keyword_info(DBA_KEY_YEARMIN, &info));
		gen_ensure_equals(string(info->desc), string("Year or minimum year queried"));

		gen_ensure_equals(dba_record_keyword_byname("lat"), DBA_KEY_LAT);
		CHECKED(dba_record_keyword_info(DBA_KEY_LAT, &info));
		gen_ensure_equals(string(info->desc), string("Latitude"));

		gen_ensure_equals(dba_record_keyword_byname("lon"), DBA_KEY_LON);
		CHECKED(dba_record_keyword_info(DBA_KEY_LON, &info));
		gen_ensure_equals(string(info->desc), string("Longitude"));
	}

	{
		/* Record gets and sets */
		dba_err err;
		int ival;
		dba_record rec;

		CHECKED(dba_record_create(&rec));

		err = dba_record_key_enqi(rec, DBA_KEY_ANA_ID, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		err = dba_record_key_seti(rec, DBA_KEY_VAR, 1);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_TYPE);

		CHECKED(dba_record_key_seti(rec, DBA_KEY_ANA_ID, -10));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_LAT, 1234567));
		CHECKED(dba_record_key_setd(rec, DBA_KEY_LON, 76.54321));
		CHECKED(dba_record_key_setc(rec, DBA_KEY_YEARMIN, "1976"));
		CHECKED(dba_record_var_setc(rec, WR_VAR(0, 2, 121), "456"));

		fail_unless_int_is(key, DBA_KEY_ANA_ID, -10);
		fail_unless_double_is(key, DBA_KEY_ANA_ID, -10.0);
		fail_unless_int_is(key, DBA_KEY_LON, 7654321);
		fail_unless_double_is(key, DBA_KEY_LON, 76.54321);
		fail_unless_char_is(key, DBA_KEY_LON, "7654321");

		fail_unless_int_is(key, DBA_KEY_LAT, 1234567);
		fail_unless_double_is(key, DBA_KEY_LAT, 12.34567);
		fail_unless_char_is(key, DBA_KEY_LAT, "1234567");

		fail_unless_int_is(key, DBA_KEY_YEARMIN, 1976);
		fail_unless_double_is(key, DBA_KEY_YEARMIN, 1976);
		fail_unless_char_is(key, DBA_KEY_YEARMIN, "1976");

		fail_unless_int_is(var, WR_VAR(0, 2, 121), 456);
		/*fail_unless_float_is(rec, "B02121", 45600000000.0)*/;
		fail_unless_double_is(var, WR_VAR(0, 2, 121), 45600000000.0);
		fail_unless_char_is(var, WR_VAR(0, 2, 121), "456");

		CHECKED(dba_record_key_unset(rec, DBA_KEY_LAT));

		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		/* fprintf(stderr, "IVAL: %d\n", ival); */
		/* fprintf(stderr, "DVAL: %f\n", fval); */
		/*
		{
			int i = 7654321;
			double f = (double)i / 100000;
			fprintf(stderr, "I: %d, F: %f\n", i, f);
		}
		*/

		/* See if clear clears */
		dba_record_clear(rec);
		
		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		dba_record_clear(rec);

		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		dba_record_delete(rec);
	}
#endif

/* Test repmemo handling */
template<> template<>
void to::test<3>()
{
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
}

// Test var_id matcher
template<> template<>
void to::test<4>()
{
    Record matcher;
    matcher.set("data_id", 1);
    std::unique_ptr<Matcher> m = Matcher::create(matcher);

    Msg matched;
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

    matched.set(newvar(WR_VAR(0, 12, 101), 21.5), Level(1), Trange::instant());
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

    std::unique_ptr<wreport::Var> var = newvar(WR_VAR(0, 12, 103), 18.5);
    var->seta(ap_newvar(WR_VAR(0, 33, 195), 1));
    matched.set(move(var), Level(1), Trange::instant());
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
}

// Test station_id matcher
template<> template<>
void to::test<5>()
{
    Record matcher;
    matcher.set("ana_id", 1);
    std::unique_ptr<Matcher> m = Matcher::create(matcher);

    Msg matched;
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

    matched.seti(WR_VAR(0, 1, 192), 2, -1, Level::ana(), Trange::ana());
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

    matched.seti(WR_VAR(0, 1, 192), 1, -1, Level::ana(), Trange::ana());
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
}

// Test station WMO matcher
template<> template<>
void to::test<6>()
{
    {
        Record matcher;
        matcher.set("block", 11);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

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
        Record matcher;
        matcher.set("block", 11);
        matcher.set("station", 222);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

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
}

// Test date matcher
template<> template<>
void to::test<7>()
{
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(1999);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(2000);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmax", 2000);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(2001);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(2000);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }
    {
        Record matcher;
        matcher.set("yearmin", 2000);
        matcher.set("yearmax", 2010);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(1999);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(2011);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_year(2000);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);

        matched.set_year(2005);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);

        matched.set_year(2010);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }
}

// Test coordinates matcher
template<> template<>
void to::test<8>()
{
    {
        Record matcher;
        matcher.set("latmin", 45.0);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

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
        Record matcher;
        matcher.set("latmax", 45.0);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

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
        Record matcher;
        matcher.set("lonmin", 45.0);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

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
        Record matcher;
        matcher.set("lonmax", 45.0);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

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
        Record matcher;
        matcher.set("latmin", 45.0);
        matcher.set("latmax", 46.0);
        matcher.set("lonmin", 10.0);
        matcher.set("lonmax", 12.0);
        std::unique_ptr<Matcher> m = Matcher::create(matcher);

        Msg matched;
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_latitude(45.5);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_longitude(13.0);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

        matched.set_longitude(11.0);
        ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
    }
}

// Test rep_memo matcher
template<> template<>
void to::test<9>()
{
    Record matcher;
    matcher.set(DBA_KEY_REP_MEMO, "synop");
    std::unique_ptr<Matcher> m = Matcher::create(matcher);

    Msg matched;
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

    matched.set_rep_memo("temp");
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_NO);

    matched.set_rep_memo("synop");
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
}

// Test empty matcher
template<> template<>
void to::test<10>()
{
    Record matcher;
    std::unique_ptr<Matcher> m = Matcher::create(matcher);

    Msg matched;
    ensure(m->match(MatchedMsg(matched)) == matcher::MATCH_YES);
}

// Test CSV encoding/decoding
template<> template<>
void to::test<11>()
{
    Msg msg;
    msg.type = MSG_TEMP;
    //msg->type = MSG_SYNOP;

    // Fill in the dba_msg
    msg.set_year(2011);
    msg.set_month(5);
    msg.set_day(3);
    msg.set_hour(12);
    msg.set_minute(30);
    msg.set_second(45);
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
}

}

/* vim:set ts=4 sw=4: */
