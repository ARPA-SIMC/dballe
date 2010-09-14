/*
 * wreport/varinfo - Variable information
 *
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

#include "config.h"

#include <strings.h>		/* bzero */
#include <cstring>		/* strcmp */
#include <math.h>		/* rint */
#include <limits.h>		/* INT_MIN, INT_MAX */

#include "varinfo.h"
#include "aliases.h"

namespace wreport {

Varcode descriptor_code(const char* entry)
{
	int res = 0;
	switch (entry[0])
	{
		case 'B':
		case '0':
			res = 0; break;
		case 'R':
		case '1':
			res = 1 << 14; break;
		case 'C':
		case '2':
			res = 2 << 14; break;
		case 'D':
		case '3':
			res = 3 << 14; break;
		default:
			return varcode_alias_resolve(entry);
	}
	return res | DBA_STRING_TO_VAR(entry+1);
}

_Varinfo::_Varinfo()
	: _ref(0)
{
}

MutableVarinfo MutableVarinfo::create_singleuse(Varcode code)
{
	MutableVarinfo res(new _Varinfo);
	res->reset();
	res->var = code;
	return res;
}

void _Varinfo::reset()
{
	var = 0;
	bzero(desc, sizeof(desc));
	bzero(unit, sizeof(unit));
	scale = 0;
	ref = 0;
	len = 0;
	bit_ref = 0;
	bit_len = 0;
	flags = 0;
	imin = imax = 0;
	dmin = dmax = 0.0;
	alteration = 0;
	alterations = 0;
	bzero(bufr_unit, sizeof(bufr_unit));
	bufr_scale = 0;
}

void _Varinfo::set(Varcode var, const char* desc, const char* unit, int scale, int ref, int len, int bit_ref, int bit_len, int flags, const char* bufr_unit, int bufr_scale)
{
	this->var = var;
	strncpy(this->desc, desc, 64);
	strncpy(this->unit, unit, 24);
	this->scale = scale;
	this->ref = ref;
	this->len = len;
	this->bit_ref = bit_ref;
	this->bit_len = bit_len;
	this->flags = flags;
	this->alteration = 0;
	this->alterations = 0;
	strncpy(this->bufr_unit, bufr_unit ? bufr_unit : unit, 24);
	this->bufr_scale = bufr_scale;

	compute_range();
}

/* Postprocess the data, filling in minval and maxval */
void _Varinfo::compute_range()
{
	if (is_string())
	{
		imin = imax = 0;
		dmin = dmax = 0.0;
	} else {
		if (len >= 10)
		{
			imin = INT_MIN;
			imax = INT_MAX;
		} else {
			int bufr_min = bit_ref;
			int bufr_max = exp2(bit_len) + bit_ref;
			// We subtract 2 because 2^bit_len-1 is the
			// BUFR missing value.
			// We cannot subtract 2 from the delayed replication
			// factors because RADAR BUFR messages have 255
			// subsets, and the delayed replication field is 8
			// bits, so 255 is the missing value, and if we
			// disallow it here we cannot import radars anymore.
			if (WR_VAR_X(var) != 31)
				bufr_max -= 2;
			// We subtract 2 because 10^len-1 is the
			// CREX missing value
			int crex_min = -(int)(exp10(len) - 1.0);
			int crex_max = (int)(exp10(len) - 2.0);
			/*
			 * If the unit is the same between BUFR and CREX, take
			 * the most restrictive extremes.
			 *
			 * If the unit is different, take the most permissive
			 * extremes, to make sure to fit values in both units
			 */
			if (strcmp(unit, bufr_unit) == 0)
			{
				imin = bufr_min > crex_min ? bufr_min : crex_min;
				imax = bufr_max < crex_max ? bufr_max : crex_max;
			} else {
				imin = bufr_min < crex_min ? bufr_min : crex_min;
				imax = bufr_max > crex_max ? bufr_max : crex_max;
			}
			/*
			i->imin = i->bit_ref;
			i->imax = exp2(i->bit_len) + i->bit_ref - 2;
			*/
			//i->imin = -(int)(exp10(i->len) - 1.0);
			//i->imax = (int)(exp10(i->len) - 1.0);
		}
		dmin = decode_int(imin);
		dmax = decode_int(imax);
	}
}

static const double scales[] = {
	1.0,
	10.0,
	100.0,
	1000.0,
	10000.0,
	100000.0,
	1000000.0,
	10000000.0,
	100000000.0,
	1000000000.0,
	10000000000.0,
	100000000000.0,
	1000000000000.0,
	10000000000000.0,
	100000000000000.0,
	1000000000000000.0,
	10000000000000000.0,
};

/* Decode a double value from its integer representation and Varinfo encoding
 * informations */
double _Varinfo::decode_int(int val) const throw ()
{
	if (scale > 0)
		return (val - ref) / scales[scale];
	else if (scale < 0)
		return (val - ref) * scales[-scale];
	else
		return val - ref;
}

double _Varinfo::bufr_decode_int(uint32_t ival) const throw ()
{
       if (bufr_scale >= 0)
               return ((double)ival + bit_ref) / scales[bufr_scale];
       else
               return ((double)ival + bit_ref) * scales[-bufr_scale];
}

/* Encode a double value from its integer representation and Varinfo encoding
 * informations */
int _Varinfo::encode_int(double fval) const throw ()
{
	if (scale > 0)
		return (int)rint((fval + ref) * scales[scale]);
	else if (scale < 0)
		return (int)rint((fval + ref) / scales[-scale]);
	else
		return (int)rint(fval + ref);
}

}

/* vim:set ts=4 sw=4: */
