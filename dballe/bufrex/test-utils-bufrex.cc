/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/bufrex/codec.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace dballe;

namespace bufrex {
namespace tests {

TestBufrexEnv::TestBufrexEnv()
{
	bufrex::codec_init();
}
TestBufrexEnv::~TestBufrexEnv()
{
	bufrex::codec_shutdown();
}

std::auto_ptr<bufrex::Msg> _read_test_msg_header_raw(const wibble::tests::Location& loc, const char* filename, Encoding type)
{
	inner_ensure(type == BUFR || type == CREX);

	std::auto_ptr<Rawmsg> rmsg = inner_read_rawmsg(filename, type);

	// Decode the sample message
	std::auto_ptr<bufrex::Msg> res;
	switch (type)
	{
		case BUFR: res.reset(new BufrMsg); break;
		case CREX: res.reset(new CrexMsg); break;
		default: inner_ensure(false); break;
	}
	res->decode_header(*rmsg);

	return res;
}

std::auto_ptr<bufrex::Msg> _read_test_msg_raw(const wibble::tests::Location& loc, const char* filename, Encoding type)
{
	inner_ensure(type == BUFR || type == CREX);

	std::auto_ptr<Rawmsg> rmsg = inner_read_rawmsg(filename, type);

	// Decode the sample message
	std::auto_ptr<bufrex::Msg> res = Msg::create(type);
	res->decode(*rmsg);
	return res;
}

std::auto_ptr<bufrex::Msg> _reencode_test(const wibble::tests::Location& loc, const bufrex::Msg& msg)
{
	Rawmsg raw;
	msg.encode(raw);

	std::auto_ptr<bufrex::Msg> res = Msg::create(raw.encoding);
	res->decode(raw);
	return res;
}

}
}

// vim:set ts=4 sw=4:
