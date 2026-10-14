#include "dballe/core/query.h"
#include "dballe/core/var.h"
#include <cstring>

using namespace wreport;

namespace dballe::core {

// Check if a string is empty or contains only spaces
static bool spaces_only(const char* str)
{
    while (*str && isspace(*str))
        ++str;
    return *str == 0;
}

static long strtol_checked(const char* val)
{
    char* endptr = nullptr;
    auto res     = strtol(val, &endptr, 10);
    if (!spaces_only(endptr))
        wreport::error_domain::throwf("value '%s' is not a valid integer", val);
    return res;
}

static double strtod_checked(const char* val)
{
    char* endptr = nullptr;
    auto res     = strtod(val, &endptr);
    if (!spaces_only(endptr))
        wreport::error_domain::throwf(
            "value '%s' is not a valid floating point value", val);
    return res;
}

static int dballe_int(const char* val)
{
    if (strcmp(val, "-") == 0)
        return MISSING_INT;
    else
        return strtol_checked(val);
}

static int dballe_lev_int(const char* val)
{
    if (strcmp(val, "-") == 0)
        return REQUIRED_MISSING_INT;
    else
        return strtol_checked(val);
}

static uint16_t dballe_int16(const char* val)
{
    if (strcmp(val, "-") == 0)
        return 0xffff;
    else
        return strtol_checked(val);
}

static uint8_t dballe_int8(const char* val)
{
    if (strcmp(val, "-") == 0)
        return 0xff;
    else
        return strtol_checked(val);
}

static std::string dballe_string(const char* val)
{
    if (strcmp(val, "-") == 0)
        return std::string();
    else
        return val;
}

static Ident dballe_ident(const char* val)
{
    if (strcmp(val, "-") == 0)
        return Ident();
    else
        return Ident(val);
}

void Query::setf(const char* key, unsigned len, const char* val)
{
    switch (key) // mklookup
    {
        case "priority": priomin = priomax = dballe_int(val);
        case "priomax":  priomax = dballe_int(val);
        case "priomin":  priomin = dballe_int(val);
        case "rep_memo": report = dballe_string(val);
        case "report":   report = dballe_string(val);
        case "ana_id":   ana_id = dballe_int(val);
        case "mobile":   mobile = dballe_int(val);
        case "ident":    ident = dballe_ident(val);
        case "lat":
            if (strcmp(val, "-") == 0)
                latrange = LatRange();
            else
            {
                double dval = strtod_checked(val);
                latrange.set(dval, dval);
            }
        case "lon":
            if (strcmp(val, "-") == 0)
                lonrange = LonRange();
            else
            {
                double dval = strtod_checked(val);
                lonrange.set(dval, dval);
            }
        case "latmax":
            if (strcmp(val, "-") == 0)
                latrange.imax = LatRange::IMAX;
            else
                latrange.imax = Coords::lat_to_int(strtod_checked(val));
        case "latmin":
            if (strcmp(val, "-") == 0)
                latrange.imin = LatRange::IMIN;
            else
                latrange.imin = Coords::lat_to_int(strtod_checked(val));
        case "lonmax":
            if (strcmp(val, "-") == 0)
                lonrange.imax = MISSING_INT;
            else
                lonrange.imax = Coords::lon_to_int(strtod_checked(val));
        case "lonmin":
            if (strcmp(val, "-") == 0)
                lonrange.imin = MISSING_INT;
            else
                lonrange.imin = Coords::lon_to_int(strtod_checked(val));
        case "year":       dtrange.min.year = dtrange.max.year = dballe_int16(val);
        case "month":      dtrange.min.month = dtrange.max.month = dballe_int8(val);
        case "day":        dtrange.min.day = dtrange.max.day = dballe_int8(val);
        case "hour":       dtrange.min.hour = dtrange.max.hour = dballe_int8(val);
        case "min":        dtrange.min.minute = dtrange.max.minute = dballe_int8(val);
        case "sec":        dtrange.min.second = dtrange.max.second = dballe_int8(val);
        case "yearmax":    dtrange.max.year = dballe_int16(val);
        case "yearmin":    dtrange.min.year = dballe_int16(val);
        case "monthmax":   dtrange.max.month = dballe_int8(val);
        case "monthmin":   dtrange.min.month = dballe_int8(val);
        case "daymax":     dtrange.max.day = dballe_int8(val);
        case "daymin":     dtrange.min.day = dballe_int8(val);
        case "hourmax":    dtrange.max.hour = dballe_int8(val);
        case "hourmin":    dtrange.min.hour = dballe_int8(val);
        case "minumax":    dtrange.max.minute = dballe_int8(val);
        case "minumin":    dtrange.min.minute = dballe_int8(val);
        case "secmax":     dtrange.max.second = dballe_int8(val);
        case "secmin":     dtrange.min.second = dballe_int8(val);
        case "leveltype1": level.ltype1 = dballe_int(val);
        case "l1":         level.l1 = dballe_lev_int(val);
        case "leveltype2": level.ltype2 = dballe_lev_int(val);
        case "l2":         level.l2 = dballe_lev_int(val);
        case "pindicator": trange.pind = dballe_int(val);
        case "p1":         trange.p1 = dballe_int(val);
        case "p2":         trange.p2 = dballe_int(val);
        case "var":
            varcodes.clear();
            if (strcmp(val, "-") != 0)
                varcodes.insert(resolve_varcode(val));
        case "varlist":
            varcodes.clear();
            if (strcmp(val, "-") != 0)
                resolve_varlist(val, varcodes);
        case "query":       query = dballe_string(val);
        case "ana_filter":  ana_filter = dballe_string(val);
        case "data_filter": data_filter = dballe_string(val);
        case "attr_filter": attr_filter = dballe_string(val);
        case "limit":       limit = dballe_int(val);
        case "block":       block = dballe_int(val);
        case "station":     station = dballe_int(val);
        default:
            wreport::error_notfound::throwf("key %s is not valid for a query",
                                            key);
    }
}

void Query::unset(const char* key, unsigned len)
{
    switch (key) // mklookup
    {
        case "priority":    priomin = priomax = MISSING_INT;
        case "priomax":     priomax = MISSING_INT;
        case "priomin":     priomin = MISSING_INT;
        case "rep_memo":    report.clear();
        case "report":      report.clear();
        case "ana_id":      ana_id = MISSING_INT;
        case "mobile":      mobile = MISSING_INT;
        case "ident":       ident.clear();
        case "lat":         latrange = LatRange();
        case "lon":         lonrange = LonRange();
        case "latmax":      latrange.imax = LatRange::IMAX;
        case "latmin":      latrange.imin = LatRange::IMIN;
        case "lonmax":      lonrange.imax = MISSING_INT;
        case "lonmin":      lonrange.imin = MISSING_INT;
        case "year":        dtrange.min.year = dtrange.max.year = 0xffff;
        case "month":       dtrange.min.month = dtrange.max.month = 0xff;
        case "day":         dtrange.min.day = dtrange.max.day = 0xff;
        case "hour":        dtrange.min.hour = dtrange.max.hour = 0xff;
        case "min":         dtrange.min.minute = dtrange.max.minute = 0xff;
        case "sec":         dtrange.min.second = dtrange.max.second = 0xff;
        case "yearmax":     dtrange.max.year = 0xffff;
        case "yearmin":     dtrange.min.year = 0xffff;
        case "monthmax":    dtrange.max.month = 0xff;
        case "monthmin":    dtrange.min.month = 0xff;
        case "daymax":      dtrange.max.day = 0xff;
        case "daymin":      dtrange.min.day = 0xff;
        case "hourmax":     dtrange.max.hour = 0xff;
        case "hourmin":     dtrange.min.hour = 0xff;
        case "minumax":     dtrange.max.minute = 0xff;
        case "minumin":     dtrange.min.minute = 0xff;
        case "secmax":      dtrange.max.second = 0xff;
        case "secmin":      dtrange.min.second = 0xff;
        case "leveltype1":  level.ltype1 = MISSING_INT;
        case "l1":          level.l1 = MISSING_INT;
        case "leveltype2":  level.ltype2 = MISSING_INT;
        case "l2":          level.l2 = MISSING_INT;
        case "pindicator":  trange.pind = MISSING_INT;
        case "p1":          trange.p1 = MISSING_INT;
        case "p2":          trange.p2 = MISSING_INT;
        case "var":         varcodes.clear();
        case "varlist":     varcodes.clear();
        case "query":       query.clear();
        case "ana_filter":  ana_filter.clear();
        case "data_filter": data_filter.clear();
        case "attr_filter": attr_filter.clear();
        case "limit":       limit = MISSING_INT;
        case "block":       block = MISSING_INT;
        case "station":     station = MISSING_INT;
        default:
            wreport::error_notfound::throwf("key %s is not valid for a query",
                                            key);
    }
}

} // namespace dballe::core
