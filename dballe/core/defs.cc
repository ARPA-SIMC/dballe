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

Ident::Ident(const char* value) : value(value ? strdup(value) : nullptr) {}
Ident::Ident(const Ident& o) : value(o.value ? strdup(o.value) : nullptr) {}
Ident::Ident(Ident&& o) : value(o.value) { o.value = nullptr; }
Ident::~Ident() { free(value); }
Ident& Ident::operator=(const Ident& o)
{
    if (value == o.value) return *this;
    free(value);
    value = o.value ? strdup(o.value) : nullptr;
    return *this;
}
Ident& Ident::operator=(Ident&& o)
{
    if (value == o.value) return *this;
    free(value);
    if (o.value)
    {
        value = strdup(o.value);
        o.value = nullptr;
    } else
        value = nullptr;
    return *this;
}
Ident& Ident::operator=(const char* o)
{
    if (value) free(value);
    value = o ? strdup(o) : nullptr;
    return *this;

}
Ident& Ident::operator=(const std::string& o)
{
    if (value) free(value);
    value = strndup(o.c_str(), o.size());
    return *this;
}
void Ident::clear()
{
    free(value);
    value = 0;
}
int Ident::compare(const Ident& o) const
{
    if (!value && !o.value) return 0;
    if (!value && o.value) return -1;
    if (value && !o.value) return 1;
    return strcmp(value, o.value);
}
int Ident::compare(const char* o) const
{
    if (!value && !o) return 0;
    if (!value && o) return -1;
    if (value && !o) return 1;
    return strcmp(value, o);
}
int Ident::compare(const std::string& o) const
{
    if (!value) return -1;
    return strcmp(value, o.c_str());
}
Ident::operator std::string() const
{
    if (!value) throw error_consistency("ident is not set");
    return std::string(value);
}


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

std::ostream& operator<<(std::ostream& out, const Ident& i)
{
    if (i.is_missing())
        out << "(null)";
    else
        out << (const char*)i;
    return out;
}

}
