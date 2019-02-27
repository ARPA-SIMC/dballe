#include "query.h"
#include "var.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace core {

void Query::setf(const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = strtol(val, nullptr, 10);
        case "priomax":     priomax = strtol(val, nullptr, 10);
        case "priomin":     priomin = strtol(val, nullptr, 10);
        case "rep_memo":    report = val;
        case "report":      report = val;
        case "ana_id":      ana_id = strtol(val, nullptr, 10);
        case "mobile":      mobile = strtol(val, nullptr, 10);
        case "ident":       ident = val;
        case "lat":         { double dval = strtod(val, nullptr); latrange.set(dval, dval); }
        case "lon":         { double dval = strtod(val, nullptr); lonrange.set(dval, dval); }
        case "latmax":      latrange.imax = Coords::lat_to_int(strtod(val, nullptr));
        case "latmin":      latrange.imin = Coords::lat_to_int(strtod(val, nullptr));
        case "lonmax":      lonrange.imax = Coords::lon_to_int(strtod(val, nullptr));
        case "lonmin":      lonrange.imin = Coords::lon_to_int(strtod(val, nullptr));
        case "year":        dtrange.min.year   = dtrange.max.year = strtol(val, nullptr, 10);
        case "month":       dtrange.min.month  = dtrange.max.month = strtol(val, nullptr, 10);
        case "day":         dtrange.min.day    = dtrange.max.day = strtol(val, nullptr, 10);
        case "hour":        dtrange.min.hour   = dtrange.max.hour = strtol(val, nullptr, 10);
        case "min":         dtrange.min.minute = dtrange.max.minute = strtol(val, nullptr, 10);
        case "sec":         dtrange.min.second = dtrange.max.second = strtol(val, nullptr, 10);
        case "yearmax":     dtrange.max.year = strtol(val, nullptr, 10);
        case "yearmin":     dtrange.min.year = strtol(val, nullptr, 10);
        case "monthmax":    dtrange.max.month = strtol(val, nullptr, 10);
        case "monthmin":    dtrange.min.month = strtol(val, nullptr, 10);
        case "daymax":      dtrange.max.day = strtol(val, nullptr, 10);
        case "daymin":      dtrange.min.day = strtol(val, nullptr, 10);
        case "hourmax":     dtrange.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     dtrange.min.hour = strtol(val, nullptr, 10);
        case "minumax":     dtrange.max.minute = strtol(val, nullptr, 10);
        case "minumin":     dtrange.min.minute = strtol(val, nullptr, 10);
        case "secmax":      dtrange.max.second = strtol(val, nullptr, 10);
        case "secmin":      dtrange.min.second = strtol(val, nullptr, 10);
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
