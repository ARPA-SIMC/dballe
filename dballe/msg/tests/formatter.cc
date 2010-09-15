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

#include <test-utils-msg.h>
#include <dballe/msg/formatter.h>
#include <dballe/msg/defs.h>

using namespace dballe;
using namespace std;

namespace tut {

struct formatter_shar
{
};
TESTGRP(formatter);

// Try to get descriptions for all the layers
template<> template<>
void to::test<1>()
{
	for (int i = 0; i < 261; ++i)
	{
		describe_level(i, 0);
		describe_level_or_layer(i, 0, i, 0);
	}
}

// Try to get descriptions for all the time ranges
template<> template<>
void to::test<2>()
{
	for (int i = 0; i < 256; ++i)
		describe_trange(i, 0, 0);
}

// Verify some well-known descriptions
template<> template<>
void to::test<3>()
{
	ensure_equals(describe_level(MISSING_INT, MISSING_INT), "-");
	ensure_equals(describe_level_or_layer(103, 2000, INT_MAX, INT_MAX), "2.000m above ground");
	ensure_equals(describe_level_or_layer(103, 2000, 103, 4000),
			"Layer from [2.000m above ground] to [4.000m above ground]");
	ensure_equals(describe_trange(254, 86400, MISSING_INT),
			"Instantaneous value, forecast time 1d");
}

}

/* vim:set ts=4 sw=4: */
