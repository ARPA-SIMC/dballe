#include "dballe/core/data.h"
#include "dballe/core/var.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace core {

void Data::setf(const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "rep_memo":    station.report = val;
        case "report":      station.report = val;
        case "ana_id":      station.id = strtol(val, nullptr, 10);
        case "ident":       station.ident = val;
        case "lat":         { double dval = strtod(val, nullptr); station.coords.set_lat(dval); }
        case "lon":         { double dval = strtod(val, nullptr); station.coords.set_lon(dval); }
        case "year":        datetime.year   = strtol(val, nullptr, 10);
        case "month":       datetime.month  = strtol(val, nullptr, 10);
        case "day":         datetime.day    = strtol(val, nullptr, 10);
        case "hour":        datetime.hour   = strtol(val, nullptr, 10);
        case "min":         datetime.minute = strtol(val, nullptr, 10);
        case "sec":         datetime.second = strtol(val, nullptr, 10);
        case "leveltype1":  level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          level.l1     = strtol(val, nullptr, 10);
        case "leveltype2":  level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          level.l2     = strtol(val, nullptr, 10);
        case "pindicator":  trange.pind  = strtol(val, nullptr, 10);
        case "p1":          trange.p1    = strtol(val, nullptr, 10);
        case "p2":          trange.p2    = strtol(val, nullptr, 10);
        default:            values.setf(key, val);
    }
}

}
}
