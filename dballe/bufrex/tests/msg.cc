/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-bufrex.h>
#include <dballe/bufrex/msg.h>

namespace tut {
using namespace tut_dballe;

struct bufrex_msg_shar
{
	TestBufrexEnv testenv;

	bufrex_msg_shar()
	{
	}

	~bufrex_msg_shar()
	{
	}
};
TESTGRP(bufrex_msg);

template<> template<>
void to::test<1>()
{
	int c, sc, lc;

	CHECKED(bufrex_msg_parse_template("1.2.3", &c, &sc, &lc));
	gen_ensure_equals(c, 1);
	gen_ensure_equals(sc, 2);
	gen_ensure_equals(lc, 3);

	CHECKED(bufrex_msg_parse_template("generic", &c, &sc, &lc));
	gen_ensure_equals(c, 255);
	gen_ensure_equals(sc, 255);
	gen_ensure_equals(lc, 0);

	CHECKED(bufrex_msg_parse_template("synop", &c, &sc, &lc));
	gen_ensure_equals(c, 0);
	gen_ensure_equals(sc, 255);
	gen_ensure_equals(lc, 1);

	CHECKED(bufrex_msg_parse_template("temp", &c, &sc, &lc));
	gen_ensure_equals(c, 2);
	gen_ensure_equals(sc, 255);
	gen_ensure_equals(lc, 101);
}

}

// vim:set ts=4 sw=4:
