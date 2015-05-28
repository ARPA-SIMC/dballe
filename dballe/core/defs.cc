#include "defs.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dballe/core/vasprintf.h"
#include "dballe/core/record.h"
#include <math.h>
#include <ostream>
#include <iomanip>

using namespace std;
using namespace wreport;

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

Encoding parse_encoding(const std::string& s)
{
    if (s == "BUFR") return BUFR;
    if (s == "CREX") return CREX;
    if (s == "AOF") return AOF;
    error_notfound::throwf("unsupported encoding '%s'", s.c_str());
}

static int str_to_val(const char* str)
{
    if (str == NULL || str[0] == 0 || strcmp(str, "-") == 0)
        return MISSING_INT;
    else
        return strtol(str, NULL, 10);
}

std::ostream& operator<<(std::ostream& out, const Level& l)
{
    l.to_stream(out);
    return out;
}

std::ostream& operator<<(std::ostream& out, const Trange& l)
{
    l.to_stream(out);
    return out;
}

int Coords::normalon(int lon)
{
    return ((lon + 18000000) % 36000000) - 18000000;
}

double Coords::fnormalon(double lon)
{
    return fmod(lon + 180.0, 360.0) - 180.0;
}

Coords::Coords(int lat, int lon)
    : lat(lat),
      lon(normalon(lon))
{
}

Coords::Coords(double lat, double lon)
    : lat(lround(lat * 100000)),
      lon(normalon(lround(lon * 100000)))
{
}

double Coords::dlat() const { return (double)lat/100000.0; }
double Coords::dlon() const { return (double)lon/100000.0; }

void Coords::set_lat(int new_lat) { lat = new_lat; }
void Coords::set_lon(int new_lon) { lon = normalon(new_lon); }
void Coords::set_lat(double new_lat) { lat = lround(new_lat * 100000); }
void Coords::set_lon(double new_lon) { lon = normalon(lround(new_lon * 100000)); }

std::ostream& operator<<(std::ostream& out, const Coords& c)
{
    out << "(" << setprecision(5) << c.dlat()
        << "," << setprecision(5) << c.dlon()
        << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, const Date& dt)
{
    dt.to_stream_iso8601(out);
    return out;
}

std::ostream& operator<<(std::ostream& out, const Time& dt)
{
    dt.to_stream_iso8601(out);
    return out;
}

std::ostream& operator<<(std::ostream& out, const Datetime& dt)
{
    dt.to_stream_iso8601(out);
    return out;
}

}
