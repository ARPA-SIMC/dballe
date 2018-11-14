#include "query.h"
#include "var.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace core {

void Query::setf(const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    prio_min = prio_max = strtol(val, nullptr, 10);
        case "priomax":     prio_max = strtol(val, nullptr, 10);
        case "priomin":     prio_min = strtol(val, nullptr, 10);
        case "rep_memo":    rep_memo = val;
        case "ana_id":      ana_id = strtol(val, nullptr, 10);
        case "mobile":      mobile = strtol(val, nullptr, 10);
        case "ident":       ident = val;
        case "lat":         { double dval = strtod(val, nullptr); latrange.set(dval, dval); }
        case "lon":         { double dval = strtod(val, nullptr); lonrange.set(dval, dval); }
        case "latmax":      latrange.imax = Coords::lat_to_int(strtod(val, nullptr));
        case "latmin":      latrange.imin = Coords::lat_to_int(strtod(val, nullptr));
        case "lonmax":      lonrange.imax = Coords::lon_to_int(strtod(val, nullptr));
        case "lonmin":      lonrange.imin = Coords::lon_to_int(strtod(val, nullptr));
        case "year":        datetime.min.year   = datetime.max.year = strtol(val, nullptr, 10);
        case "month":       datetime.min.month  = datetime.max.month = strtol(val, nullptr, 10);
        case "day":         datetime.min.day    = datetime.max.day = strtol(val, nullptr, 10);
        case "hour":        datetime.min.hour   = datetime.max.hour = strtol(val, nullptr, 10);
        case "min":         datetime.min.minute = datetime.max.minute = strtol(val, nullptr, 10);
        case "sec":         datetime.min.second = datetime.max.second = strtol(val, nullptr, 10);
        case "yearmax":     datetime.max.year = strtol(val, nullptr, 10);
        case "yearmin":     datetime.min.year = strtol(val, nullptr, 10);
        case "monthmax":    datetime.max.month = strtol(val, nullptr, 10);
        case "monthmin":    datetime.min.month = strtol(val, nullptr, 10);
        case "daymax":      datetime.max.day = strtol(val, nullptr, 10);
        case "daymin":      datetime.min.day = strtol(val, nullptr, 10);
        case "hourmax":     datetime.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     datetime.min.hour = strtol(val, nullptr, 10);
        case "minumax":     datetime.max.minute = strtol(val, nullptr, 10);
        case "minumin":     datetime.min.minute = strtol(val, nullptr, 10);
        case "secmax":      datetime.max.second = strtol(val, nullptr, 10);
        case "secmin":      datetime.min.second = strtol(val, nullptr, 10);
        case "leveltype1":  level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          level.l1 = strtol(val, nullptr, 10);
        case "leveltype2":  level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          level.l2 = strtol(val, nullptr, 10);
        case "pindicator":  trange.pind = strtol(val, nullptr, 10);
        case "p1":          trange.p1 = strtol(val, nullptr, 10);
        case "p2":          trange.p2 = strtol(val, nullptr, 10);
        case "var":         varcodes.clear(); varcodes.insert(resolve_varcode(val));
        case "varlist":     varcodes.clear(); resolve_varlist(val, varcodes);
        case "query":       query = val;
        case "ana_filter":  ana_filter = val;
        case "data_filter": data_filter = val;
        case "attr_filter": attr_filter = val;
        case "limit":       limit = strtol(val, nullptr, 10);
        case "block":       block = strtol(val, nullptr, 10);
        case "station":     station = strtol(val, nullptr, 10);
        default: wreport::error_notfound::throwf("key %s is not valid for a query", key);
    }
}

}
}
