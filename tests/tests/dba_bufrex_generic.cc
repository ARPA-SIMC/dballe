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

}

/* vim:set ts=4 sw=4: */
