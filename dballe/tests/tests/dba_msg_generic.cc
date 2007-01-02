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

#include <extra/test-utils.h>
#include <dballe/bufrex/bufrex_conv.h>
#include <dballe/bufrex/crex_decoder.h>
#include <dballe/bufrex/crex_encoder.h>
#include <dballe/bufrex/bufr_decoder.h>
#include <dballe/bufrex/bufr_encoder.h>
#include <dballe/msg/dba_msg_synop.h>

namespace tut {
using namespace tut_dballe;

struct dba_msg_generic_shar
{
	dba_msg_generic_shar()
	{
	}

	~dba_msg_generic_shar()
	{
	}
};
TESTGRP(dba_msg_generic);

#define IS(field, val) gen_ensure_var_equals(msg->field, val);
#define UN(field) gen_ensure_var_undef(msg->field);


template<> template<>
void to::test<1>()
{
	dba_msg m = read_test_msg("crex/test-synop0.crex", CREX);
	gen_ensure_equals(m->type, MSG_SYNOP);
	dba_msg_synop msg = (dba_msg_synop)m;

	/* dba_msg_print((dba_msg)synop, stderr); */
	
	IS(var_ana_block, 10);
	IS(var_ana_station, 837);
	IS(var_st_type, 1);
	IS(var_year, 2004);
	IS(var_month, 11);
	IS(var_day, 30);
	IS(var_hour, 12);
	IS(var_minute, 0);
	IS(var_year, 2004);
	IS(var_latitude, 48.22);
	IS(var_longitude, 9.92);
	IS(var_height, 550.0);
	UN(var_height_baro);
	IS(var_press, 94340.0);
	IS(var_press_msl, 100940.0);
	//TESTD(var_press_tend, -170);
	IS(var_press_tend, 7.0);
	IS(var_wind_dir, 80.0);
	IS(var_wind_speed, 6.0);
	IS(var_temp, 3.0);
	IS(var_dewpoint, 0.7);
	UN(var_humidity);
	IS(var_visibility, 5000.0);
	IS(var_pres_wtr, 10);
	IS(var_past_wtr1, 2);
	IS(var_past_wtr2, 2);
	IS(var_cloud_cover, 100);
	IS(var_cloud_amt, 8);
	IS(var_cloud_height, 450.0);
	IS(var_cloud_type1, 35);
	IS(var_cloud_type2, 61);
	IS(var_cloud_type3, 60);
	IS(var_cloud0_amt, 8);
	IS(var_cloud0_type, 6);
	IS(var_cloud0_height, 350.0);
	UN(var_cloud1_amt);
	UN(var_cloud1_type);
	UN(var_cloud1_height);
	UN(var_cloud2_amt);
	UN(var_cloud2_type);
	UN(var_cloud2_height);
	UN(var_cloud3_amt);
	UN(var_cloud3_type);
	UN(var_cloud3_height);
	UN(var_tot_prec24);
	UN(var_tot_snow);

	dba_msg_delete(m);
}

/* Test going from dba_msg to BUFR and back */
template<> template<>
void to::test<2>()
{
	const char* files[] = {
		"bufr/obs0-1.22.bufr", 
		"bufr/obs0-3.504.bufr", 
		"bufr/obs1-9.2.bufr", 
		"bufr/obs1-11.16.bufr", 
		"bufr/obs1-13.36.bufr", 
		"bufr/obs1-19.3.bufr", 
		"bufr/obs1-21.1.bufr", 
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
		bufrex_raw raw1 = read_test_msg_raw(files[i], BUFR);

		// Save category as a reference for reencoding
		int type, subtype;
		CHECKED(bufrex_raw_get_category(raw1, &type, &subtype));
		
		// Convert in a dba_msg
		dba_msg msg1;
		CHECKED(bufrex_to_msg(&msg1, raw1));

		// Reencode the dba_msg in another bufrex_raw
		bufrex_raw raw2;
		CHECKED(bufr_from_msg(msg1, &raw2, type, subtype));

		// Test that it is reencodable back to BUFR
		dba_rawmsg encoded;
		CHECKED(dba_rawmsg_create(&encoded));
		CHECKED(bufr_encoder_encode(raw2, encoded));
		
		// Convert the second bufrex_raw in a dba_msg
		dba_msg msg2;
		CHECKED(bufrex_to_msg(&msg2, raw2));

		/*
		if (string(files[i]).find("1-21") != string::npos)
		{
			dba_msg_print(msg1, stderr);
			dba_msg_print(msg2, stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msg_diff(msg1, msg2, &diffs, stderr);
		gen_ensure_equals(diffs, 0);

		//cerr << files[i] << ": ok" << endl;

		dba_msg_delete(msg1);
		dba_msg_delete(msg2);
		bufrex_raw_delete(raw1);
		bufrex_raw_delete(raw2);
		dba_rawmsg_delete(encoded);
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
		bufrex_raw raw1 = read_test_msg_raw(files[i], CREX);

		// Save category as a reference for reencoding
		int type, subtype;
		CHECKED(bufrex_raw_get_category(raw1, &type, &subtype));
		if (subtype == 0)
			type = 0;
		
		// Convert in a dba_msg
		dba_msg msg1;
		CHECKED(bufrex_to_msg(&msg1, raw1));

		// Reencode the dba_msg in another bufrex_raw
		bufrex_raw raw2;
		CHECKED(crex_from_msg(msg1, &raw2, type, subtype));
		
		// Test that it is reencodable back to CREX
		dba_rawmsg encoded;
		CHECKED(dba_rawmsg_create(&encoded));
		CHECKED(crex_encoder_encode(raw2, encoded));
		
		// Convert the second bufrex_raw in a dba_msg
		dba_msg msg2;
		CHECKED(bufrex_to_msg(&msg2, raw2));

		// Compare the two dba_msg
		int diffs = 0;
		dba_msg_diff(msg1, msg2, &diffs, stderr);
		gen_ensure_equals(diffs, 0);

		dba_msg_delete(msg1);
		dba_msg_delete(msg2);
		bufrex_raw_delete(raw1);
		bufrex_raw_delete(raw2);
		dba_rawmsg_delete(encoded);
	}
	test_untag();
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */

}

/* vim:set ts=4 sw=4: */
