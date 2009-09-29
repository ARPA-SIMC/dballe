/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#define _GNU_SOURCE
/* _GNU_SOURCE is defined to have asprintf */

#include "formatter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /* strtoul, getenv */
#include <limits.h>

dba_err dba_formatter_describe_level(int ltype, int l1, char** res)
{
	switch (ltype)
	{
		case 1:	  *res = strdup("Ground or water surface"); break;
		case 2:	  *res = strdup("Cloud base level"); break;
		case 3:	  *res = strdup("Level of cloud tops"); break;
		case 4:	  *res = strdup("Level of 0°C isotherm"); break;
		case 5:	  *res = strdup("Level of adiabatic condensation lifted from the surface"); break;
		case 6:	  *res = strdup("Maximum wind level"); break;
		case 7:	  *res = strdup("Tropopause"); break;
		case 8:	  if (l1 == 0)
				  	*res = strdup("Nominal top of atmosphere");
				  else
				  	asprintf(res, "Nominal top of atmosphere, channel %d", l1);
				  break;
		case 9:	  *res = strdup("Sea bottom"); break;
		case 20:  asprintf(res, "Isothermal level, %.1fK", (double)l1/10); break;
		case 100: asprintf(res, "Isobaric surface, %.2fhPa", (double)l1/100); break;
		case 101: *res = strdup("Mean sea level"); break;
		case 102: asprintf(res, "%.3fm above mean sea level", (double)l1/1000); break;
		case 103: asprintf(res, "%.3fm above ground", (double)l1/1000); break;
		case 104: asprintf(res, "Sigma level %.5f", (double)l1/10000); break;
		case 105: asprintf(res, "Hybrid level %d", l1); break;
		case 106: asprintf(res, "%.3fm below land surface", (double)l1/1000); break;
		case 107: asprintf(res, "Isentropic (theta) level, potential temperature %.1fK", (double)l1/10); break;
		case 108: asprintf(res, "Pressure difference %.2fhPa from ground to level", (double)l1/100); break;
 		case 109: asprintf(res, "Potential vorticity surface %.3f 10-6 K m2 kg-1 s-1", (double)l1/1000); break;
		case 111: asprintf(res, "ETA* level %.5f", (double)l1/10000); break;
		case 117: asprintf(res, "Mixed layer depth %.3fm", (double)l1/1000); break;
		case 160: asprintf(res, "%.3fm below sea level", (double)l1/1000); break;
		case 200: *res = strdup("Entire atmosphere (considered as a single layer)"); break;
		case 201: *res = strdup("Entire ocean (considered as a single layer)"); break;
		case 204: *res = strdup("Highest tropospheric freezing level"); break;
		case 206: *res = strdup("Grid scale cloud bottom level"); break;
		case 207: *res = strdup("Grid scale cloud top level"); break;
		case 209: *res = strdup("Boundary layer cloud bottom level"); break;
		case 210: *res = strdup("Boundary layer cloud top level"); break;
		case 211: *res = strdup("Boundary layer cloud layer"); break;
		case 212: *res = strdup("Low cloud bottom level"); break;
		case 213: *res = strdup("Low cloud top level"); break;
		case 214: *res = strdup("Low cloud layer"); break;
		case 215: *res = strdup("Cloud ceiling"); break;
		case 220: *res = strdup("Planetary Boundary Layer"); break;
		case 222: *res = strdup("Middle cloud bottom level"); break;
		case 223: *res = strdup("Middle cloud top level"); break;
		case 224: *res = strdup("Middle cloud layer"); break;
		case 232: *res = strdup("High cloud bottom level"); break;
		case 233: *res = strdup("High cloud top level"); break;
		case 234: *res = strdup("High cloud layer"); break;
		case 235: asprintf(res, "Ocean Isotherm Level, %.1fK", (double)l1/10); break;
		case 240: *res = strdup("Ocean Mixed Layer"); break;
		case 242: *res = strdup("Convective cloud bottom level"); break;
		case 243: *res = strdup("Convective cloud top level"); break;
		case 244: *res = strdup("Convective cloud layer"); break;
		case 245: *res = strdup("Lowest level of the wet bulb zero"); break;
		case 246: *res = strdup("Maximum equivalent potential temperature level"); break;
		case 247: *res = strdup("Equilibrium level"); break;
		case 248: *res = strdup("Shallow convective cloud bottom level"); break;
		case 249: *res = strdup("Shallow convective cloud top level"); break;
		case 251: *res = strdup("Deep convective cloud bottom level"); break;
		case 252: *res = strdup("Deep convective cloud top level"); break;
		case 253: *res = strdup("Lowest bottom level of supercooled liquid water layer"); break;
		case 254: *res = strdup("Highest top level of supercooled liquid water layer"); break;
		case 255: *res = strdup("Missing"); break;
		case 256:
			switch (l1) {
				case 0: *res = strdup("General cloud group"); break;
				case 1: *res = strdup("CL"); break;
				case 2: *res = strdup("CM"); break;
				case 3: *res = strdup("CH"); break;
				default: asprintf(res, "%d %d", ltype, l1); break;
			}
			break;
		case 257: *res = strdup("Information about the station that generated the data"); break;
		case 258:
			switch (l1) {
				case 1: *res = strdup("First synop cloud group"); break;
				case 2: *res = strdup("Second synop cloud group"); break;
				case 3: *res = strdup("Third synop cloud group"); break;
				case 4: *res = strdup("Fourth synop cloud group"); break;
				default: asprintf(res, "%d %d", ltype, l1); break;
			}
			break;
		default:	asprintf(res, "%d %d", ltype, l1); break;
	}
	return dba_error_ok();
}

dba_err dba_formatter_describe_level_or_layer(int ltype1, int l1, int ltype2, int l2, char** buf)
{
	dba_err err = DBA_OK;
	char* a = NULL;
	char* b = NULL;

	if (ltype2 < 0 || ltype2 == INT_MAX)
		return dba_formatter_describe_level(ltype1, l1, buf);

	DBA_RUN_OR_GOTO(cleanup, dba_formatter_describe_level(ltype1, l1, &a));
	DBA_RUN_OR_GOTO(cleanup, dba_formatter_describe_level(ltype2, l2, &b));
	asprintf(buf, "Layer from [%s] to [%s]", a, b);

cleanup:
	if (a) free(a);
	if (b) free(b);
	return err == DBA_OK ? dba_error_ok() : err;
}


static int format_seconds(char* buf, int bufsize, int val)
{
	int i = 0;
	if (val / (3600*24) != 0)
	{
		i += snprintf(buf+i, bufsize-i, "%dd ", val / (3600*24));
		val = abs(val) % (3600*24);
	}
	if (val / 3600 != 0)
	{
		i += snprintf(buf+i, bufsize-i, "%dh ", val / 3600);
		val = abs(val) % 3600;
	}
	if (val / 60 != 0)
	{
		i += snprintf(buf+i, bufsize-i, "%dm ", val / 60);
		val = abs(val) % 60;
	}
	if (val)
		i += snprintf(buf+i, bufsize-i, "%ds ", val);
	buf[i] = 0;

	return i;
}

dba_err dba_formatter_describe_trange(int ptype, int p1, int p2, char** res)
{
	char duration[40], offset[60];

	if (p1 == 0)
		offset[0] = 0;
	else
	{
		int i = 0;
		i += snprintf(offset+i, 60-i, ", forecast time ");
		i = format_seconds(offset+i, 60-i, p1);
	}

	if (p2 == 0)
		strncpy(duration, " (instant)", 40);
	else {
		int i = 0;
		i += snprintf(duration+i, 40-i, " over ");
		format_seconds(duration+i, 40-i, p2);
	}

	switch (ptype)
	{
		case 0:   asprintf(res, "Average%s%s", duration, offset); break;
		case 1:   asprintf(res, "Accumulation%s%s", duration, offset); break;
		case 2:   asprintf(res, "Maximum over%s%s", duration, offset); break;
		case 3:   asprintf(res, "Minimum over%s%s", duration, offset); break;
		case 4:   asprintf(res, "Difference (end minus beginning)%s%s", duration, offset); break;
		case 5:   asprintf(res, "Root Mean Square%s%s", duration, offset); break;
		case 6:   asprintf(res, "Standard Deviation%s%s", duration, offset); break;
		case 7:   asprintf(res, "Covariance (temporal variance)%s%s", duration, offset); break;
		case 8:   asprintf(res, "Difference (beginning minus end)%s%s", duration, offset); break;
		case 9:   asprintf(res, "Ratio%s%s", duration, offset); break;
		case 51:  asprintf(res, "Climatological Mean Value%s%s", duration, offset); break;
		case 200: asprintf(res, "Vectorial mean%s%s", duration, offset); break;
		case 201: asprintf(res, "Mode%s%s", duration, offset); break;
		case 202: asprintf(res, "Standard deviation vectorial mean%s%s", duration, offset); break;
		case 203: asprintf(res, "Vectorial maximum%s%s", duration, offset); break;
		case 204: asprintf(res, "Vectorial minimum%s%s", duration, offset); break;
		case 205: asprintf(res, "Product with a valid time ranging%s%s", duration, offset); break;
		case 254: asprintf(res, "Instantaneous value%s", offset); break;
		default:  asprintf(res, "%d %d %d", ptype, p1, p2); break;
	}
	return dba_error_ok();
}


/* vim:set ts=4 sw=4: */
