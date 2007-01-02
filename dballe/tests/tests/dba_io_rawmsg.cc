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
#include <dballe/core/rawmsg.h>

namespace tut {
using namespace tut_dballe;

struct dba_io_rawmsg_shar
{
	dba_io_rawmsg_shar()
	{
	}

	~dba_io_rawmsg_shar()
	{
	}
};
TESTGRP(dba_io_rawmsg);

// Basic generic tests
template<> template<>
void to::test<1>()
{
	int val;
	const unsigned char* buf;
	dba_rawmsg msg;
	
	CHECKED(dba_rawmsg_create(&msg));
	gen_ensure_equals(msg->offset, 0);
	gen_ensure_equals(msg->index, 0);
	gen_ensure_equals(msg->buf, (unsigned const char*)0);
	gen_ensure_equals(msg->len, 0);
	gen_ensure_equals(msg->alloclen, 0);

	/* The message should be properly empty */
	gen_ensure_equals(msg->offset, 0);
	CHECKED(dba_rawmsg_get_raw(msg, &buf, &val));
	gen_ensure_equals(buf, (unsigned const char*)0);
	gen_ensure_equals(val, 0);

	/* Resetting an empty message should do anything special */
	dba_rawmsg_reset(msg);
	gen_ensure_equals(msg->offset, 0);
	gen_ensure_equals(msg->index, 0);
	gen_ensure_equals(msg->buf, (unsigned const char*)0);

	/* The message should still be properly empty */
	gen_ensure_equals(msg->offset, 0);
	CHECKED(dba_rawmsg_get_raw(msg, &buf, &val));
	gen_ensure_equals(buf, (unsigned const char*)0);
	gen_ensure_equals(val, 0);

	/* Expanding the buffer should allocate something */
	CHECKED(dba_rawmsg_expand_buffer(msg));
	gen_ensure_equals(msg->offset, 0);
	gen_ensure_equals(msg->index, 0);
	gen_ensure_equals(msg->len, 0);
	gen_ensure(msg->buf != NULL);
	gen_ensure(msg->alloclen > 0);

	/* Resetting should not deallocate the buffer */
	msg->offset = msg->index = msg->len = 42;
	dba_rawmsg_reset(msg);
	gen_ensure_equals(msg->offset, 0);
	gen_ensure_equals(msg->index, 0);
	gen_ensure_equals(msg->len, 0);
	gen_ensure(msg->buf != NULL);
	gen_ensure(msg->alloclen > 0);

	dba_rawmsg_delete(msg);
}

}

/* vim:set ts=4 sw=4: */
