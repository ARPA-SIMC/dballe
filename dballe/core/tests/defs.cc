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
#include <dballe/core/defs.h>

using namespace dballe;
using namespace std;

namespace tut {

struct defs_shar
{
};
TESTGRP(defs);

// Try to get descriptions for all the layers
template<> template<>
void to::test<1>()
{
	for (int i = 0; i < 261; ++i)
	{
		Level(i).describe();
		Level(i, 0).describe();
		Level(i, MISSING_INT, i, MISSING_INT).describe();
		Level(i, 0, i, 0).describe();
	}
}

// Try to get descriptions for all the time ranges
template<> template<>
void to::test<2>()
{
	for (int i = 0; i < 256; ++i)
	{
		Trange(i).describe();
		Trange(i, 0).describe();
		Trange(i, 0, 0).describe();
	}
}

// Verify some well-known descriptions
template<> template<>
void to::test<3>()
{
	ensure_equals(Level().describe(), "-");
	ensure_equals(Level(103, 2000).describe(), "2.000m above ground");
	ensure_equals(Level(103, 2000, 103, 4000).describe(),
			"Layer from [2.000m above ground] to [4.000m above ground]");
	ensure_equals(Trange(254, 86400).describe(),
			"Instantaneous value");
	ensure_equals(Trange(2, 0, 43200).describe(), "Maximum over 12h at forecast time 0");
	ensure_equals(Trange(3, 194400, 43200).describe(), "Minimum over 12h at forecast time 2d 6h");
}

}

/* vim:set ts=4 sw=4: */
