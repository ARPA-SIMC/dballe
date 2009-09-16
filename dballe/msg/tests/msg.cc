/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-msg.h>
#include <dballe/msg/datum.h>
#include <dballe/msg/level.h>

/*
extern "C" {
	dba_err dba_record_keyword_selftest();
};
*/

namespace tut {
using namespace tut_dballe;

struct msg_shar
{
	TestMsgEnv testenv;

	msg_shar()
	{
	}

	~msg_shar()
	{
	}
};
TESTGRP(msg);

// Ensure that the datum vector inside the level is in strict ascending order
void _ensure_level_is_sorted(const char* file, int line, dba_msg_level lev)
{
	if (lev->data_count < 2)
		return;
	for (int i = 0; i < lev->data_count - 1; i++)
		inner_ensure(dba_msg_datum_compare(lev->data[i], lev->data[i + 1]) < 0);
}
#define gen_ensure_level_is_sorted(x) _ensure_level_is_sorted(__FILE__, __LINE__, (x))

// Ensure that the level vector inside the message is in strict ascending order
void _ensure_msg_is_sorted(const char* file, int line, dba_msg msg)
{
	if (msg->data_count < 2)
		return;
	for (int i = 0; i < msg->data_count - 1; i++)
		inner_ensure(dba_msg_level_compare(msg->data[i], msg->data[i + 1]) < 0);
}
#define gen_ensure_msg_is_sorted(x) _ensure_msg_is_sorted(__FILE__, __LINE__, (x))


/* Test dba_msg_datum */
template<> template<>
void to::test<1>()
{
	dba_msg_datum d1, d2;
	dba_var v1, v2;

	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v1));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v2));

	CHECKED(dba_msg_datum_create(1, 2, 3, &d1));
	CHECKED(dba_msg_datum_create(1, 3, 2, &d2));

	gen_ensure_equals(d1->pind, 1);
	gen_ensure_equals(d1->p1, 2);
	gen_ensure_equals(d1->p2, 3);
	gen_ensure_equals(d2->pind, 1);
	gen_ensure_equals(d2->p1, 3);
	gen_ensure_equals(d2->p2, 2);

	d1->var = v1;
	d2->var = v2;

	gen_ensure(dba_msg_datum_compare(d1, d2) < 0);
	gen_ensure(dba_msg_datum_compare(d2, d1) > 0);
	gen_ensure_equals(dba_msg_datum_compare(d1, d1), 0);
	gen_ensure_equals(dba_msg_datum_compare(d2, d2), 0);

	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 1, 2, 4) < 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 1, 2, 2) > 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 1, 3, 3) < 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 1, 1, 3) > 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 2, 2, 3) < 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 0, 2, 3) > 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 2), 1, 2, 3) < 0);
	gen_ensure(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 0), 1, 2, 3) > 0);
	gen_ensure_equals(dba_msg_datum_compare2(d1, DBA_VAR(0, 1, 1), 1, 2, 3), 0);

	/* No need to delete v1 and v2, since dba_msg_datum takes ownership of the
	 * variables */
	dba_msg_datum_delete(d1);
	dba_msg_datum_delete(d2);
}

/* Test dba_msg_level external ordering */
template<> template<>
void to::test<2>()
{
	dba_msg_level lev1, lev2;

	CHECKED(dba_msg_level_create(1, 2, 3, 4, &lev1));
	CHECKED(dba_msg_level_create(2, 1, 4, 3, &lev2));

	gen_ensure_equals(lev1->data_count, 0);
	gen_ensure_equals(lev1->ltype1, 1);
	gen_ensure_equals(lev1->l1, 2);
	gen_ensure_equals(lev1->ltype2, 3);
	gen_ensure_equals(lev1->l2, 4);
	gen_ensure_equals(lev2->data_count, 0);
	gen_ensure_equals(lev2->ltype1, 2);
	gen_ensure_equals(lev2->l1, 1);
	gen_ensure_equals(lev2->ltype2, 4);
	gen_ensure_equals(lev2->l2, 3);

	gen_ensure(dba_msg_level_compare(lev1, lev2) < 0);
	gen_ensure(dba_msg_level_compare(lev2, lev1) > 0);
	gen_ensure_equals(dba_msg_level_compare(lev1, lev1), 0);
	gen_ensure_equals(dba_msg_level_compare(lev2, lev2), 0);

	gen_ensure(dba_msg_level_compare2(lev1, 1, 2, 4, 4) < 0);
	gen_ensure(dba_msg_level_compare2(lev1, 1, 2, 2, 4) > 0);
	gen_ensure(dba_msg_level_compare2(lev1, 1, 3, 3, 4) < 0);
	gen_ensure(dba_msg_level_compare2(lev1, 1, 1, 3, 4) > 0);
	gen_ensure(dba_msg_level_compare2(lev1, 2, 2, 3, 4) < 0);
	gen_ensure(dba_msg_level_compare2(lev1, 0, 2, 3, 4) > 0);
	gen_ensure_equals(dba_msg_level_compare2(lev1, 1, 2, 3, 4), 0);

	dba_msg_level_delete(lev1);
	dba_msg_level_delete(lev2);
}

/* Test dba_msg_level internal ordering */
template<> template<>
void to::test<3>()
{
	dba_msg_level lev;

	CHECKED(dba_msg_level_create(1, 2, 3, 4, &lev));

	dba_var v1, v2, v3, v4;
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v1));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v2));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v3));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v4));

	CHECKED(dba_msg_level_set_nocopy(lev, v1, 1, 2, 2));
	gen_ensure_equals(lev->data_count, 1);
	CHECKED(dba_msg_level_set_nocopy(lev, v2, 1, 1, 2));
	gen_ensure_equals(lev->data_count, 2);
	CHECKED(dba_msg_level_set_nocopy(lev, v4, 1, 2, 1));
	gen_ensure_equals(lev->data_count, 3);
	// Variables with same code on same timerange must get substituded and not
	// added
	CHECKED(dba_msg_level_set_nocopy(lev, v3, 1, 2, 1));
	gen_ensure_equals(lev->data_count, 3);

	gen_ensure_level_is_sorted(lev);

	gen_ensure(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 2, 2) != NULL);
	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 2, 2)->var, v1);

	gen_ensure(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 1, 2) != NULL);
	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 1, 2)->var, v2);

	gen_ensure(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 2, 1) != NULL);
	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 2, 1)->var, v3);

	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 2), 1, 2, 2), (dba_msg_datum)0);
	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 2, 2, 2), (dba_msg_datum)0);
	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 0, 2), (dba_msg_datum)0);
	gen_ensure_equals(dba_msg_level_find(lev, DBA_VAR(0, 1, 1), 1, 2, 0), (dba_msg_datum)0);
}

/* Test dba_msg internal ordering */
template<> template<>
void to::test<4>()
{
	dba_msg msg;

	CHECKED(dba_msg_create(&msg));

	dba_var v1, v2, v3, v4;
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v1));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v2));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v3));
	CHECKED(dba_var_create_local(DBA_VAR(0, 1, 1), &v4));

	CHECKED(dba_msg_set_nocopy(msg, v4, 2, 2, 2, 2, 1, 1, 1));
	gen_ensure_equals(msg->data_count, 1);
	CHECKED(dba_msg_set_nocopy(msg, v3, 2, 2, 2, 2, 1, 1, 1));
	gen_ensure_equals(msg->data_count, 1);
	CHECKED(dba_msg_set_nocopy(msg, v1, 1, 1, 1, 1, 1, 1, 1));
	gen_ensure_equals(msg->data_count, 2);
	CHECKED(dba_msg_set_nocopy(msg, v2, 1, 1, 1, 1, 2, 2, 2));
	gen_ensure_equals(msg->data_count, 2);

	gen_ensure_msg_is_sorted(msg);

	gen_ensure(dba_msg_find(msg, DBA_VAR(0, 1, 1), 1, 1, 1, 1, 1, 1, 1) != NULL);
	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 1), 1, 1, 1, 1, 1, 1, 1)->var, v1);

	gen_ensure(dba_msg_find(msg, DBA_VAR(0, 1, 1), 1, 1, 1, 1, 2, 2, 2) != NULL);
	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 1), 1, 1, 1, 1, 2, 2, 2)->var, v2);

	gen_ensure(dba_msg_find(msg, DBA_VAR(0, 1, 1), 2, 2, 2, 2, 1, 1, 1) != NULL);
	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 1), 2, 2, 2, 2, 1, 1, 1)->var, v3);

	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 2), 1, 1, 1, 1, 2, 2, 2), (dba_msg_datum)0);
	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 1), 0, 0, 0, 0, 1, 1, 1), (dba_msg_datum)0);
	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 1), 3, 3, 3, 3, 1, 1, 1), (dba_msg_datum)0);
	gen_ensure_equals(dba_msg_find(msg, DBA_VAR(0, 1, 1), 1, 1, 1, 1, 3, 3, 3), (dba_msg_datum)0);
}

/* Try to write a generic message from scratch */
template<> template<>
void to::test<5>()
{
	dba_msg msg;

	CHECKED(dba_msg_create(&msg));
	msg->type = MSG_GENERIC;
	//msg->type = MSG_SYNOP;

	// Fill in the dba_msg
	CHECKED(dba_msg_seti(msg, DBA_VAR(0, 4, 1), 2008,   -1, 257, 0, 0, 0, 0, 0, 0));
	CHECKED(dba_msg_seti(msg, DBA_VAR(0, 4, 2),    5,   -1, 257, 0, 0, 0, 0, 0, 0));
	CHECKED(dba_msg_seti(msg, DBA_VAR(0, 4, 3),    7,   -1, 257, 0, 0, 0, 0, 0, 0));
	// ...
	CHECKED(dba_msg_setd(msg, DBA_VAR(0, 5, 1),   45.0, -1, 257, 0, 0, 0, 0, 0, 0));
	CHECKED(dba_msg_setd(msg, DBA_VAR(0, 6, 1),   11.0, -1, 257, 0, 0, 0, 0, 0, 0));
	// ...
	CHECKED(dba_msg_setd(msg, DBA_VAR(0,12, 1),  273.0, 75, 102, 2000, 0, 0, 254, 0, 0));

	// Append the dba_msg to a dba_msgs
	dba_msgs msgs;
	CHECKED(dba_msgs_create(&msgs));
	CHECKED(dba_msgs_append_acquire(msgs, msg));

	// Encode to BUFR
	dba_rawmsg rmsg;
	CHECKED(dba_marshal_encode(msgs, BUFR, &rmsg));

	// Write it out
	//dba_file file;
	//CHECKED(dba_file_create(BUFR, "/tmp/prova", "wb", &file));
	//CHECKED(dba_file_write(file, rmsg));
	//dba_file_delete(file);

	dba_rawmsg_delete(rmsg);
	dba_msgs_delete(msgs);
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
		CHECKED(dba_record_var_setc(rec, DBA_VAR(0, 2, 121), "456"));

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

		fail_unless_int_is(var, DBA_VAR(0, 2, 121), 456);
		/*fail_unless_float_is(rec, "B02121", 45600000000.0)*/;
		fail_unless_double_is(var, DBA_VAR(0, 2, 121), 45600000000.0);
		fail_unless_char_is(var, DBA_VAR(0, 2, 121), "456");

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
void to::test<6>()
{
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_SYNOP)), MSG_SYNOP);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_METAR)), MSG_METAR);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_SHIP)), MSG_SHIP);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_BUOY)), MSG_BUOY);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_AIREP)), MSG_AIREP);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_AMDAR)), MSG_AMDAR);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_ACARS)), MSG_ACARS);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_PILOT)), MSG_PILOT);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_TEMP)), MSG_TEMP);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_TEMP_SHIP)), MSG_TEMP_SHIP);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_SAT)), MSG_SAT);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_POLLUTION)), MSG_POLLUTION);
	gen_ensure_equals(dba_msg_type_from_repmemo(dba_msg_repmemo_from_type(MSG_GENERIC)), MSG_GENERIC);

	gen_ensure_equals(dba_msg_type_from_repmemo("synop"), MSG_SYNOP);
	gen_ensure_equals(dba_msg_type_from_repmemo("SYNOP"), MSG_SYNOP); // Case insensitive
	gen_ensure_equals(dba_msg_type_from_repmemo("metar"), MSG_METAR);
	gen_ensure_equals(dba_msg_type_from_repmemo("ship"), MSG_SHIP);
	gen_ensure_equals(dba_msg_type_from_repmemo("buoy"), MSG_BUOY);
	gen_ensure_equals(dba_msg_type_from_repmemo("airep"), MSG_AIREP);
	gen_ensure_equals(dba_msg_type_from_repmemo("amdar"), MSG_AMDAR);
	gen_ensure_equals(dba_msg_type_from_repmemo("acars"), MSG_ACARS);
	gen_ensure_equals(dba_msg_type_from_repmemo("pilot"), MSG_PILOT);
	gen_ensure_equals(dba_msg_type_from_repmemo("temp"), MSG_TEMP);
	gen_ensure_equals(dba_msg_type_from_repmemo("tempship"), MSG_TEMP_SHIP);
	gen_ensure_equals(dba_msg_type_from_repmemo("satellite"), MSG_SAT);
	gen_ensure_equals(dba_msg_type_from_repmemo("pollution"), MSG_POLLUTION);
	gen_ensure_equals(dba_msg_type_from_repmemo("generic"), MSG_GENERIC);
	gen_ensure_equals(dba_msg_type_from_repmemo("antani"), MSG_GENERIC);
	gen_ensure_equals(dba_msg_type_from_repmemo(""), MSG_GENERIC);
	gen_ensure_equals(dba_msg_type_from_repmemo(NULL), MSG_GENERIC);
}

}

/* vim:set ts=4 sw=4: */
