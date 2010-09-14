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

#include <test-utils-wreport.h>
#include <wreport/varinfo.h>
#include <cstring>

using namespace wreport;

namespace tut {

struct varinfo_shar
{
	varinfo_shar()
	{
	}

	~varinfo_shar()
	{
	}
};
TESTGRP(varinfo);

/* Test varcode encoding functions */
template<> template<>
void to::test<1>()
{
	ensure_equals(WR_VAR(0, 0, 0), 0);
	ensure_equals(WR_VAR(0, 0, 255), 0xff);
	ensure_equals(WR_VAR(0, 1, 0), 0x100);
	ensure_equals(WR_VAR(0, 63, 0), 0x3f00);
	ensure_equals(WR_VAR(0, 63, 255), 0x3fff);
	ensure_equals(WR_VAR(1, 0, 0), 0x4000);
	ensure_equals(WR_VAR(2, 0, 255), 0x80ff);
	ensure_equals(WR_VAR(3, 1, 0), 0xc100);
	ensure_equals(WR_VAR(1, 63, 0), 0x7f00);
	ensure_equals(WR_VAR(2, 63, 255), 0xbfff);
	ensure_equals(WR_VAR(3, 63, 255), 0xffff);

	ensure_equals(DBA_STRING_TO_VAR("12345"), WR_VAR(0, 12, 345));
	ensure_equals(DBA_STRING_TO_VAR("00345"), WR_VAR(0, 0, 345));
	ensure_equals(DBA_STRING_TO_VAR("00000"), WR_VAR(0, 0, 0));
	ensure_equals(DBA_STRING_TO_VAR("63255"), WR_VAR(0, 63, 255));

	ensure_equals(descriptor_code("B12345"), WR_VAR(0, 12, 345));
	ensure_equals(descriptor_code("R00345"), WR_VAR(1, 0, 345));
	ensure_equals(descriptor_code("C00000"), WR_VAR(2, 0, 0));
	ensure_equals(descriptor_code("D63255"), WR_VAR(3, 63, 255));
	ensure_equals(descriptor_code("012345"), WR_VAR(0, 12, 345));
	ensure_equals(descriptor_code("100345"), WR_VAR(1, 0, 345));
	ensure_equals(descriptor_code("200000"), WR_VAR(2, 0, 0));
	ensure_equals(descriptor_code("363255"), WR_VAR(3, 63, 255));
}

/* Test varcode alteration functions */
template<> template<>
void to::test<2>()
{
	Alteration a = DBA_ALT(1, 2);
	ensure_equals(DBA_ALT_WIDTH(a), 1);
	ensure_equals(DBA_ALT_SCALE(a), 2);

	a = DBA_ALT(-1, -2);
	ensure_equals(DBA_ALT_WIDTH(a), -1);
	ensure_equals(DBA_ALT_SCALE(a), -2);
}

/* Test instantiation of singleuse Varinfos */
template<> template<>
void to::test<3>()
{
	MutableVarinfo info = MutableVarinfo::create_singleuse(WR_VAR(2, 20, 0));
	ensure_equals(info->var, WR_VAR(2, 20, 0));
}

/* Test the calculation of bounds */
template<> template<>
void to::test<4>()
{
	MutableVarinfo info = MutableVarinfo::create_singleuse(WR_VAR(0, 15, 194));
	info->set(WR_VAR(0, 15, 194),		// Var
		  "[SIM] O3 Concentration",	// Desc
		  "KG/M**3",			// Unit
		  10, 0, 5, 0, 17);		// Scale, ref, len, bit_ref, bit_len
	info->compute_range();
	ensure_equals(info->dmin, 0);
	ensure_equals(info->dmax, 9.9998e-06);
	ensure(!info->is_string());
}

/* Test encoding doubles to ints */
template<> template<>
void to::test<5>()
{
	MutableVarinfo info = MutableVarinfo::create_singleuse(WR_VAR(0, 6, 2));
	info->set(WR_VAR(0, 6, 2),		// Var
		  "LONGITUDE (COARSE ACCURACY)",// Desc
		  "DEGREE",			// Unit
		  2, 0, 5, -18000, 16);		// Scale, ref, len, bit_ref, bit_len
	info->bufr_scale = 2;
	info->compute_range();
	ensure_equals(info->dmin, -180);
	ensure_equals(info->dmax, 475.34);
	ensure(!info->is_string());
	// ensure_equals(info->decode_int(16755), -12.45);
	ensure_equals(info->bufr_decode_int(16755), -12.45);
}

}

/* vim:set ts=4 sw=4: */
