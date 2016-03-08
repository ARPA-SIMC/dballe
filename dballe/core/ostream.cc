#include "defs.h"
#include <ostream>
#include <iomanip>

using namespace std;

namespace dballe {

std::ostream& operator<<(std::ostream& out, const Coords& c)
{
    out << fixed
        << "(" << setprecision(5) << c.dlat()
        << "," << setprecision(5) << c.dlon()
        << ")"
        << defaultfloat;
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
    out << fixed
        << "(" << setprecision(5) << dmin
        << " to " << setprecision(5) << dmax
        << ")"
        << defaultfloat;
    return out;
}

std::ostream& operator<<(std::ostream& out, const LonRange& lr)
{
    double dmin, dmax;
    lr.get(dmin, dmax);
    out << fixed
        << "(" << setprecision(5) << dmin
        << " to " << setprecision(5) << dmax
        << ")"
        << defaultfloat;
    return out;
}

std::ostream& operator<<(std::ostream& out, const Ident& i)
{
    if (i.is_missing())
        out << "(null)";
    else
        out << (const char*)i;
    return out;
}

}
