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

#include <test-utils-core.h>
#include <dballe/core/rawmsg.h>

using namespace std;
using namespace dballe;

namespace tut {

struct rawmsg_shar
{
	rawmsg_shar()
	{
	}

	~rawmsg_shar()
	{
	}
};
TESTGRP(rawmsg);

// Basic generic tests
template<> template<>
void to::test<1>()
{
	Rawmsg msg;

	ensure(msg.file.empty());
	ensure_equals(msg.offset, 0);
	ensure_equals(msg.index, 0);
	ensure_equals(msg.size(), 0u);

	/* Resetting an empty message should do anything special */
	msg.clear();
	ensure(msg.file.empty());
	ensure_equals(msg.offset, 0);
	ensure_equals(msg.index, 0);
	ensure_equals(msg.size(), 0u);
}

}

/* vim:set ts=4 sw=4: */
