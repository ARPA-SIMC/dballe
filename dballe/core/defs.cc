/*
 * msg/defs - Common definitions
 *
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "defs.h"

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include "dballe/core/vasprintf.h"
#include "dballe/core/record.h"
#include <math.h>
#include <ostream>
#include <iomanip>

using namespace std;

namespace dballe {

const char* encoding_name(Encoding enc)
{
	switch (enc)
	{
		case BUFR: return "BUFR";
		case CREX: return "CREX";
		case AOF: return "AOF";
		default: return "(unknown)";
	}
}

static int str_to_val(const char* str)
{
    if (str == NULL || str[0] == 0 || strcmp(str, "-") == 0)
        return MISSING_INT;
    else
        return strtol(str, NULL, 10);
}

Level::Level(const char* ltype1, const char* l1, const char* ltype2, const char* l2)
    : ltype1(str_to_val(ltype1)), l1(str_to_val(l1)), ltype2(str_to_val(ltype2)), l2(str_to_val(l2))
{
}

Trange::Trange(const char* pind, const char* p1, const char* p2)
    : pind(str_to_val(pind)), p1(str_to_val(p1)), p2(str_to_val(p2))
{
}


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

static std::string describe_level(int ltype, int l1)
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

std::string Level::describe() const
{
    if (ltype2 == MISSING_INT || l2 == MISSING_INT)
        return describe_level(ltype1, l1);

    if (ltype1 == 256)
    {
        string lev1 = describe_level(ltype1, l1);
        string lev2 = describe_level(ltype2, l2);
        return lev1 + ", " + lev2;
    } else {
        string lev1 = describe_level(ltype1, l1);
        string lev2 = describe_level(ltype2, l2);
        return "Layer from [" + lev1 + "] to [" + lev2 + "]";
    }
}

void Level::format(std::ostream& out, const char* undef) const
{
    if (ltype1 == MISSING_INT) out << undef; else out << ltype1;
    out << ",";
    if (l1 == MISSING_INT) out << undef; else out << l1;
    out << ",";
    if (ltype2 == MISSING_INT) out << undef; else out << ltype2;
    out << ",";
    if (l2 == MISSING_INT) out << undef; else out << l2;
}

std::ostream& operator<<(std::ostream& out, const Level& l)
{
    l.format(out);
    return out;
}

void Trange::format(std::ostream& out, const char* undef) const
{
    if (pind == MISSING_INT) out << undef; else out << pind;
    out << ",";
    if (p1 == MISSING_INT) out << undef; else out << p1;
    out << ",";
    if (p2 == MISSING_INT) out << undef; else out << p2;
}

std::ostream& operator<<(std::ostream& out, const Trange& l)
{
    l.format(out);
    return out;
}

static std::string format_seconds(int val)
{
	static const int bufsize = 128;
	char buf[bufsize];

	if (val == MISSING_INT) return "-";

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
	{
		strncpy(buf, "0", bufsize);
		i = 1;
	}
	buf[i] = 0;

	return buf;
}

static std::string mkdesc(const std::string& root, int p1, int p2)
{
	if (p1 == MISSING_INT && p2 == MISSING_INT)
		return root;

	string res = root;
	if (p2 != MISSING_INT)
		res += " over " + format_seconds(p2);

	if (p1 == MISSING_INT)
		return res;
	if (p1 < 0)
		return res + format_seconds(-p1) + " before reference time";
	return res + " at forecast time " + format_seconds(p1);
}

std::string Trange::describe() const
{
	switch (pind)
	{
		case 0:   return mkdesc("Average", p1, p2);
		case 1:   return mkdesc("Accumulation", p1, p2);
		case 2:   return mkdesc("Maximum", p1, p2);
		case 3:   return mkdesc("Minimum", p1, p2);
		case 4:   return mkdesc("Difference (end minus beginning)", p1, p2);
		case 5:   return mkdesc("Root Mean Square", p1, p2);
		case 6:   return mkdesc("Standard Deviation", p1, p2);
		case 7:   return mkdesc("Covariance (temporal variance)", p1, p2);
		case 8:   return mkdesc("Difference (beginning minus end)", p1, p2);
		case 9:   return mkdesc("Ratio", p1, p2);
		case 51:  return mkdesc("Climatological Mean Value", p1, p2);
		case 200: return mkdesc("Vectorial mean", p1, p2);
		case 201: return mkdesc("Mode", p1, p2);
		case 202: return mkdesc("Standard deviation vectorial mean", p1, p2);
		case 203: return mkdesc("Vectorial maximum", p1, p2);
		case 204: return mkdesc("Vectorial minimum", p1, p2);
		case 205: return mkdesc("Product with a valid time ranging", p1, p2);
		case 254:
			  if (p1 == 0 && p2 == 0)
				  return "Analysis or observation, istantaneous value";
			  else
				  return "Forecast at t+" + format_seconds(p1) + ", instantaneous value";
		default:  return fmtf("%d %d %d", pind, p1, p2);
	}
}


// Normalise longitude values to the [-180..180[ interval
static inline int normalon(int lon)
{
    return ((lon + 18000000) % 36000000) - 18000000;
}


Coord::Coord(int lat, int lon)
    : lat(lat),
      lon(normalon(lon))
{
}

Coord::Coord(double lat, double lon)
    : lat(lround(lat * 100000)),
      lon(normalon(lround(lon * 100000)))
{
}

double Coord::dlat() const { return (double)lat/100000.0; }
double Coord::dlon() const { return (double)lon/100000.0; }

std::ostream& operator<<(std::ostream& out, const Coord& c)
{
    out << "(" << setprecision(5) << c.dlat()
        << "," << setprecision(5) << c.dlon()
        << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, const Date& dt)
{
    out <<        setw(4) << setfill('0') << dt.year
        << '-' << setw(2) << setfill('0') << (unsigned)dt.month
        << '-' << setw(2) << setfill('0') << (unsigned)dt.day;
    return out;
}

std::ostream& operator<<(std::ostream& out, const Datetime& dt)
{
    out <<        setw(4) << setfill('0') << dt.year
        << '-' << setw(2) << setfill('0') << (unsigned)dt.month
        << '-' << setw(2) << setfill('0') << (unsigned)dt.day
        << 'T' << setw(2) << setfill('0') << (unsigned)dt.hour
        << ':' << setw(2) << setfill('0') << (unsigned)dt.minute
        << ':' << setw(2) << setfill('0') << (unsigned)dt.second;
    return out;
}

} // namespace dballe

/* vim:set ts=4 sw=4: */
