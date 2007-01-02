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

#include <extra/test-utils-msg.h>
#include <dballe/msg/file.h>
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/msg.h>

namespace tut {
using namespace tut_dballe;

struct bufrex_codec_shar
{
	bufrex_codec_shar()
	{
	}

	~bufrex_codec_shar()
	{
		test_untag();
	}
};
TESTGRP(bufrex_codec);

#define IS(field, val) do { \
		dba_var var = dba_msg_get_##field##_var(msg); \
		gen_ensure(var != 0); \
		gen_ensure_var_equals(var, val); \
	} while (0)
#define UN(field) do { \
		dba_var var = dba_msg_get_##field##_var(msg); \
		if (var != 0) \
			gen_ensure_var_undef(var); \
	} while (0)

template<> template<>
void to::test<1>()
{
	dba_msgs msgs = read_test_msg("crex/test-synop0.crex", CREX);
	dba_msg msg = msgs->msgs[0];
	gen_ensure_equals(msg->type, MSG_SYNOP);

	/* dba_msg_print((dba_msg)synop, stderr); */
	
	IS(block, 10);
	IS(station, 837);
	IS(st_type, 1);
	IS(year, 2004);
	IS(month, 11);
	IS(day, 30);
	IS(hour, 12);
	IS(minute, 0);
	IS(year, 2004);
	IS(latitude, 48.22);
	IS(longitude, 9.92);
	IS(height, 550.0);
	UN(height_baro);
	IS(press, 94340.0);
	IS(press_msl, 100940.0);
	//TESTD(var_press_tend, -170);
	IS(press_tend, 7.0);
	IS(wind_dir, 80.0);
	IS(wind_speed, 6.0);
	IS(temp_2m, 276.2);
	IS(dewpoint_2m, 273.9);
	UN(humidity);
	IS(visibility, 5000.0);
	IS(pres_wtr, 10);
	IS(past_wtr1, 2);
	IS(past_wtr2, 2);
	IS(cloud_n, 100);
	IS(cloud_nh, 8);
	IS(cloud_hh, 450.0);
	IS(cloud_cl, 35);
	IS(cloud_cm, 61);
	IS(cloud_ch, 60);
	IS(cloud_n1, 8);
	IS(cloud_c1, 6);
	IS(cloud_h1, 350.0);
	UN(cloud_n2);
	UN(cloud_c2);
	UN(cloud_h2);
	UN(cloud_n3);
	UN(cloud_c3);
	UN(cloud_h3);
	UN(cloud_n4);
	UN(cloud_c4);
	UN(cloud_h4);
	UN(tot_prec24);
	UN(tot_snow);

	dba_msg_delete(msg);
}

/* Test going from dba_msg to BUFR and back */
template<> template<>
void to::test<2>()
{
	const char* files[] = {
		"bufr/obs0-1.22.bufr", 
		"bufr/obs0-1.11188.bufr",
		"bufr/obs0-3.504.bufr", 
		"bufr/obs1-9.2.bufr", 
		"bufr/obs1-11.16.bufr", 
		"bufr/obs1-13.36.bufr", 
		"bufr/obs1-19.3.bufr", 
		"bufr/obs1-21.1.bufr", 
		"bufr/obs1-140.454.bufr", 
		"bufr/obs2-101.16.bufr", 
		"bufr/obs2-102.1.bufr", 
		"bufr/obs2-91.2.bufr", 
		"bufr/obs4-142.13803.bufr", 
		"bufr/obs4-142.1.bufr", 
		"bufr/obs4-144.4.bufr", 
		"bufr/obs4-145.4.bufr", 
		"bufr/test-airep1.bufr",
		"bufr/test-temp1.bufr", 
		NULL
	};

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		// Read the test message in a bufrex_raw
		bufrex_msg braw1 = read_test_msg_raw(files[i], BUFR);

		// Save category as a reference for reencoding
		int type = braw1->type;
		int subtype = braw1->subtype;
		
		// Finish converting in a dba_msg
		dba_msgs msgs1;
		CHECKED(bufrex_msg_to_dba_msgs(braw1, &msgs1));

		// Reencode the dba_msg in another dba_rawmsg
		dba_rawmsg raw2;
		CHECKED(bufrex_encode_bufr(msgs1, type, subtype, &raw2));

		// Parse the second dba_rawmsg
		dba_msgs msgs2;
		CHECKED(bufrex_decode_bufr(raw2, &msgs2));

		if (string(files[i]).find("2-101.16") != string::npos)
		{
			FILE* outraw = fopen("/tmp/1to2.txt", "w");
			bufrex_msg braw;
			CHECKED(bufrex_msg_create(&braw, BUFREX_BUFR));
			braw->type = type;
			braw->subtype = subtype;
			braw->opt.bufr.origin = 98;
			braw->opt.bufr.master_table = 6;
			braw->opt.bufr.local_table = 1;
			CHECKED(bufrex_msg_load_tables(braw));
			CHECKED(bufrex_msg_from_dba_msgs(braw, msgs1));
			bufrex_msg_print(braw, outraw);
			fclose(outraw);
			bufrex_msg_delete(braw);

			FILE* out1 = fopen("/tmp/msg1.txt", "w");
			FILE* out2 = fopen("/tmp/msg2.txt", "w");
				
			dba_msgs_print(msgs1, out1);
			dba_msgs_print(msgs2, out2);
			fclose(out1);
			fclose(out2);
		}

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(msgs1, msgs2, &diffs, stderr);
		gen_ensure_equals(diffs, 0);

		//cerr << files[i] << ": ok" << endl;

		dba_msgs_delete(msgs1);
		dba_msgs_delete(msgs2);
		bufrex_msg_delete(braw1);
		dba_rawmsg_delete(raw2);
	}
	test_untag();
}

/* Test going from CREX to dba_msg and back */
template<> template<>
void to::test<3>()
{
	const char* files[] = {
		"crex/test-mare0.crex",
		"crex/test-mare1.crex",
		"crex/test-mare2.crex",
		"crex/test-synop0.crex",
		"crex/test-synop1.crex",
		"crex/test-synop2.crex",
		"crex/test-synop3.crex",
		"crex/test-temp0.crex",
		NULL
	};

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		// Read the test message in a bufrex_raw
		bufrex_msg braw1 = read_test_msg_raw(files[i], CREX);

		// Save category as a reference for reencoding
		int type = braw1->type;
		int subtype = braw1->subtype;
		if (subtype == 0)
			type = 0;
		
		// Finish converting in a dba_msg
		dba_msgs msgs1;
		CHECKED(bufrex_msg_to_dba_msgs(braw1, &msgs1));

		// Reencode the dba_msg in another dba_rawmsg
		dba_rawmsg raw2;
		CHECKED(bufrex_encode_crex(msgs1, type, subtype, &raw2));

		// Parse the second dba_rawmsg
		dba_msgs msgs2;
		CHECKED(bufrex_decode_crex(raw2, &msgs2));

		/*
		if (string(files[i]).find("mare2") != string::npos)
		{
			dba_msg_print(msg1, stderr);
			dba_msg_print(msg2, stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(msgs1, msgs2, &diffs, stderr);
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(msgs1);
		dba_msgs_delete(msgs2);
		bufrex_msg_delete(braw1);
		dba_rawmsg_delete(raw2);
	}
	test_untag();
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */

}

/* vim:set ts=4 sw=4: */
