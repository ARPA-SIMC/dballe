/*
 * DB-ALLe - Archive for punctual meteorological data
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

#include "formatter.h"
#include "defs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /* strtoul, getenv */
#include <stdarg.h>

using namespace std;

namespace dballe {

namespace {
std::string fmtf( const char* f, ... )
{
    char *c;
    va_list ap;
    va_start( ap, f );
    vasprintf( &c, f, ap );
    std::string ret( c );
    free( c );
    return ret;
}
}

std::string describe_level(int ltype, int l1)
{
	switch (ltype)
	{
		case 1:	  return "Ground or water surface";
		case 2:	  return "Cloud base level";
		case 3:	  return "Level of cloud tops";
		case 4:	  return "Level of 0°C isotherm";
		case 5:	  return "Level of adiabatic condensation lifted from the surface";
		case 6:	  return "Maximum wind level";
		case 7:	  return "Tropopause";
		case 8:	  if (l1 == 0)
				  return "Nominal top of atmosphere";
			  else
				  return fmtf("Nominal top of atmosphere, channel %d", l1);
		case 9:	  return "Sea bottom";
		case 20:  return fmtf("Isothermal level, %.1fK", (double)l1/10);
		case 100: return fmtf("Isobaric surface, %.2fhPa", (double)l1/100);
		case 101: return "Mean sea level";
		case 102: return fmtf("%.3fm above mean sea level", (double)l1/1000);
		case 103: return fmtf("%.3fm above ground", (double)l1/1000);
		case 104: return fmtf("Sigma level %.5f", (double)l1/10000);
		case 105: return fmtf("Hybrid level %d", l1);
		case 106: return fmtf("%.3fm below land surface", (double)l1/1000);
		case 107: return fmtf("Isentropic (theta) level, potential temperature %.1fK", (double)l1/10);
		case 108: return fmtf("Pressure difference %.2fhPa from ground to level", (double)l1/100);
 		case 109: return fmtf("Potential vorticity surface %.3f 10-6 K m2 kg-1 s-1", (double)l1/1000);
		case 111: return fmtf("ETA* level %.5f", (double)l1/10000);
		case 117: return fmtf("Mixed layer depth %.3fm", (double)l1/1000);
		case 160: return fmtf("%.3fm below sea level", (double)l1/1000);
		case 200: return "Entire atmosphere (considered as a single layer)";
		case 201: return "Entire ocean (considered as a single layer)";
		case 204: return "Highest tropospheric freezing level";
		case 206: return "Grid scale cloud bottom level";
		case 207: return "Grid scale cloud top level";
		case 209: return "Boundary layer cloud bottom level";
		case 210: return "Boundary layer cloud top level";
		case 211: return "Boundary layer cloud layer";
		case 212: return "Low cloud bottom level";
		case 213: return "Low cloud top level";
		case 214: return "Low cloud layer";
		case 215: return "Cloud ceiling";
		case 220: return "Planetary Boundary Layer";
		case 222: return "Middle cloud bottom level";
		case 223: return "Middle cloud top level";
		case 224: return "Middle cloud layer";
		case 232: return "High cloud bottom level";
		case 233: return "High cloud top level";
		case 234: return "High cloud layer";
		case 235: return fmtf("Ocean Isotherm Level, %.1fK", (double)l1/10); break;
		case 240: return "Ocean Mixed Layer";
		case 242: return "Convective cloud bottom level";
		case 243: return "Convective cloud top level";
		case 244: return "Convective cloud layer";
		case 245: return "Lowest level of the wet bulb zero";
		case 246: return "Maximum equivalent potential temperature level";
		case 247: return "Equilibrium level";
		case 248: return "Shallow convective cloud bottom level";
		case 249: return "Shallow convective cloud top level";
		case 251: return "Deep convective cloud bottom level";
		case 252: return "Deep convective cloud top level";
		case 253: return "Lowest bottom level of supercooled liquid water layer";
		case 254: return "Highest top level of supercooled liquid water layer";
		case 255: return "Missing";
		case 256: return "Clouds";
		case 257: return "Information about the station that generated the data";
		case 258:
			switch (l1) {
				case 0: return "General cloud group";
				case 1: return "CL";
				case 2: return "CM";
				case 3: return "CH";
				default: return fmtf("%d %d", ltype, l1);
			}
			break;
		case 259: return fmtf("Cloud group %d", l1);
		case 260: return fmtf("Cloud drift group %d", l1);
		case 261: return fmtf("Cloud elevation group %d", l1);
		case 262: return "Direction and elevation of clouds";
	        case MISSING_INT: return "-";
		default:	return fmtf("%d %d", ltype, l1); break;
	}
}

std::string describe_level_or_layer(int ltype1, int l1, int ltype2, int l2)
{
	if (ltype2 == MISSING_INT || ltype2 == MISSING_INT)
		return describe_level(ltype1, l1);

	string lev1 = describe_level(ltype1, l1);
	string lev2 = describe_level(ltype2, l2);
	return "Layer from [" + lev1 + "] to [" + lev2 + "]";
}


static std::string format_seconds(int val)
{
	static const int bufsize = 128;
	char buf[bufsize];

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
	if (i > 0)
		--i;
	else
		strncpy(buf, "0", bufsize);
	buf[i] = 0;

	return buf;
}

std::string describe_trange(int ptype, int p1, int p2)
{
	string duration;
	string offset;

	if (p1 != MISSING_INT)
		offset = ", forecast time " + format_seconds(p1);

	if (p2 == MISSING_INT)
		duration = " (instant)";
	else
		offset = " over " + format_seconds(p1);

	switch (ptype)
	{
		case 0:   return "Average" + duration + offset;
		case 1:   return "Accumulation" + duration + offset;
		case 2:   return "Maximum over" + duration + offset;
		case 3:   return "Minimum over" + duration + offset;
		case 4:   return "Difference (end minus beginning)" + duration + offset;
		case 5:   return "Root Mean Square" + duration + offset;
		case 6:   return "Standard Deviation" + duration + offset;
		case 7:   return "Covariance (temporal variance)" + duration + offset;
		case 8:   return "Difference (beginning minus end)" + duration + offset;
		case 9:   return "Ratio" + duration + offset;
		case 51:  return "Climatological Mean Value" + duration + offset;
		case 200: return "Vectorial mean" + duration + offset;
		case 201: return "Mode" + duration + offset;
		case 202: return "Standard deviation vectorial mean" + duration + offset;
		case 203: return "Vectorial maximum" + duration + offset;
		case 204: return "Vectorial minimum" + duration + offset;
		case 205: return "Product with a valid time ranging" + duration + offset;
		case 254: return "Instantaneous value" + offset;
		default:  return fmtf("%d %d %d", ptype, p1, p2);
	}
}

}

/* vim:set ts=4 sw=4: */
