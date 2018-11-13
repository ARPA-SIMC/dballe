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
        case "priority":    query.prio_min = query.prio_max = dballe_int_from_python(val);
        case "priomax":     query.prio_max = dballe_int_from_python(val);
        case "priomin":     query.prio_min = dballe_int_from_python(val);
        case "rep_memo":    query.rep_memo = dballe_nullable_string_from_python(val);
        case "ana_id":      query.ana_id = dballe_int_from_python(val);
        case "mobile":      query.mobile = dballe_int_from_python(val);
        case "ident":       query.ident = ident_from_python(val);
        case "lat":         { int ival = dballe_int_lat_from_python(val); query.latrange.set(ival, ival); }
        case "lon":         { int ival = dballe_int_lon_from_python(val); query.lonrange.set(ival, ival); }
        case "latmax":      query.latrange.set(query.latrange.imin, dballe_int_lat_from_python(val));
        case "latmin":      query.latrange.set(dballe_int_lat_from_python(val), query.latrange.imax);
        case "lonmax":      query.lonrange.set(query.lonrange.imin, dballe_int_lon_from_python(val));
        case "lonmin":      query.lonrange.set(dballe_int_lon_from_python(val), query.lonrange.imax);
        case "year":        query.datetime.min.year   = query.datetime.max.year   = datetime_int16_from_python(val);
        case "month":       query.datetime.min.month  = query.datetime.max.month  = datetime_int8_from_python(val);
        case "day":         query.datetime.min.day    = query.datetime.max.day    = datetime_int8_from_python(val);
        case "hour":        query.datetime.min.hour   = query.datetime.max.hour   = datetime_int8_from_python(val);
        case "min":         query.datetime.min.minute = query.datetime.max.minute = datetime_int8_from_python(val);
        case "sec":         query.datetime.min.second = query.datetime.max.second = datetime_int8_from_python(val);
        case "yearmax":     query.datetime.max.year   = datetime_int16_from_python(val);
        case "yearmin":     query.datetime.min.year   = datetime_int16_from_python(val);
        case "monthmax":    query.datetime.max.month  = datetime_int8_from_python(val);
        case "monthmin":    query.datetime.min.month  = datetime_int8_from_python(val);
        case "daymax":      query.datetime.max.day    = datetime_int8_from_python(val);
        case "daymin":      query.datetime.min.day    = datetime_int8_from_python(val);
        case "hourmax":     query.datetime.max.hour   = datetime_int8_from_python(val);
        case "hourmin":     query.datetime.min.hour   = datetime_int8_from_python(val);
        case "minumax":     query.datetime.max.minute = datetime_int8_from_python(val);
        case "minumin":     query.datetime.min.minute = datetime_int8_from_python(val);
        case "secmax":      query.datetime.max.second = datetime_int8_from_python(val);
        case "secmin":      query.datetime.min.second = datetime_int8_from_python(val);
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
        case "datetime":    query.datetime.min = query.datetime.max = datetime_from_python(val);
        case "datetimemin": query.datetime.min = datetime_from_python(val);
        case "datetimemax": query.datetime.max = datetime_from_python(val);
        case "level":       query.level = level_from_python(val);
        case "trange":      query.trange = trange_from_python(val);
        default:            PyErr_Format(PyExc_KeyError, "key %s not valid for a query", key); throw PythonException();
    }
}

}
}
