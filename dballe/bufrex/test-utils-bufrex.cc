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

#include "test-utils-bufrex.h"

#include <dballe/msg/aof_codec.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

extern "C" {
void bufrex_codec_init(void);
void bufrex_codec_shutdown(void);
}

namespace tut_dballe {

TestBufrexEnv::TestBufrexEnv()
{
	bufrex_codec_init();
}
TestBufrexEnv::~TestBufrexEnv()
{
	bufrex_codec_shutdown();
}

bufrex_msg _read_test_msg_header_raw(const char* file, int line, const char* filename, dba_encoding type)
{
	inner_ensure(type == BUFR || type == CREX);

	dba_rawmsg rawmsg = _read_rawmsg(file, line, filename, type);

	// Decode the sample message
	bufrex_msg bufrex;
	switch (type)
	{
		case BUFR: INNER_CHECKED(bufrex_msg_create(BUFREX_BUFR, &bufrex)); break;
		case CREX: INNER_CHECKED(bufrex_msg_create(BUFREX_CREX, &bufrex)); break;
		default: inner_ensure(false); break;
	}
	INNER_CHECKED(bufrex_msg_decode_header(bufrex, rawmsg));
	
	dba_rawmsg_delete(rawmsg);
	return bufrex;
}

bufrex_msg _read_test_msg_raw(const char* file, int line, const char* filename, dba_encoding type)
{
	inner_ensure(type == BUFR || type == CREX);

	dba_rawmsg rawmsg = _read_rawmsg(file, line, filename, type);

	// Decode the sample message
	bufrex_msg bufrex;
	switch (type)
	{
		case BUFR: INNER_CHECKED(bufrex_msg_create(BUFREX_BUFR, &bufrex)); break;
		case CREX: INNER_CHECKED(bufrex_msg_create(BUFREX_CREX, &bufrex)); break;
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
	INNER_CHECKED(bufrex_msg_create(msg->encoding_type, &bufrex));
	INNER_CHECKED(bufrex_msg_decode(bufrex, raw));

	dba_rawmsg_delete(raw);

	return bufrex;
}

}

// vim:set ts=4 sw=4:
