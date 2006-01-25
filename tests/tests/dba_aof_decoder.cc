#include <tests/test-utils.h>
#include <dballe/aof/aof_decoder.h>
#include <dballe/dba_file.h>

namespace tut {
using namespace tut_dballe;

struct dba_aof_decoder_shar
{
	dba_aof_decoder_shar()
	{
	}

	~dba_aof_decoder_shar()
	{
	}
};
TESTGRP(dba_aof_decoder);


// Test simple decoding
template<> template<>
void to::test<1>()
{
	const char* files[] = {
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs2-244.0.aof",
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
		CHECKED(dba_file_create(&file, AOF, files[i], "r"));

		/* Read the file header */
		/* CHECKED(aof_file_read_header(file, 0, 0)); */

		/* Read the data from file */
		CHECKED(dba_file_read_raw(file, raw, &found));
		gen_ensure_equals(found, 1);

		/* Parse it */
		{
			dba_msg dmsg = NULL;
			CHECKED(aof_decoder_decode(raw, &dmsg));
			gen_ensure(dmsg != NULL);

			dba_msg_delete(dmsg);
		}
		dba_file_delete(file);
	}
	test_untag();

	dba_rawmsg_delete(raw);

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

}

/* vim:set ts=4 sw=4: */
