#include <tests/test-utils.h>
#include <dballe/dba_file.h>
#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/msg/dba_msg.h>

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

template<> template<>
void to::test<1>()
{
	dba_file file_gen;
	dba_file file_synop;
	int gfound, bfound;
	dba_msg gen, synop;

	CHECKED(dba_file_create(&file_gen, BUFR, "bufr/gen-generic.bufr", "r"));
	CHECKED(dba_file_create(&file_synop, BUFR, "bufr/gen-synop.bufr", "r"));

	CHECKED(dba_file_read(file_gen, &gen, &gfound));
	CHECKED(dba_file_read(file_synop, &synop, &bfound));
	gen_ensure_equals(gfound, bfound);
	do {
		gen_ensure_equals(gen->type, MSG_GENERIC);

		/* Export gen as a synop message */
		/* TODO: use the same template as the other synop message */
		dba_rawmsg raw;
		gen->type = MSG_SYNOP;
		CHECKED(bufrex_encode_bufr(gen, 0, 0, &raw));

		/* Parse the second dba_rawmsg */
		dba_msg synop1;
		CHECKED(bufrex_decode_bufr(raw, &synop1));

		/* Check that the data are the same */
		int diffs = 0;
		dba_msg_diff(synop, synop1, &diffs, stderr);

#if 0
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

			FILE* out1 = fopen("/tmp/synop.txt", "w");
			FILE* out2 = fopen("/tmp/synop1.txt", "w");
				
			dba_msg_print(synop, out1);
			dba_msg_print(synop1, out2);
			fclose(out1);
			fclose(out2);
		}
#endif
		
		gen_ensure_equals(diffs, 0);

		dba_msg_delete(gen);
		dba_msg_delete(synop);
		dba_msg_delete(synop1);
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
void to::test<2>()
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

	/* Encode the message */
	dba_rawmsg raw;
	CHECKED(bufrex_encode_bufr(msg, 0, 0, &raw));

	/* Decode the message */
	dba_msg msg1;
	CHECKED(bufrex_decode_bufr(raw, &msg1));
	dba_rawmsg_delete(raw);

	/* Check that everything is still there */
	int diffs = 0;
	dba_msg_diff(msg, msg1, &diffs, stderr);
	gen_ensure_equals(diffs, 0);

	dba_msg_delete(msg);
	dba_msg_delete(msg1);
}

}

/* vim:set ts=4 sw=4: */
