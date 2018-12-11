#include "types.h"
#include "dballe/core/data.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace python {

void data_setpy(core::Data& data, const char* key, unsigned len, PyObject* val)
{
    switch (key) { // mklookup
        case "rep_memo":    data.station.report = dballe_nullable_string_from_python(val);
        case "ana_id":      data.station.id = dballe_int_from_python(val);
        case "ident":       data.station.ident = ident_from_python(val);
        case "lat":         set_lat_from_python(val, data.station.coords);
        case "lon":         set_lon_from_python(val, data.station.coords);
        case "year":        data.datetime.year   = datetime_int16_from_python(val);
        case "month":       data.datetime.month  = datetime_int8_from_python(val);
        case "day":         data.datetime.day    = datetime_int8_from_python(val);
        case "hour":        data.datetime.hour   = datetime_int8_from_python(val);
        case "min":         data.datetime.minute = datetime_int8_from_python(val);
        case "sec":         data.datetime.second = datetime_int8_from_python(val);
        case "leveltype1":  data.level.ltype1 = dballe_int_from_python(val);
        case "l1":          data.level.l1     = dballe_int_from_python(val);
        case "leveltype2":  data.level.ltype2 = dballe_int_from_python(val);
        case "l2":          data.level.l2     = dballe_int_from_python(val);
        case "pindicator":  data.trange.pind  = dballe_int_from_python(val);
        case "p1":          data.trange.p1    = dballe_int_from_python(val);
        case "p2":          data.trange.p2    = dballe_int_from_python(val);
        case "datetime":    data.datetime = datetime_from_python(val);
        case "level":       data.level = level_from_python(val);
        case "trange":      data.trange = trange_from_python(val);
        default:            set_values_from_python(data.values, resolve_varcode(key), val);
    }
}

}
}
