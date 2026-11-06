#include "dballe/core/data.h"
#include "types.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace python {

template <typename Enq> void data_enq_generic(const core::Data& data, Enq& enq)
{
    if (enq.search_b_values(data.values))
        return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) // mklookup
    {
        case "rep_memo":   enq.set_string(data.station.report);
        case "report":     enq.set_string(data.station.report);
        case "ana_id":     enq.set_dballe_int(data.station.id);
        case "mobile":     enq.set_bool(data.station.ident.is_missing());
        case "ident":      enq.set_ident(data.station.ident);
        case "lat":        enq.set_lat(data.station.coords.lat);
        case "lon":        enq.set_lon(data.station.coords.lon);
        case "coords":     enq.set_coords(data.station.coords);
        case "station":    enq.set_station(data.station);
        case "datetime":   enq.set_datetime(data.datetime);
        case "year":       enq.set_int(data.datetime.year);
        case "month":      enq.set_int(data.datetime.month);
        case "day":        enq.set_int(data.datetime.day);
        case "hour":       enq.set_int(data.datetime.hour);
        case "min":        enq.set_int(data.datetime.minute);
        case "sec":        enq.set_int(data.datetime.second);
        case "level":      enq.set_level(data.level);
        case "leveltype1": enq.set_dballe_int(data.level.ltype1);
        case "l1":         enq.set_dballe_int(data.level.l1);
        case "leveltype2": enq.set_dballe_int(data.level.ltype2);
        case "l2":         enq.set_dballe_int(data.level.l2);
        case "trange":     enq.set_trange(data.trange);
        case "pindicator": enq.set_dballe_int(data.trange.pind);
        case "p1":         enq.set_dballe_int(data.trange.p1);
        case "p2":         enq.set_dballe_int(data.trange.p2);
        default:           enq.search_alias_values(data.values);
    }
}

} // namespace python
} // namespace dballe
