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

LatRange::LatRange(double min, double max)
    : imin(ll_to_int(min)),
      imax(ll_to_int(max))
{
}

bool LatRange::is_missing() const { return imin == IMIN && imax == IMAX; }

void LatRange::get(double& min, double& max) const
{
    min = ll_from_int(imin);
    max = ll_from_int(imax);
}

void LatRange::set(int min, int max)
{
    imin = min;
    imax = max;
}

void LatRange::set(double min, double max)
{
    imin = ll_to_int(min);
    imax = ll_to_int(max);
}

bool LatRange::contains(int lat) const
{
    return lat >= imin && lat <= imax;
}

bool LatRange::contains(double lat) const
{
    int ilat = ll_to_int(lat);
    return ilat >= imin && ilat <= imax;
}

LonRange::LonRange(int min, int max)
    : imin(Coords::normalon(min)), imax(Coords::normalon(max))
{
    if (min != max && imin == imax)
        imin = imax = MISSING_INT;
}

LonRange::LonRange(double min, double max)
    : imin(Coords::normalon(ll_to_int(min))), imax(Coords::normalon(ll_to_int(max)))
{
    if (min != max && imin == imax)
        imin = imax = MISSING_INT;
}

bool LonRange::is_missing() const
{
    return imin == MISSING_INT || imax == MISSING_INT;
}

void LonRange::get(double& min, double& max) const
{
    if (is_missing())
    {
        min = -180.0;
        max = 180.0;
    } else {
        min = ll_from_int(imin);
        max = ll_from_int(imax);
    }
}

void LonRange::set(int min, int max)
{
    bool differ = min != max;
    imin = Coords::normalon(min);
    imax = Coords::normalon(max);
    // Catch cases like min=0 max=360, that would match anything, and set them
    // to missing range match
    if (differ && imin == imax)
        imin = imax = MISSING_INT;
}

void LonRange::set(double min, double max)
{
    set(ll_to_int(min), ll_to_int(max));
}

bool LonRange::contains(int lon) const
{
    if (imin == imax)
    {
        if (imin == MISSING_INT)
            return true;
        return lon == imin;
    } else if (imin < imax) {
        return lon >= imin && lon <= imax;
    } else {
        return ((lon >= imin and lon <= 18000000)
             or (lon >= -18000000 and lon <= imax));
    }
}

bool LonRange::contains(double lon) const
{
    return contains(ll_to_int(lon));
}

}
