/*
 * wreport/conv - Unit conversions
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

#include "conv.h"

#include <math.h>

/*
Cloud type VM		Cloud type 20012
WMO code 0509
Ch	0			10
Ch	1			11
Ch	...			...
Ch	9			19
Ch	/			60

WMO code 0515
Cm	0			20
Cm	1			21
Cm	...			...
Cm	9			29
Cm	/			61

WMO code 0513
Cl	0			30
Cl	1			31
Cl	...			...
Cl	9			39
Cl	/			62

				missing value: 63

WMO code 0500
Per i cloud type nei 4 gruppi ripetuti del synop:
	0..9 -> 0..9
	/ -> 59
*/

#if 0
dba_err dba_convert_WMO0500_to_BUFR20012(int from, int* to)
{
	if (from >= 0 && from <= 9)
		*to = from;
	else if (from == -1) /* FIXME: check what is the value for '/' */
		*to = 59;
	else
		return dba_error_notfound("value %d not found in WMO code table 0500", from);
	return dba_error_ok();
}

dba_err dba_convert_BUFR20012_to_WMO0500(int from, int* to)
{
	if (from >= 0 && from <= 9)
		*to = from;
	else if (from == 59)
		*to = -1; /* FIXME: check what is the value for '/' */
	else
		return dba_error_notfound(
				"BUFR 20012 value %d cannot be represented with WMO code table 0500",
				from);
	return dba_error_ok();
}

dba_err dba_convert_WMO0509_to_BUFR20012(int from, int* to)
{
	if (from >= 0 && from <= 9)
		*to = from + 10;
	else if (from == -1) /* FIXME: check what is the value for '/' */
		*to = 60;
	else
		return dba_error_notfound("value %d not found in WMO code table 0509", from);
	return dba_error_ok();
}

dba_err dba_convert_BUFR20012_to_WMO0509(int from, int* to)
{
	if (from >= 10 && from <= 19)
		*to = from - 10;
	else if (from == 60)
		*to = -1; /* FIXME: check what is the value for '/' */
	else
		return dba_error_notfound(
				"BUFR 20012 value %d cannot be represented with WMO code table 0509",
				from);
	return dba_error_ok();
}

dba_err dba_convert_WMO0515_to_BUFR20012(int from, int* to)
{
	if (from >= 0 && from <= 9)
		*to = from + 20;
	else if (from == -1) /* FIXME: check what is the value for '/' */
		*to = 61;
	else
		return dba_error_notfound("value %d not found in WMO code table 0515", from);
	return dba_error_ok();
}

dba_err dba_convert_BUFR20012_to_WMO0515(int from, int* to)
{
	if (from >= 20 && from <= 29)
		*to = from - 20;
	else if (from == 61)
		*to = -1; /* FIXME: check what is the value for '/' */
	else
		return dba_error_notfound(
				"BUFR 20012 value %d cannot be represented with WMO code table 0515",
				from);
	return dba_error_ok();
}

dba_err dba_convert_WMO0513_to_BUFR20012(int from, int* to)
{
	if (from >= 0 && from <= 9)
		*to = from + 30;
	else if (from == -1) /* FIXME: check what is the value for '/' */
		*to = 62;
	else
		return dba_error_notfound("value %d not found in WMO code table 0513", from);
	return dba_error_ok();
}

dba_err dba_convert_BUFR20012_to_WMO0513(int from, int* to)
{
	if (from >= 30 && from <= 39)
		*to = from - 30;
	else if (from == 62)
		*to = -1; /* FIXME: check what is the value for '/' */
	else
		return dba_error_notfound(
				"BUFR 20012 value %d cannot be represented with WMO code table 0513",
				from);
	return dba_error_ok();
}

dba_err dba_convert_WMO4677_to_BUFR20003(int from, int* to)
{
	if (from <= 99)
		*to = from;
	else
		return dba_error_consistency("cannot handle WMO4677 present weather (%d) values above 99", from);
	return dba_error_ok();
}

dba_err dba_convert_BUFR20003_to_WMO4677(int from, int* to)
{
	if (from <= 99)
		*to = from;
	else
		return dba_error_consistency("cannot handle BUFR 20003 present weather (%d) values above 99", from);
	return dba_error_ok();
}

dba_err dba_convert_WMO4561_to_BUFR20004(int from, int* to)
{
	if (from <= 9)
		*to = from;
	else
		return dba_error_consistency("cannot handle WMO4561 past weather (%d) values above 9", from);
	return dba_error_ok();
}

dba_err dba_convert_BUFR20004_to_WMO4561(int from, int* to)
{
	if (from <= 9)
		*to = from;
	else
		return dba_error_consistency("cannot handle BUFR 20004 present weather (%d) values above 9", from);
	return dba_error_ok();
}

dba_err dba_convert_icao_to_press(double from, double* to)
{
	static const double ZA = 5.252368255329;
	static const double ZB = 44330.769230769;
	static const double ZC = 0.000157583169442;
	static const double P0 = 1013.25;
	static const double P11 = 226.547172;

	if (from <= 11000)
		/* We are below 11 km */
		*to = P0 * pow(1 - from / ZB, ZA);
	else
		/* We are above 11 km */
		*to = P11 * exp(-ZC * (from - 11000));

	return dba_error_ok();
}

dba_err dba_convert_press_to_icao(double from, double* to)
{
	return dba_error_unimplemented("converting pressure to ICAO height");
}

dba_err dba_convert_AOFVSS_to_BUFR08001(int from, int* to)
{
	*to = 0;
	if (from & (1 << 0))	/* Maximum wind level	*/
		*to |= 8;
	if (from & (1 << 1))	/* Tropopause		*/
		*to |= 16;
	/* Skipped */		/* Part D, non-standard level data, p < 100hPa */
	if (from & (1 << 3))	/* Part C, standard level data, p < 100hPa */
		*to |= 32;
	/* Skipped */		/* Part B, non-standard level data, p > 100hPa */
	if (from & (1 << 5))	/* Part A, standard level data, p > 100hPa */
		*to |= 32;
	if (from & (1 << 6))	/* Surface */
		*to |= 64;
	if (from & (1 << 7))	/* Significant wind level */
		*to |= 2;
	if (from & (1 << 8))	/* Significant temperature level */
		*to |= 4;
	return dba_error_ok();
}
#endif
