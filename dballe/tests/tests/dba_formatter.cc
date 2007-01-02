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
#include <dballe/formatter.h>

namespace tut {
using namespace tut_dballe;

struct dba_formatter_shar
{
};
TESTGRP(dba_formatter);

// Try to get descriptions for all the layers
template<> template<>
void to::test<1>()
{
	for (int i = 0; i < 258; ++i)
	{
		char* t = 0;
		CHECKED(dba_formatter_describe_level(i, 0, 0, &t));
		gen_ensure(t != 0);
		free(t);
	}
}

// Try to get descriptions for all the time ranges
template<> template<>
void to::test<2>()
{
	for (int i = 0; i < 256; ++i)
	{
		char* t = 0;
		CHECKED(dba_formatter_describe_trange(i, 0, 0, &t));
		gen_ensure(t != 0);
		free(t);
	}
}


}

/* vim:set ts=4 sw=4: */
