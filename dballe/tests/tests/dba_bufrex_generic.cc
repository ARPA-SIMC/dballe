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
#include <dballe/msg/file.h>
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/msg.h>

namespace tut {
using namespace tut_dballe;

struct dba_bufrex_generic_shar
{
	dba_bufrex_generic_shar()
	{
	}

	~dba_bufrex_generic_shar()
	{
		test_untag();
	}
};
TESTGRP(dba_bufrex_generic);

// Try encoding and decoding an empty generic message
template<> template<>
void to::test<1>()
{
	dba_msgs msgs;
	CHECKED(dba_msgs_create(&msgs));

	dba_msg msg;
	CHECKED(dba_msg_create(&msg));

	CHECKED(dba_msgs_append_acquire(msgs, msg));

	/* Export msg as a generic message */
	dba_rawmsg raw;
	CHECKED(bufrex_encode_bufr(msgs, 0, 0, &raw));

	/* Parse it back */
	dba_msgs msgs1;
	CHECKED(bufrex_decode_bufr(raw, &msgs1));

	/* Check that the data are the same */
	int diffs = 0;
	dba_msgs_diff(msgs, msgs1, &diffs, stderr);
	if (diffs != 0) track_different_msgs(msgs, msgs1, "generic0");
	gen_ensure_equals(diffs, 0);

	dba_rawmsg_delete(raw);
	dba_msgs_delete(msgs);
	dba_msgs_delete(msgs1);
}

// Try encoding and decoding a generic message
template<> template<>
void to::test<2>()
{
	dba_msg msg;
	CHECKED(dba_msg_create(&msg));

	/* Fill up msg */
	CHECKED(dba_msg_set_press(			msg, 15,	45));
	CHECKED(dba_msg_set_height_anem(	msg, 15,	45));
	CHECKED(dba_msg_set_tot_snow(		msg, 15,	45));
	CHECKED(dba_msg_set_visibility(		msg, 15,	45));
	CHECKED(dba_msg_set_pres_wtr(		msg,  5,	45));
	CHECKED(dba_msg_set_metar_wtr(		msg,  5,	45));
	CHECKED(dba_msg_set_water_temp(		msg, 15,	45));
	CHECKED(dba_msg_set_past_wtr1(		msg,  2,	45));
	CHECKED(dba_msg_set_past_wtr2(		msg,  2,	45));
	CHECKED(dba_msg_set_press_tend(		msg,  5,	45));
	CHECKED(dba_msg_set_tot_prec24(		msg, 15,	45));
	CHECKED(dba_msg_set_press_3h(		msg, 15,	45));
	CHECKED(dba_msg_set_press_msl(		msg, 15,	45));
	CHECKED(dba_msg_set_qnh(			msg, 15,	45));
	CHECKED(dba_msg_set_temp_2m(		msg, 15,	45));
	CHECKED(dba_msg_set_wet_temp_2m(	msg, 15,	45));
	CHECKED(dba_msg_set_dewpoint_2m(	msg, 15,	45));
	CHECKED(dba_msg_set_humidity(		msg, 15,	45));
	CHECKED(dba_msg_set_wind_dir(		msg, 15,	45));
	CHECKED(dba_msg_set_wind_speed(		msg, 15,	45));
	CHECKED(dba_msg_set_ex_ccw_wind(	msg, 15,	45));
	CHECKED(dba_msg_set_ex_cw_wind(		msg, 15,	45));
	CHECKED(dba_msg_set_wind_max(		msg, 15,	45));
	CHECKED(dba_msg_set_cloud_n(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_nh(		msg, 10,	45));
	CHECKED(dba_msg_set_cloud_hh(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_cl(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_cm(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_ch(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_n1(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_c1(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_h1(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_n2(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_c2(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_h2(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_n3(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_c3(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_h3(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_n4(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_c4(		msg, 3,		45));
	CHECKED(dba_msg_set_cloud_h4(		msg, 3,		45));
	CHECKED(dba_msg_set_block(			msg, 3,		45));
	CHECKED(dba_msg_set_station(		msg, 3,		45));
	CHECKED(dba_msg_set_flight_reg_no(	msg, "pippo", 45));
	CHECKED(dba_msg_set_ident(			msg, "cippo", 45));
	CHECKED(dba_msg_set_st_dir(			msg, 3,		45));
	CHECKED(dba_msg_set_st_speed(		msg, 3,		45));
	CHECKED(dba_msg_set_st_name(		msg, "ciop", 45));
	CHECKED(dba_msg_set_st_name_icao(	msg, "cip", 45));
	CHECKED(dba_msg_set_st_type(		msg, 1,		45));
	CHECKED(dba_msg_set_wind_inst(		msg, 3,		45));
	CHECKED(dba_msg_set_temp_precision(	msg, 1.23,	45));
	CHECKED(dba_msg_set_sonde_type(		msg, 3,		45));
	CHECKED(dba_msg_set_sonde_method(	msg, 3,		45));
	CHECKED(dba_msg_set_navsys(			msg, 3,		45));
	CHECKED(dba_msg_set_data_relay(		msg, 3,		45));
	CHECKED(dba_msg_set_flight_roll(	msg, 3,		45));
	CHECKED(dba_msg_set_latlon_spec(	msg, 3,		45));
	CHECKED(dba_msg_set_year(			msg, 3,		45));
	CHECKED(dba_msg_set_month(			msg, 3,		45));
	CHECKED(dba_msg_set_day(			msg, 3,		45));
	CHECKED(dba_msg_set_hour(			msg, 3,		45));
	CHECKED(dba_msg_set_minute(			msg, 3,		45));
	CHECKED(dba_msg_set_latitude(		msg, 3,		45));
	CHECKED(dba_msg_set_longitude(		msg, 3,		45));
	CHECKED(dba_msg_set_height(			msg, 3,		45));
	CHECKED(dba_msg_set_height_baro(	msg, 3,		45));
	CHECKED(dba_msg_set_flight_phase(	msg, 3,		45));
	CHECKED(dba_msg_set_timesig(		msg, 3,		45));
	CHECKED(dba_msg_set_flight_press(	msg, 3,		45));

	dba_msgs msgs;
	CHECKED(dba_msgs_create(&msgs));
	CHECKED(dba_msgs_append_acquire(msgs, msg));

	/* Export msg as a generic message */
	dba_rawmsg raw;
	CHECKED(bufrex_encode_bufr(msgs, 0, 0, &raw));

	/* Parse it back */
	dba_msgs msgs1;
	CHECKED(bufrex_decode_bufr(raw, &msgs1));

	/* Check that the data are the same */
	int diffs = 0;
	dba_msgs_diff(msgs, msgs1, &diffs, stderr);
	if (diffs != 0)
	{
		dba_file out;
		CHECKED(dba_file_create(&out, BUFR, "/tmp/generic0.bufr", "w"));
		CHECKED(dba_file_write_raw(out, raw));
		dba_file_delete(out);
		track_different_msgs(msgs, msgs1, "generic0");
	}
	gen_ensure_equals(diffs, 0);

	dba_rawmsg_delete(raw);
	dba_msgs_delete(msgs);
	dba_msgs_delete(msgs1);
}

template<> template<>
void to::test<3>()
{
	dba_file file_gen;
	dba_file file_synop;
	int gfound, bfound, count = 0;
	dba_msgs gen, synop;

	CHECKED(dba_file_create(&file_gen, BUFR, "bufr/gen-generic.bufr", "r"));
	CHECKED(dba_file_create(&file_synop, BUFR, "bufr/gen-synop.bufr", "r"));

	CHECKED(dba_file_read(file_gen, &gen, &gfound));
	CHECKED(dba_file_read(file_synop, &synop, &bfound));
	gen_ensure_equals(gfound, bfound);
	do {
		count++;
		gen_ensure_equals(gen->msgs[0]->type, MSG_GENERIC);

		/* Export gen as a synop message */
		/* TODO: use the same template as the other synop message */
		dba_rawmsg raw;
		gen->msgs[0]->type = MSG_SYNOP;
		CHECKED(bufrex_encode_bufr(gen, 0, 0, &raw));

		/* Parse the second dba_rawmsg */
		dba_msgs synop1;
		CHECKED(bufrex_decode_bufr(raw, &synop1));

		/* Check that the data are the same */
		int diffs = 0;
		dba_msgs_diff(synop, synop1, &diffs, stderr);

		if (diffs != 0)
		{
			/*
			FILE* outraw = fopen("/tmp/1to2.txt", "w");
			bufrex_raw braw;
			CHECKED(bufrex_raw_create(&braw, BUFREX_BUFR));
			braw->type = type;
			braw->subtype = subtype;
			braw->opt.bufr.origin = 98;
			braw->opt.bufr.master_table = 6;
			braw->opt.bufr.local_table = 1;
			CHECKED(bufrex_raw_load_tables(braw));
			CHECKED(bufrex_raw_from_msg(braw, msg1));
			bufrex_raw_print(braw, outraw);
			fclose(outraw);
			bufrex_raw_delete(braw);
			*/
			fprintf(stderr, "Mismatch on message #%d\n", count);
			track_different_msgs(synop, synop1, "generic");
		}

		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(gen);
		dba_msgs_delete(synop);
		dba_msgs_delete(synop1);
		CHECKED(dba_file_read(file_gen, &gen, &gfound));
		CHECKED(dba_file_read(file_synop, &synop, &bfound));
		gen_ensure_equals(gfound, bfound);
	} while (gfound);

	dba_file_delete(file_gen);
	dba_file_delete(file_synop);
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */

/* Check that attributes are properly exported */
template<> template<>
void to::test<4>()
{
	dba_msg msg;

	/* Create a new message */
	CHECKED(dba_msg_create(&msg));
	msg->type = MSG_GENERIC;

	/* Set some metadata */
	CHECKED(dba_msg_set_year(msg, 2006, -1));
	CHECKED(dba_msg_set_month(msg, 1, -1));
	CHECKED(dba_msg_set_day(msg, 19, -1));
	CHECKED(dba_msg_set_hour(msg, 14, -1));
	CHECKED(dba_msg_set_minute(msg, 50, -1));
	CHECKED(dba_msg_set_latitude(msg, 50.0, -1));
	CHECKED(dba_msg_set_longitude(msg, 12.0, -1));

	/* Create a variable to add to the message */
	dba_var var;
	CHECKED(dba_var_create_local(DBA_VAR(0, 12, 1), &var));
	CHECKED(dba_var_setd(var, 270.15));

	/* Add some attributes to the variable */
	dba_var attr;
	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 2), &attr));
	CHECKED(dba_var_seti(attr, 1));
	CHECKED(dba_var_seta_nocopy(var, attr));

	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 3), &attr));
	CHECKED(dba_var_seti(attr, 2));
	CHECKED(dba_var_seta_nocopy(var, attr));

	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 5), &attr));
	CHECKED(dba_var_seti(attr, 3));
	CHECKED(dba_var_seta_nocopy(var, attr));

	/* Add the variable to the message */
	CHECKED(dba_msg_set_nocopy(msg, var, 1, 0, 0, 0, 0, 0));

	dba_msgs msgs;
	CHECKED(dba_msgs_create(&msgs));
	CHECKED(dba_msgs_append_acquire(msgs, msg));

	/* Encode the message */
	dba_rawmsg raw;
	CHECKED(bufrex_encode_bufr(msgs, 0, 0, &raw));

	/* Decode the message */
	dba_msgs msgs1;
	CHECKED(bufrex_decode_bufr(raw, &msgs1));
	dba_rawmsg_delete(raw);

	/* Check that everything is still there */
	int diffs = 0;
	dba_msgs_diff(msgs, msgs1, &diffs, stderr);
	gen_ensure_equals(diffs, 0);

	dba_msgs_delete(msgs);
	dba_msgs_delete(msgs1);
}

}

/* vim:set ts=4 sw=4: */
