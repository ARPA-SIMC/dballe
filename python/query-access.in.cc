#include "record.h"
#include "types.h"
#include "dballe/core/query.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace python {

void query_setpy(core::Query& query, const char* key, unsigned len, PyObject* val)
{
    switch (key) { // mklookup
        case "priority":    query.priomin = query.priomax = dballe_int_from_python(val);
        case "priomax":     query.priomax = dballe_int_from_python(val);
        case "priomin":     query.priomin = dballe_int_from_python(val);
        case "rep_memo":    query.report = dballe_nullable_string_from_python(val);
        case "ana_id":      query.ana_id = dballe_int_from_python(val);
        case "mobile":      query.mobile = dballe_int_from_python(val);
        case "ident":       query.ident = ident_from_python(val);
        case "lat":         { int ival = dballe_int_lat_from_python(val); query.latrange.set(ival, ival); }
        case "lon":         { int ival = dballe_int_lon_from_python(val); query.lonrange.set(ival, ival); }
        case "latmax":      query.latrange.set(query.latrange.imin, dballe_int_lat_from_python(val));
        case "latmin":      query.latrange.set(dballe_int_lat_from_python(val), query.latrange.imax);
        case "lonmax":      query.lonrange.set(query.lonrange.imin, dballe_int_lon_from_python(val));
        case "lonmin":      query.lonrange.set(dballe_int_lon_from_python(val), query.lonrange.imax);
        case "year":        query.dtrange.min.year   = query.dtrange.max.year   = datetime_int16_from_python(val);
        case "month":       query.dtrange.min.month  = query.dtrange.max.month  = datetime_int8_from_python(val);
        case "day":         query.dtrange.min.day    = query.dtrange.max.day    = datetime_int8_from_python(val);
        case "hour":        query.dtrange.min.hour   = query.dtrange.max.hour   = datetime_int8_from_python(val);
        case "min":         query.dtrange.min.minute = query.dtrange.max.minute = datetime_int8_from_python(val);
        case "sec":         query.dtrange.min.second = query.dtrange.max.second = datetime_int8_from_python(val);
        case "yearmax":     query.dtrange.max.year   = datetime_int16_from_python(val);
        case "yearmin":     query.dtrange.min.year   = datetime_int16_from_python(val);
        case "monthmax":    query.dtrange.max.month  = datetime_int8_from_python(val);
        case "monthmin":    query.dtrange.min.month  = datetime_int8_from_python(val);
        case "daymax":      query.dtrange.max.day    = datetime_int8_from_python(val);
        case "daymin":      query.dtrange.min.day    = datetime_int8_from_python(val);
        case "hourmax":     query.dtrange.max.hour   = datetime_int8_from_python(val);
        case "hourmin":     query.dtrange.min.hour   = datetime_int8_from_python(val);
        case "minumax":     query.dtrange.max.minute = datetime_int8_from_python(val);
        case "minumin":     query.dtrange.min.minute = datetime_int8_from_python(val);
        case "secmax":      query.dtrange.max.second = datetime_int8_from_python(val);
        case "secmin":      query.dtrange.min.second = datetime_int8_from_python(val);
        case "leveltype1":  query.level.ltype1 = dballe_int_from_python(val);
        case "l1":          query.level.l1     = dballe_int_from_python(val);
        case "leveltype2":  query.level.ltype2 = dballe_int_from_python(val);
        case "l2":          query.level.l2     = dballe_int_from_python(val);
        case "pindicator":  query.trange.pind  = dballe_int_from_python(val);
        case "p1":          query.trange.p1    = dballe_int_from_python(val);
        case "p2":          query.trange.p2    = dballe_int_from_python(val);
        case "var":         query.varcodes.clear(); query.varcodes.insert(varcode_from_python(val));
        case "varlist":     query.varcodes = varcodes_from_python(val);
        case "query":       query.query = dballe_nullable_string_from_python(val);
        case "ana_filter":  query.ana_filter = dballe_nullable_string_from_python(val);
        case "data_filter": query.data_filter = dballe_nullable_string_from_python(val);
        case "attr_filter": query.attr_filter = dballe_nullable_string_from_python(val);
        case "limit":       query.limit = dballe_int_from_python(val);
        case "datetime":    query.dtrange.min = query.dtrange.max = datetime_from_python(val);
        case "datetimemin": query.dtrange.min = datetime_from_python(val);
        case "datetimemax": query.dtrange.max = datetime_from_python(val);
        case "level":       query.level = level_from_python(val);
        case "trange":      query.trange = trange_from_python(val);
        default:            PyErr_Format(PyExc_KeyError, "key %s not valid for a query", key); throw PythonException();
    }
}

}
}
