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

#include <tests/test-utils.h>
#include <dballe/core/rawfile.h>

namespace tut {
using namespace tut_dballe;

struct dba_io_rawfile_shar
{
	dba_io_rawfile_shar()
	{
	}

	~dba_io_rawfile_shar()
	{
	}
};
TESTGRP(dba_io_rawfile);

// Trivial create test
template<> template<>
void to::test<1>()
{
	dba_rawfile file;

	/* Create the file reader */
	CHECKED(dba_rawfile_create(&file, "(stdin)", "r"));

	dba_rawfile_delete(file);
}

#if 0
static void test_parse(const char* src, int line, const char* fname, struct bufr_match* m)
{
	bufr_message msg;
	int found;
	char* str;
	int val;

	/* Create the message to test */
	CHECKED(bufr_message_create(&msg));

	/* Resetting an empty message should do no harm */
	bufr_message_reset(msg);

	/* Read the data from file */
	CHECKED(bufr_file_read(file, msg, &found));
	tb_fail_unless(found == 1);

	/* Parse it */
	CHECKED(bufr_message_parse(msg));

	/* Check the parsed values */
	test_bufr(src, line, msg, m);

	/* Try reencoding it */
	bufr_message_reset_encoded(msg);
	CHECKED(bufr_message_encode(msg));
	CHECKED(bufrex_message_get_raw(msg, &str, &val));
	fail_unless(val != 0);
	fail_unless(str[0] != 0);

	/* fprintf(stderr, "Encoded:\n - - -\n%.*s\n - - -\n", val, str); */

	/* Try reparsing it */
	bufr_message_reset_decoded(msg);
	CHECKED(bufr_message_parse(msg));

	/* Check the reparsed values */
	test_bufr(src, line, msg, m);

	bufr_message_delete(msg);
	bufr_file_delete(file);
}
#endif

}

/* vim:set ts=4 sw=4: */
