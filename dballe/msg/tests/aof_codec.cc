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
#include <dballe/msg/aof_codec.h>
#include <dballe/msg/file.h>

namespace tut {
using namespace tut_dballe;

struct aof_codec_shar
{
	aof_codec_shar()
	{
	}

	~aof_codec_shar()
	{
	}
};
TESTGRP(aof_codec);

// Test simple decoding
template<> template<>
void to::test<1>()
{
	const char* files[] = {
		"aof/obs1-11.0.aof",
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs2-244.0.aof",
		"aof/obs2-244.1.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs5-36.30.aof",
		"aof/obs6-32.1573.aof",
		"aof/obs6-32.0.aof",
		NULL,
	};
	dba_rawmsg raw;

	/* Initialise decoding objects */
	CHECKED(dba_rawmsg_create(&raw));

	for (size_t i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		dba_file file;
		int found;

		/* Create the file reader */
		CHECKED(dba_file_create(AOF, files[i], "r", &file));

		/* Read the file header */
		/* CHECKED(aof_file_read_header(file, 0, 0)); */

		/* Read the data from file */
		CHECKED(dba_file_read(file, raw, &found));
		gen_ensure_equals(found, 1);

		/* Parse it */
		dba_msgs msgs = NULL;
		CHECKED(aof_codec_decode(raw, &msgs));
		gen_ensure(msgs != NULL);

		dba_msgs_delete(msgs);
		dba_file_delete(file);
	}
	test_untag();

	dba_rawmsg_delete(raw);
}

void strip_attributes(dba_msgs msgs)
{
	for (int msgidx = 0; msgidx < msgs->len; ++msgidx)
	{
		dba_msg msg = msgs->msgs[msgidx];
		for (int i = 0; i < msg->data_count; i++)
		{
			dba_msg_level lev = msg->data[i];
			for (int j = 0; j < lev->data_count; j++)
			{
				dba_msg_datum dat = lev->data[j];
				dba_var_clear_attrs(dat->var);
			}
		}
	}
}

void normalise_encoding_quirks(dba_msgs amsgs, dba_msgs bmsgs)
{
	int len = amsgs->len;
	if (len > bmsgs->len) len = bmsgs->len;
	for (int msgidx = 0; msgidx < len; ++msgidx)
	{
		dba_msg amsg = amsgs->msgs[msgidx];
		dba_msg bmsg = bmsgs->msgs[msgidx];

		// Recode BUFR attributes to match the AOF 2-bit values
		for (int i = 0; i < bmsg->data_count; i++)
		{
			dba_msg_level lev = bmsg->data[i];
			for (int j = 0; j < lev->data_count; j++)
			{
				dba_msg_datum dat = lev->data[j];
				dba_var_attr_iterator iter = dba_var_attr_iterate(dat->var);
				for (; iter != NULL; iter = dba_var_attr_iterator_next(iter))
				{
					dba_var attr = dba_var_attr_iterator_attr(iter);
					if (dba_var_code(attr) == DBA_VAR(0, 33, 7))
					{
						int val;
						CHECKED(dba_var_enqi(attr, &val));
						// Recode val using one of the value in the 4 steps of AOF
						if (val > 75)
							val = 76;
						else if (val > 50)
							val = 51;
						else if (val > 25)
							val = 26;
						else
							val = 0;
						CHECKED(dba_var_seti(attr, val));
					}
				}
			}
		}
		
		dba_var var;

		if ((var = dba_msg_get_block_var(bmsg)) != NULL)
			dba_var_clear_attrs(var);
		if ((var = dba_msg_get_station_var(bmsg)) != NULL)
			dba_var_clear_attrs(var);
		if ((var = dba_msg_get_st_type_var(bmsg)) != NULL)
			dba_var_clear_attrs(var);
		if ((var = dba_msg_get_ident_var(bmsg)) != NULL)
			dba_var_clear_attrs(var);
		if ((var = dba_msg_get_flight_phase_var(bmsg)) != NULL)
			dba_var_clear_attrs(var);

		if ((var = dba_msg_get_cloud_cl_var(bmsg)) != NULL &&
				strtoul(dba_var_value(var), NULL, 0) == 62 &&
				dba_msg_get_cloud_cl_var(amsg) == NULL)
			CHECKED(dba_msg_set_cloud_cl_var(amsg, var));

		if ((var = dba_msg_get_cloud_cm_var(bmsg)) != NULL &&
				strtoul(dba_var_value(var), NULL, 0) == 61 &&
				dba_msg_get_cloud_cm_var(amsg) == NULL)
			CHECKED(dba_msg_set_cloud_cm_var(amsg, var));

		if ((var = dba_msg_get_cloud_ch_var(bmsg)) != NULL &&
				strtoul(dba_var_value(var), NULL, 0) == 60 &&
				dba_msg_get_cloud_ch_var(amsg) == NULL)
			CHECKED(dba_msg_set_cloud_ch_var(amsg, var));

		if ((var = dba_msg_get_height_anem_var(bmsg)) != NULL &&
				dba_msg_get_height_anem_var(amsg) == NULL)
			CHECKED(dba_msg_set_height_anem_var(amsg, var));

		if ((var = dba_msg_get_navsys_var(bmsg)) != NULL &&
				dba_msg_get_navsys_var(amsg) == NULL)
			CHECKED(dba_msg_set_navsys_var(amsg, var));

		// In AOF, only synops and ships can encode direction and speed
		if (amsg->type != MSG_SHIP && amsg->type != MSG_SYNOP)
		{
			if ((var = dba_msg_get_st_dir_var(bmsg)) != NULL &&
					dba_msg_get_st_dir_var(amsg) == NULL)
				CHECKED(dba_msg_set_st_dir_var(amsg, var));

			if ((var = dba_msg_get_st_speed_var(bmsg)) != NULL &&
					dba_msg_get_st_speed_var(amsg) == NULL)
				CHECKED(dba_msg_set_st_speed_var(amsg, var));
		}

		if ((var = dba_msg_get_press_tend_var(bmsg)) != NULL &&
				dba_msg_get_press_tend_var(amsg) == NULL)
			CHECKED(dba_msg_set_press_tend_var(amsg, var));

		// AOF AMDAR has pressure indication, BUFR AMDAR has only height
		if (amsg->type == MSG_AMDAR)
		{
			dba_var p = dba_msg_get_flight_press_var(amsg);
#if 0
			dba_var h = dba_msg_get_height_var(bmsg);
			if (p && h)
			{
				double press, height;
				CHECKED(dba_var_enqd(p, &press));
				CHECKED(dba_var_enqd(h, &height));
				dba_msg_level l = dba_msg_find_level(bmsg, 103, (int)height, 0);
				if (l)
				{
					l->ltype = 100;
					l->l1 = (int)(press/100);
					CHECKED(dba_msg_set_flight_press_var(bmsg, p));
				}
			}
#endif
			if (p)
				CHECKED(dba_msg_set_flight_press_var(bmsg, p));
		}

		if (amsg->type == MSG_TEMP)
		{
			if ((var = dba_msg_get_sonde_type_var(bmsg)) != NULL &&
					dba_msg_get_sonde_type_var(amsg) == NULL)
			CHECKED(dba_msg_set_sonde_type_var(amsg, var));

			if ((var = dba_msg_get_sonde_method_var(bmsg)) != NULL &&
					dba_msg_get_sonde_method_var(amsg) == NULL)
			CHECKED(dba_msg_set_sonde_method_var(amsg, var));
		}

		if (amsg->type == MSG_TEMP_SHIP)
		{
			if ((var = dba_msg_get_height_var(bmsg)) != NULL &&
					dba_msg_get_height_var(amsg) == NULL)
			CHECKED(dba_msg_set_height_var(amsg, var));
		}

		if (amsg->type == MSG_TEMP || amsg->type == MSG_TEMP_SHIP)
		{
			if ((var = dba_msg_get_cloud_n_var(bmsg)) != NULL &&
					dba_msg_get_cloud_n_var(amsg) == NULL)
				CHECKED(dba_msg_set_cloud_n_var(amsg, var));

			if ((var = dba_msg_get_cloud_nh_var(bmsg)) != NULL &&
					dba_msg_get_cloud_nh_var(amsg) == NULL)
				CHECKED(dba_msg_set_cloud_nh_var(amsg, var));

			if ((var = dba_msg_get_cloud_hh_var(bmsg)) != NULL &&
					dba_msg_get_cloud_hh_var(amsg) == NULL)
				CHECKED(dba_msg_set_cloud_hh_var(amsg, var));

			if ((var = dba_msg_get_cloud_cl_var(bmsg)) != NULL &&
					dba_msg_get_cloud_cl_var(amsg) == NULL)
				CHECKED(dba_msg_set_cloud_cl_var(amsg, var));

			if ((var = dba_msg_get_cloud_cm_var(bmsg)) != NULL &&
					dba_msg_get_cloud_cm_var(amsg) == NULL)
				CHECKED(dba_msg_set_cloud_cm_var(amsg, var));

			if ((var = dba_msg_get_cloud_ch_var(bmsg)) != NULL &&
					dba_msg_get_cloud_ch_var(amsg) == NULL)
				CHECKED(dba_msg_set_cloud_ch_var(amsg, var));

#define FIX_GEOPOTENTIAL
#ifdef FIX_GEOPOTENTIAL
			// Convert the geopotentials back to heights in a dirty way, but it
			// should make the comparison more meaningful.  It catches this case:
			//  - AOF has height, that gets multiplied by 9.80665
			//    25667 becomes 251707
			//  - BUFR stores geopotential, without the last digit cifra
			//    251707 becomes 251710
			//  - However, if we go back to heights, the precision should be
			//    preserved
			//    251710 / 9.80664 becomes 25667 as it was
			for (int i = 0; i < amsg->data_count; i++)
			{
				dba_msg_level lev = amsg->data[i];
				for (int j = 0; j < lev->data_count; j++)
				{
					dba_msg_datum dat = lev->data[j];
					if (dba_var_code(dat->var) == DBA_VAR(0, 10, 3))
					{
						double dval;
						CHECKED(dba_var_enqd(dat->var, &dval));
						dval /= 9.80665;
						CHECKED(dba_var_setd(dat->var, dval));
					}
				}
			}
			for (int i = 0; i < bmsg->data_count; i++)
			{
				dba_msg_level lev = bmsg->data[i];
				for (int j = 0; j < lev->data_count; j++)
				{
					dba_msg_datum dat = lev->data[j];
					if (dba_var_code(dat->var) == DBA_VAR(0, 10, 3))
					{
						double dval;
						CHECKED(dba_var_enqd(dat->var, &dval));
						dval /= 9.80665;
						CHECKED(dba_var_setd(dat->var, dval));
					}
				}
			}

			// Decoding BUFR temp messages copies data from the surface level into
			// more classical places: compensate by copying the same data in the
			// AOF file
			{
				dba_var v;

				if ((v = dba_msg_get_press_var(bmsg)) != NULL)
					CHECKED(dba_msg_set_press_var(amsg, v));
				if ((v = dba_msg_get_temp_2m_var(bmsg)) != NULL)
					CHECKED(dba_msg_set_temp_2m_var(amsg, v));
				if ((v = dba_msg_get_dewpoint_2m_var(bmsg)) != NULL)
					CHECKED(dba_msg_set_dewpoint_2m_var(amsg, v));
				if ((v = dba_msg_get_wind_dir_var(bmsg)) != NULL)
					CHECKED(dba_msg_set_wind_dir_var(amsg, v));
				if ((v = dba_msg_get_wind_speed_var(bmsg)) != NULL)
					CHECKED(dba_msg_set_wind_speed_var(amsg, v));
			}
#endif
		}

		// Remove attributes from all vertical sounding significances
		for (int i = 0; i < bmsg->data_count; i++)
		{
			dba_msg_level lev = bmsg->data[i];
			for (int j = 0; j < lev->data_count; j++)
			{
				dba_msg_datum dat = lev->data[j];
				if (dba_var_code(dat->var) == DBA_VAR(0, 8, 1))
					dba_var_clear_attrs(dat->var);
			}
		}
	}
}

// Reencode to BUFR and compare
template<> template<>
void to::test<2>()
{
	const char* files[] = {
		"aof/obs1-11.0.aof",
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs2-244.0.aof",
		"aof/obs2-244.1.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs5-36.30.aof",
		"aof/obs6-32.1573.aof",
		"aof/obs6-32.0.aof",
		NULL,
	};

	for (size_t i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		dba_msgs amsgs = read_test_msg(files[i], AOF);
		
		dba_rawmsg raw;
		CHECKED(dba_marshal_encode(amsgs, BUFR, &raw));

		dba_msgs bmsgs;
		CHECKED(dba_marshal_decode(raw, &bmsgs));

		normalise_encoding_quirks(amsgs, bmsgs);

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(amsgs, bmsgs, &diffs, stderr);
		if (diffs) track_different_msgs(amsgs, bmsgs, "aof-bufr");
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(amsgs);
		dba_msgs_delete(bmsgs);
		dba_rawmsg_delete(raw);
	}
	test_untag();
}

// Reencode to CREX and compare
template<> template<>
void to::test<3>()
{
	const char* files[] = {
		"aof/obs1-11.0.aof",
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs2-244.0.aof",
		"aof/obs2-244.1.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs5-36.30.aof",
		"aof/obs6-32.1573.aof",
		"aof/obs6-32.0.aof",
		NULL,
	};

	for (size_t i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		dba_msgs amsgs = read_test_msg(files[i], AOF);
		
		dba_rawmsg raw;
		CHECKED(dba_marshal_encode(amsgs, CREX, &raw));

		dba_msgs bmsgs;
		CHECKED(dba_marshal_decode(raw, &bmsgs));

		strip_attributes(amsgs);
		normalise_encoding_quirks(amsgs, bmsgs);

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(amsgs, bmsgs, &diffs, stderr);
		if (diffs) track_different_msgs(amsgs, bmsgs, "aof-crex");
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(amsgs);
		dba_msgs_delete(bmsgs);
		dba_rawmsg_delete(raw);
	}
	test_untag();
}


// Compare decoding results with BUFR sample data
template<> template<>
void to::test<4>()
{
	string files[] = {
		"aof/obs1-14.63",		// OK
		"aof/obs1-21.1",		// OK
		"aof/obs1-24.2104",		// OK
//		"aof/obs1-24.34",		// Data are in fact slightly different
//		"aof/obs2-144.2198",	// Data have different precision in BUFR and AOF, but otherwise match
//		"aof/obs2-244.0",		// BUFR counterpart missing for this message
		"aof/obs4-165.2027",	// OK
//		"aof/obs5-35.61",		// Data are in fact slightly different
//		"aof/obs5-36.30",		// Data are in fact slightly different
		"aof/obs6-32.1573",		// OK
//		"aof/obs6-32.0",		// BUFR conterpart missing for this message
		"",
	};

	for (size_t i = 0; !files[i].empty(); i++)
	{
		test_tag(files[i]);

		dba_msgs amsgs = read_test_msg((files[i] + ".aof").c_str(), AOF);
		dba_msgs bmsgs = read_test_msg((files[i] + ".bufr").c_str(), BUFR);
		normalise_encoding_quirks(amsgs, bmsgs);

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(amsgs, bmsgs, &diffs, stderr);
		if (diffs) track_different_msgs(amsgs, bmsgs, "aof");
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(amsgs);
		dba_msgs_delete(bmsgs);
	}
	test_untag();
}

#if 0
	CHECKED(dba_aof_decoder_start(decoder, "test-aof-01"));
	while (1)
	{
		/* Read all the records */
		dba_err err = dba_aof_decoder_next(decoder, rec);
		if (err == DBA_ERROR && dba_error_get_code() == DBA_ERR_NOTFOUND)
			break;
		if (err) print_dba_error();
		fail_unless(err == DBA_OK);
		//dump_rec(rec);
	}
		
	CHECKED(dba_aof_decoder_start(decoder, "test-aof-02"));
	while (1)
	{
		/* Read all the records */
		dba_err err = dba_aof_decoder_next(decoder, rec);
		if (err == DBA_ERROR && dba_error_get_code() == DBA_ERR_NOTFOUND)
			break;
		if (err) print_dba_error();
		fail_unless(err == DBA_OK);
	}

	CHECKED(dba_aof_decoder_start(decoder, "test-aof-03"));
	while (1)
	{
		/* Read all the records */
		dba_err err = dba_aof_decoder_next(decoder, rec);
		if (err == DBA_ERROR && dba_error_get_code() == DBA_ERR_NOTFOUND)
			break;
		if (err) print_dba_error();
		fail_unless(err == DBA_OK);
	}

	CHECKED(dba_aof_decoder_start(decoder, "test-aof-04"));
	while (1)
	{
		/* Read all the records */
		dba_err err = dba_aof_decoder_next(decoder, rec);
		if (err == DBA_ERROR && dba_error_get_code() == DBA_ERR_NOTFOUND)
			break;
		if (err) print_dba_error();
		fail_unless(err == DBA_OK);
	}


	dba_record_delete(rec);
#endif

}

/* vim:set ts=4 sw=4: */
