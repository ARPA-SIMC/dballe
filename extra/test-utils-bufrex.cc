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

#include "test-utils.h"

#include <dballe/msg/aof_codec.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace tut_dballe {

bufrex_msg _read_test_msg_raw(const char* file, int line, const char* filename, dba_encoding type)
{
	const char* testdatadirenv = getenv("DBA_TESTDATA");
	std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
	dba_file input;
	dba_rawmsg rawmsg;
	int found;

	inner_ensure(type == BUFR || type == CREX);

	// Read the sample message
	INNER_CHECKED(dba_rawmsg_create(&rawmsg));
	INNER_CHECKED(dba_file_create(type, (testdatadir + "/" + filename).c_str(), "r", &input));
	INNER_CHECKED(dba_file_read(input, rawmsg, &found));
	inner_ensure_equals(found, 1);

	dba_file_delete(input);

	// Decode the sample message
	bufrex_msg bufrex;
	switch (type)
	{
		case BUFR: INNER_CHECKED(bufrex_msg_create(&bufrex, BUFREX_BUFR)); break;
		case CREX: INNER_CHECKED(bufrex_msg_create(&bufrex, BUFREX_CREX)); break;
		default: inner_ensure(false); break;
	}
	INNER_CHECKED(bufrex_msg_decode(bufrex, rawmsg));
	
	dba_rawmsg_delete(rawmsg);
	return bufrex;
}

bufrex_msg _reencode_test(const char* file, int line, bufrex_msg msg)
{
	dba_rawmsg raw;
	INNER_CHECKED(bufrex_msg_encode(msg, &raw));

	bufrex_msg bufrex;
	INNER_CHECKED(bufrex_msg_create(&bufrex, msg->encoding_type));
	INNER_CHECKED(bufrex_msg_decode(bufrex, raw));

	dba_rawmsg_delete(raw);

	return bufrex;
}

}
