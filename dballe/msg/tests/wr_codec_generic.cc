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
#include "msg/wr_codec.h"
#include <wreport/notes.h>
#include <wreport/bulletin.h>

using namespace dballe;
using namespace wreport;
using namespace std;

namespace tut {

struct wr_codec_generic_shar
{
};
TESTGRP(wr_codec_generic);

// Try encoding and decoding an empty generic message
template<> template<>
void to::test<1>()
{
    auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR);
    auto_ptr<msg::Exporter> exporter = msg::Exporter::create(BUFR);

	Msgs msgs;
    msgs.acquire(auto_ptr<Msg>(new Msg));

	/* Export msg as a generic message */
	Rawmsg raw;
    exporter->to_rawmsg(msgs, raw);

	/* Parse it back */
	Msgs msgs1;
    importer->from_rawmsg(raw, msgs1);

    /* Check that the data are the same */
    notes::Collect c(cerr);
    int diffs = msgs.diff(msgs1);
    if (diffs) dballe::tests::track_different_msgs(msgs, msgs1, "genericempty");
    ensure_equals(diffs, 0);
}

// Try encoding and decoding a generic message
template<> template<>
void to::test<2>()
{
    auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR);
    auto_ptr<msg::Exporter> exporter = msg::Exporter::create(BUFR);

    auto_ptr<Msg> msg(new Msg);

	/* Fill up msg */
	msg->set_press(			15,	45);
	msg->set_height_anem(	15,	45);
	msg->set_tot_snow(		15,	45);
	msg->set_visibility(		15,	45);
	msg->set_pres_wtr(		 5,	45);
	msg->set_metar_wtr(		 5,	45);
	msg->set_water_temp(		15,	45);
	msg->set_past_wtr1_3h(	 2,	45);
	msg->set_past_wtr2_3h(	 2,	45);
	msg->set_press_tend(		 5,	45);
	msg->set_tot_prec24(		15,	45);
	msg->set_press_3h(		15,	45);
	msg->set_press_msl(		15,	45);
	msg->set_qnh(			15,	45);
	msg->set_temp_2m(		15,	45);
	msg->set_wet_temp_2m(	15,	45);
	msg->set_dewpoint_2m(	15,	45);
	msg->set_humidity(		15,	45);
	msg->set_wind_dir(		15,	45);
	msg->set_wind_speed(		15,	45);
	msg->set_ex_ccw_wind(	15,	45);
	msg->set_ex_cw_wind(		15,	45);
	msg->set_wind_gust_max_speed(	15,	45);
	msg->set_cloud_n(		3,		45);
	msg->set_cloud_nh(		10,	45);
	msg->set_cloud_hh(		3,		45);
	msg->set_cloud_cl(		3,		45);
	msg->set_cloud_cm(		3,		45);
	msg->set_cloud_ch(		3,		45);
	msg->set_cloud_n1(		3,		45);
	msg->set_cloud_c1(		3,		45);
	msg->set_cloud_h1(		3,		45);
	msg->set_cloud_n2(		3,		45);
	msg->set_cloud_c2(		3,		45);
	msg->set_cloud_h2(		3,		45);
	msg->set_cloud_n3(		3,		45);
	msg->set_cloud_c3(		3,		45);
	msg->set_cloud_h3(		3,		45);
	msg->set_cloud_n4(		3,		45);
	msg->set_cloud_c4(		3,		45);
	msg->set_cloud_h4(		3,		45);
	msg->set_block(			3,		45);
	msg->set_station(		3,		45);
	msg->set_flight_reg_no(	"pippo", 45);
	msg->set_ident(			"cippo", 45);
	msg->set_st_dir(			3,		45);
	msg->set_st_speed(		3,		45);
	msg->set_st_name(		"ciop", 45);
	msg->set_st_name_icao(	"cip", 45);
	msg->set_st_type(		1,		45);
	msg->set_wind_inst(		3,		45);
	msg->set_temp_precision(	1.23,	45);
	msg->set_sonde_type(		3,		45);
	msg->set_sonde_method(	3,		45);
	msg->set_navsys(			3,		45);
	msg->set_data_relay(		3,		45);
	msg->set_flight_roll(	3,		45);
	msg->set_latlon_spec(	3,		45);
	msg->set_year(			3,		45);
	msg->set_month(			3,		45);
	msg->set_day(			3,		45);
	msg->set_hour(			3,		45);
	msg->set_minute(			3,		45);
	msg->set_latitude(		3,		45);
	msg->set_longitude(		3,		45);
	msg->set_height_station(3,		45);
	msg->set_height_baro(	3,		45);
	msg->set_flight_phase(	3,		45);
	msg->set_timesig(		3,		45);
	//CHECKED(dba_msg_set_flight_press(	msg, 3,		45));

	Msgs msgs;
    msgs.acquire(msg);

	/* Export msg as a generic message */
	Rawmsg raw;
    exporter->to_rawmsg(msgs, raw);

	/* Parse it back */
    Msgs msgs1;
    importer->from_rawmsg(raw, msgs1);

    /* Check that the data are the same */
    notes::Collect c(cerr);
    int diffs = msgs.diff(msgs1);
    if (diffs) dballe::tests::track_different_msgs(msgs, msgs1, "generic2");
    ensure_equals(diffs, 0);
}

/*
 * Iterate two exports of the same data, one as synop and the other as generic.
 *
 * Test that the generic data, when encoded as synop, contains the same
 * information as the synop export.
 */
template<> template<>
void to::test<3>()
{
#if 0
  -- Disabled until we regenerate generic datasets with 4-element levels

	dba_file file_gen = open_test_data("bufr/gen-generic.bufr", BUFR);
	dba_file file_synop = open_test_data("bufr/gen-synop.bufr", BUFR);
	int gfound, bfound, count = 0;
	dba_msgs gen, synop;

	CHECKED(dba_file_read_msgs(file_gen, &gen, &gfound));
	CHECKED(dba_file_read_msgs(file_synop, &synop, &bfound));
	ensure_equals(gfound, bfound);
	do {
		count++;
		ensure_equals(gen->msgs[0]->type, MSG_GENERIC);

		/* Export gen as a synop message */
		/* TODO: use the same template as the other synop message */
		dba_rawmsg raw;
		gen->msgs[0]->type = MSG_SYNOP;
		CHECKED(bufrex_encode_bufr(gen, 0, 0, 0, &raw));

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

		ensure_equals(diffs, 0);

		dba_msgs_delete(gen);
		dba_msgs_delete(synop);
		dba_msgs_delete(synop1);
		CHECKED(dba_file_read_msgs(file_gen, &gen, &gfound));
		CHECKED(dba_file_read_msgs(file_synop, &synop, &bfound));
		ensure_equals(gfound, bfound);
	} while (gfound);

	dba_file_delete(file_gen);
	dba_file_delete(file_synop);
#endif
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */

/* Check that attributes are properly exported */
template<> template<>
void to::test<4>()
{
    auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR);
    auto_ptr<msg::Exporter> exporter = msg::Exporter::create(BUFR);

	/* Create a new message */
    auto_ptr<Msg> msg(new Msg);
	msg->type = MSG_GENERIC;

	/* Set some metadata */
	msg->set_year(2006);
	msg->set_month(1);
	msg->set_day(19);
	msg->set_hour(14);
	msg->set_minute(50);
	msg->set_latitude(50.0);
	msg->set_longitude(12.0);

	/* Create a variable to add to the message */
	auto_ptr<Var> var = newvar(WR_VAR(0, 12, 101), 270.15);

	/* Add some attributes to the variable */
	var->seta(newvar(WR_VAR(0, 33, 2), 1));
	var->seta(newvar(WR_VAR(0, 33, 3), 2));
	var->seta(newvar(WR_VAR(0, 33, 5), 3));

	/* Add the variable to the message */
    msg->set(var, Level(1), Trange::instant());

	/* Create a second variable to add to the message */
    var = newvar(WR_VAR(0, 12, 2), 272.0);

	/* Add some attributes to the variable */
	var->seta(newvar(WR_VAR(0, 33, 3), 1));
	var->seta(newvar(WR_VAR(0, 33, 5), 2));

	/* Add the variable to the message */
    msg->set(var, Level(1), Trange::instant());

	Msgs msgs;
    msgs.acquire(msg);

	/* Encode the message */
	Rawmsg raw;
    exporter->to_rawmsg(msgs, raw);

	/* Decode the message */
    Msgs msgs1;
    importer->from_rawmsg(raw, msgs1);

    /* Check that the data are the same */
    notes::Collect c(cerr);
    int diffs = msgs.diff(msgs1);
    if (diffs) dballe::tests::track_different_msgs(msgs, msgs1, "genericattr");
    ensure_equals(diffs, 0);
}

// Test a bug in which B01194 ([SIM] Report mnemonic) appears twice
template<> template<>
void to::test<5>()
{
    // Import a synop message
    auto_ptr<Msgs> msgs = read_msgs("bufr/obs0-1.22.bufr", BUFR);
    ensure(msgs->size() > 0);

    // Convert it to generic, with a 'ship' rep_memo
    (*msgs)[0]->type = MSG_GENERIC;
    (*msgs)[0]->set_rep_memo("ship");

    // Export it
    auto_ptr<msg::Exporter> exporter = msg::Exporter::create(BUFR);
    auto_ptr<Bulletin> bulletin(BufrBulletin::create());
    exporter->to_bulletin(*msgs, *bulletin);

    // Ensure that B01194 only appears once
    ensure_equals(bulletin->subsets.size(), 1u);
    unsigned count = 0;
    for (std::vector<wreport::Var>::const_iterator i = bulletin->subsets[0].begin(); i != bulletin->subsets[0].end(); ++i)
    {
        if (i->code() == WR_VAR(0, 1, 194))
            ++count;
    }
    ensure_equals(count, 1u);
}

}

/* vim:set ts=4 sw=4: */
