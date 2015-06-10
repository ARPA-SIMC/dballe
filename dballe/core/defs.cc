#include "defs.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <wreport/error.h>
#include "dballe/core/vasprintf.h"
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


namespace {

inline int ll_to_int(double ll) { return lround(ll * 100000.0); }
inline double ll_from_int(int ll) { return (double)ll / 100000.0; }

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
    : lat(ll_to_int(lat)),
      lon(normalon(ll_to_int(lon)))
{
}

double Coords::dlat() const { return ll_from_int(lat); }
double Coords::dlon() const { return ll_from_int(lon); }

void Coords::set_lat(int new_lat) { lat = new_lat; }
void Coords::set_lon(int new_lon) { lon = normalon(new_lon); }
void Coords::set_lat(double new_lat) { lat = ll_to_int(new_lat); }
void Coords::set_lon(double new_lon) { lon = normalon(ll_to_int(new_lon)); }

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

std::ostream& operator<<(std::ostream& out, const DatetimeRange& dtr)
{
    if (dtr.min == dtr.max)
        dtr.min.to_stream_iso8601(out);
    else
    {
        out << "(";
        dtr.min.to_stream_iso8601(out);
        out << " to ";
        dtr.max.to_stream_iso8601(out);
        out << ")";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const LatRange& lr)
{
    double dmin, dmax;
    lr.get(dmin, dmax);
    out << "(" << setprecision(5) << dmin
        << " to " << setprecision(5) << dmax
        << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, const LonRange& lr)
{
    double dmin, dmax;
    lr.get(dmin, dmax);
    out << "(" << setprecision(5) << dmin
        << " to " << setprecision(5) << dmax
        << ")";
    return out;
}

}
