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

void DatetimeRange::merge(const DatetimeRange& range)
{
    if (!min.is_missing() && (range.min.is_missing() || range.min < min))
        min = range.min;

    if (!max.is_missing() && (range.max.is_missing() || range.max > max))
        max = range.max;
}

void DatetimeRange::set(const Datetime& dt)
{
    min = dt;
    max = dt;
}

void DatetimeRange::set(const Datetime& min, const Datetime& max)
{
    this->min = min;
    this->max = max;
}

void DatetimeRange::set(
        int yemin, int momin, int damin, int homin, int mimin, int semin,
        int yemax, int momax, int damax, int homax, int mimax, int semax)
{
    if (yemin == MISSING_INT)
        min = Datetime();
    else
        min = Datetime::lower_bound(yemin, momin, damin, homin, mimin, semin);

    if (yemax == MISSING_INT)
        max = Datetime();
    else
        max = Datetime::upper_bound(yemax, momax, damax, homax, mimax, semax);
}

bool DatetimeRange::contains(const Datetime& dt) const
{
    if (min.is_missing())
        if (max.is_missing())
            return true;
        else
            return dt <= max;
    else
        if (max.is_missing())
            return dt >= min;
        else
            return min <= dt && dt <= max;
}

bool DatetimeRange::contains(const DatetimeRange& dtr) const
{
    if (!min.is_missing() && (dtr.min.is_missing() || dtr.min < min))
        return false;

    if (!max.is_missing() && (dtr.max.is_missing() || dtr.max > max))
        return false;

    return true;
}

bool DatetimeRange::is_disjoint(const DatetimeRange& dtr) const
{
    if (!max.is_missing() && !dtr.min.is_missing() && max < dtr.min)
        return true;

    if (!dtr.max.is_missing() && !min.is_missing() && dtr.max < min)
        return true;

    return false;
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
