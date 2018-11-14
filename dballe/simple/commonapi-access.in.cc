#include "commonapi.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace fortran {

bool CommonAPIImplementation::_seti(const char* key, unsigned len, int val)
{
    switch (key) { // mklookup
        case "priority":    input_query.prio_min = input_query.prio_max = val;
        case "priomax":     input_query.prio_max = val;
        case "priomin":     input_query.prio_min = val;
        case "rep_memo":    throw error_consistency("cannot seti rep_memo");
        case "ana_id":      input_query.ana_id = input_data.station.id = val;
        case "mobile":      input_query.mobile = val;
        case "ident":       throw error_consistency("cannot seti ident");
        case "lat":         input_data.station.coords.set_lat(val); input_query.latrange.set(val, val);
        case "lon":         input_data.station.coords.set_lon(val); input_query.lonrange.set(val, val);
        case "latmax":      input_query.latrange.imax = val == MISSING_INT ? LatRange::IMAX : val;
        case "latmin":      input_query.latrange.imin = val == MISSING_INT ? LatRange::IMIN : val;
        case "lonmax":      input_query.lonrange.imax = val;
        case "lonmin":      input_query.lonrange.imin = val;
        case "year":        input_query.datetime.min.year   = input_query.datetime.max.year   = input_data.datetime.year   = val;
        case "month":       input_query.datetime.min.month  = input_query.datetime.max.month  = input_data.datetime.month  = val;
        case "day":         input_query.datetime.min.day    = input_query.datetime.max.day    = input_data.datetime.day    = val;
        case "hour":        input_query.datetime.min.hour   = input_query.datetime.max.hour   = input_data.datetime.hour   = val;
        case "min":         input_query.datetime.min.minute = input_query.datetime.max.minute = input_data.datetime.minute = val;
        case "sec":         input_query.datetime.min.second = input_query.datetime.max.second = input_data.datetime.second = val;
        case "yearmax":     input_query.datetime.max.year   = val;
        case "yearmin":     input_query.datetime.min.year   = val;
        case "monthmax":    input_query.datetime.max.month  = val;
        case "monthmin":    input_query.datetime.min.month  = val;
        case "daymax":      input_query.datetime.max.day    = val;
        case "daymin":      input_query.datetime.min.day    = val;
        case "hourmax":     input_query.datetime.max.hour   = val;
        case "hourmin":     input_query.datetime.min.hour   = val;
        case "minumax":     input_query.datetime.max.minute = val;
        case "minumin":     input_query.datetime.min.minute = val;
        case "secmax":      input_query.datetime.max.second = val;
        case "secmin":      input_query.datetime.min.second = val;
        case "leveltype1":  input_query.level.ltype1 = input_data.level.ltype1 = val;
        case "l1":          input_query.level.l1     = input_data.level.l1     = val;
        case "leveltype2":  input_query.level.ltype2 = input_data.level.ltype2 = val;
        case "l2":          input_query.level.l2     = input_data.level.l2     = val;
        case "pindicator":  input_query.trange.pind  = input_data.trange.pind  = val;
        case "p1":          input_query.trange.p1    = input_data.trange.p1    = val;
        case "p2":          input_query.trange.p2    = input_data.trange.p2    = val;
        case "var":         throw error_consistency("cannot seti var");
        case "varlist":     throw error_consistency("cannot seti varlist");
        case "context_id":  throw error_consistency("cannot set context_id");
        case "query":       throw error_consistency("cannot seti query");
        case "ana_filter":  throw error_consistency("cannot seti ana_filter");
        case "data_filter": throw error_consistency("cannot seti data_filter");
        case "attr_filter": throw error_consistency("cannot seti attr_filter");
        case "limit":       input_query.limit = val;
        case "block":       input_query.block = val;   input_data.values.set(WR_VAR(0, 1, 1), val);
        case "station":     input_query.station = val; input_data.values.set(WR_VAR(0, 1, 2), val);
        default: return false;
    }
    return true;
}

bool CommonAPIImplementation::_setd(const char* key, unsigned len, double val)
{
    switch (key) { // mklookup
        case "priority":    input_query.prio_min = input_query.prio_max = val;
        case "priomax":     input_query.prio_max = val;
        case "priomin":     input_query.prio_min = val;
        case "rep_memo":    throw error_consistency("cannot setd rep_memo");
        case "ana_id":      input_query.ana_id = input_data.station.id = val;
        case "mobile":      input_query.mobile = val;
        case "ident":       throw error_consistency("cannot setd ident");
        case "lat":         input_data.station.coords.set_lat(val); input_query.latrange.set(val, val);
        case "lon":         input_data.station.coords.set_lon(val); input_query.lonrange.set(val, val);
        case "latmax":      input_query.latrange.imax = Coords::lat_to_int(val);
        case "latmin":      input_query.latrange.imin = Coords::lat_to_int(val);
        case "lonmax":      input_query.lonrange.imax = Coords::lon_to_int(val);
        case "lonmin":      input_query.lonrange.imin = Coords::lon_to_int(val);
        case "year":        input_query.datetime.min.year   = input_query.datetime.max.year   = input_data.datetime.year   = val;
        case "month":       input_query.datetime.min.month  = input_query.datetime.max.month  = input_data.datetime.month  = val;
        case "day":         input_query.datetime.min.day    = input_query.datetime.max.day    = input_data.datetime.day    = val;
        case "hour":        input_query.datetime.min.hour   = input_query.datetime.max.hour   = input_data.datetime.hour   = val;
        case "min":         input_query.datetime.min.minute = input_query.datetime.max.minute = input_data.datetime.minute = val;
        case "sec":         input_query.datetime.min.second = input_query.datetime.max.second = input_data.datetime.second = val;
        case "yearmax":     input_query.datetime.max.year   = val;
        case "yearmin":     input_query.datetime.min.year   = val;
        case "monthmax":    input_query.datetime.max.month  = val;
        case "monthmin":    input_query.datetime.min.month  = val;
        case "daymax":      input_query.datetime.max.day    = val;
        case "daymin":      input_query.datetime.min.day    = val;
        case "hourmax":     input_query.datetime.max.hour   = val;
        case "hourmin":     input_query.datetime.min.hour   = val;
        case "minumax":     input_query.datetime.max.minute = val;
        case "minumin":     input_query.datetime.min.minute = val;
        case "secmax":      input_query.datetime.max.second = val;
        case "secmin":      input_query.datetime.min.second = val;
        case "leveltype1":  input_query.level.ltype1 = input_data.level.ltype1 = val;
        case "l1":          input_query.level.l1     = input_data.level.l1     = val;
        case "leveltype2":  input_query.level.ltype2 = input_data.level.ltype2 = val;
        case "l2":          input_query.level.l2     = input_data.level.l2     = val;
        case "pindicator":  input_query.trange.pind  = input_data.trange.pind  = val;
        case "p1":          input_query.trange.p1    = input_data.trange.p1    = val;
        case "p2":          input_query.trange.p2    = input_data.trange.p2    = val;
        case "var":         throw error_consistency("cannot setd var");
        case "varlist":     throw error_consistency("cannot setd varlist");
        case "context_id":  throw error_consistency("cannot set context_id");
        case "query":       throw error_consistency("cannot setd query");
        case "ana_filter":  throw error_consistency("cannot setd ana_filter");
        case "data_filter": throw error_consistency("cannot setd data_filter");
        case "attr_filter": throw error_consistency("cannot setd attr_filter");
        case "limit":       input_query.limit = val;
        case "block":       input_query.block = val;   input_data.values.set(WR_VAR(0, 1, 1), val);
        case "station":     input_query.station = val; input_data.values.set(WR_VAR(0, 1, 2), val);
        default: return false;
    }
    return true;
}

bool CommonAPIImplementation::_setc(const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    input_query.prio_min = input_query.prio_max = strtol(val, nullptr, 10);
        case "priomax":     input_query.prio_max = strtol(val, nullptr, 10);
        case "priomin":     input_query.prio_min = strtol(val, nullptr, 10);
        case "rep_memo":    input_query.rep_memo = input_data.station.report = val;
        case "ana_id":      input_query.ana_id = input_data.station.id = strtol(val, nullptr, 10);
        case "mobile":      input_query.mobile = strtol(val, nullptr, 10);
        case "ident":       input_query.ident = input_data.station.ident = val;
        case "lat":         { int ival = strtol(val, nullptr, 10); input_data.station.coords.set_lat(ival); input_query.latrange.set(ival, ival); }
        case "lon":         { int ival = strtol(val, nullptr, 10); input_data.station.coords.set_lon(ival); input_query.lonrange.set(ival, ival); }
        case "latmax":      input_query.latrange.imax = strtol(val, nullptr, 10);
        case "latmin":      input_query.latrange.imin = strtol(val, nullptr, 10);
        case "lonmax":      input_query.lonrange.imax = strtol(val, nullptr, 10);
        case "lonmin":      input_query.lonrange.imin = strtol(val, nullptr, 10);
        case "year":        input_query.datetime.min.year   = input_query.datetime.max.year   = input_data.datetime.year   = strtol(val, nullptr, 10);
        case "month":       input_query.datetime.min.month  = input_query.datetime.max.month  = input_data.datetime.month  = strtol(val, nullptr, 10);
        case "day":         input_query.datetime.min.day    = input_query.datetime.max.day    = input_data.datetime.day    = strtol(val, nullptr, 10);
        case "hour":        input_query.datetime.min.hour   = input_query.datetime.max.hour   = input_data.datetime.hour   = strtol(val, nullptr, 10);
        case "min":         input_query.datetime.min.minute = input_query.datetime.max.minute = input_data.datetime.minute = strtol(val, nullptr, 10);
        case "sec":         input_query.datetime.min.second = input_query.datetime.max.second = input_data.datetime.second = strtol(val, nullptr, 10);
        case "yearmax":     input_query.datetime.max.year = strtol(val, nullptr, 10);
        case "yearmin":     input_query.datetime.min.year = strtol(val, nullptr, 10);
        case "monthmax":    input_query.datetime.max.month = strtol(val, nullptr, 10);
        case "monthmin":    input_query.datetime.min.month = strtol(val, nullptr, 10);
        case "daymax":      input_query.datetime.max.day = strtol(val, nullptr, 10);
        case "daymin":      input_query.datetime.min.day = strtol(val, nullptr, 10);
        case "hourmax":     input_query.datetime.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     input_query.datetime.min.hour = strtol(val, nullptr, 10);
        case "minumax":     input_query.datetime.max.minute = strtol(val, nullptr, 10);
        case "minumin":     input_query.datetime.min.minute = strtol(val, nullptr, 10);
        case "secmax":      input_query.datetime.max.second = strtol(val, nullptr, 10);
        case "secmin":      input_query.datetime.min.second = strtol(val, nullptr, 10);
        case "leveltype1":  input_query.level.ltype1 = input_data.level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          input_query.level.l1     = input_data.level.l1     = strtol(val, nullptr, 10);
        case "leveltype2":  input_query.level.ltype2 = input_data.level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          input_query.level.l2     = input_data.level.l2     = strtol(val, nullptr, 10);
        case "pindicator":  input_query.trange.pind  = input_data.trange.pind  = strtol(val, nullptr, 10);
        case "p1":          input_query.trange.p1    = input_data.trange.p1    = strtol(val, nullptr, 10);
        case "p2":          input_query.trange.p2    = input_data.trange.p2    = strtol(val, nullptr, 10);
        case "var":         input_query.varcodes.clear(); input_query.varcodes.insert(resolve_varcode(val));
        case "varlist":     input_query.varcodes.clear(); resolve_varlist(val, input_query.varcodes);
        case "context_id":  throw error_consistency("cannot set context_id");
        case "query":       input_query.query = val;
        case "ana_filter":  input_query.ana_filter = val;
        case "data_filter": input_query.data_filter = val;
        case "attr_filter": input_query.attr_filter = val;
        case "limit":       input_query.limit = strtol(val, nullptr, 10);
        case "block":       input_query.block   = strtol(val, nullptr, 10); input_data.values.set(WR_VAR(0, 1, 1), val);
        case "station":     input_query.station = strtol(val, nullptr, 10); input_data.values.set(WR_VAR(0, 1, 2), val);
        default: return false;
    }
    return true;
}

bool CommonAPIImplementation::_unset(const char* key, unsigned len)
{
    switch (key) { // mklookup
        case "priority":    input_query.prio_min = input_query.prio_max = MISSING_INT;
        case "priomax":     input_query.prio_max = MISSING_INT;
        case "priomin":     input_query.prio_min = MISSING_INT;
        case "rep_memo":    input_query.rep_memo.clear(); input_data.station.report.clear();
        case "ana_id":      input_query.ana_id = input_data.station.id = MISSING_INT;
        case "mobile":      input_query.mobile = MISSING_INT;
        case "ident":       input_data.station.ident.clear();
        case "lat":         input_data.station.coords = Coords(); input_query.latrange = LatRange();
        case "lon":         input_data.station.coords = Coords(); input_query.lonrange = LonRange();
        case "latmax":      input_query.latrange.imax = LatRange::IMAX;
        case "latmin":      input_query.latrange.imin = LatRange::IMIN;
        case "lonmax":      input_query.lonrange.imax = MISSING_INT;
        case "lonmin":      input_query.lonrange.imin = MISSING_INT;
        case "year":        input_query.datetime.min.year   = input_query.datetime.max.year   = input_data.datetime.year   = 0xffff;
        case "month":       input_query.datetime.min.month  = input_query.datetime.max.month  = input_data.datetime.month  = 0xff;
        case "day":         input_query.datetime.min.day    = input_query.datetime.max.day    = input_data.datetime.day    = 0xff;
        case "hour":        input_query.datetime.min.hour   = input_query.datetime.max.hour   = input_data.datetime.hour   = 0xff;
        case "min":         input_query.datetime.min.minute = input_query.datetime.max.minute = input_data.datetime.minute = 0xff;
        case "sec":         input_query.datetime.min.second = input_query.datetime.max.second = input_data.datetime.second = 0xff;
        case "yearmax":     input_query.datetime.max.year = 0xffff;
        case "yearmin":     input_query.datetime.min.year = 0xffff;
        case "monthmax":    input_query.datetime.max.month = 0xff;
        case "monthmin":    input_query.datetime.min.month = 0xff;
        case "daymax":      input_query.datetime.max.day = 0xff;
        case "daymin":      input_query.datetime.min.day = 0xff;
        case "hourmax":     input_query.datetime.max.hour = 0xff;
        case "hourmin":     input_query.datetime.min.hour = 0xff;
        case "minumax":     input_query.datetime.max.minute = 0xff;
        case "minumin":     input_query.datetime.min.minute = 0xff;
        case "secmax":      input_query.datetime.max.second = 0xff;
        case "secmin":      input_query.datetime.min.second = 0xff;
        case "leveltype1":  input_query.level.ltype1 = input_data.level.ltype1 = MISSING_INT;
        case "l1":          input_query.level.l1     = input_data.level.l1     = MISSING_INT;
        case "leveltype2":  input_query.level.ltype2 = input_data.level.ltype2 = MISSING_INT;
        case "l2":          input_query.level.l2     = input_data.level.l2     = MISSING_INT;
        case "pindicator":  input_query.trange.pind  = input_data.trange.pind  = MISSING_INT;
        case "p1":          input_query.trange.p1    = input_data.trange.p1    = MISSING_INT;
        case "p2":          input_query.trange.p2    = input_data.trange.p2    = MISSING_INT;
        case "var":         input_query.varcodes.clear();
        case "varlist":     input_query.varcodes.clear();
        case "context_id":  throw error_consistency("cannot set/unset context_id");
        case "query":       input_query.query.clear();
        case "ana_filter":  input_query.ana_filter.clear();
        case "data_filter": input_query.data_filter.clear();
        case "attr_filter": input_query.attr_filter.clear();
        case "limit":       input_query.limit = MISSING_INT;
        case "block":       input_query.block   = MISSING_INT; input_data.values.unset(WR_VAR(0, 1, 1));
        case "station":     input_query.station = MISSING_INT; input_data.values.unset(WR_VAR(0, 1, 2));
        default: return false;
    }
    return true;
}

}
}
