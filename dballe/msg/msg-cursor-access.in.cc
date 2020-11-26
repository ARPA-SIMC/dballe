#include "dballe/msg/cursor.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace impl {
namespace msg {

void CursorStation::enq(impl::Enq& enq) const
{
    if (enq.search_b_values(station_values)) return;
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station.report);
        case "report":      enq.set_string(station.report);
        case "ana_id":      enq.set_dballe_int(station.id);
        case "mobile":      enq.set_bool(!station.ident.is_missing());
        case "ident":       enq.set_ident(station.ident);
        case "lat":         enq.set_lat(station.coords.lat);
        case "lon":         enq.set_lon(station.coords.lon);
        case "coords":      enq.set_coords(station.coords);
        case "station":     enq.set_station(station);
        default:            enq.search_alias_values(station_values);
    }
}

void CursorStationData::enq(impl::Enq& enq) const
{
    if (enq.search_b_value(*cur)) return;
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station.report);
        case "report":      enq.set_string(station.report);
        case "ana_id":      enq.set_dballe_int(station.id);
        case "mobile":      enq.set_bool(!station.ident.is_missing());
        case "ident":       enq.set_ident(station.ident);
        case "lat":         enq.set_lat(station.coords.lat);
        case "lon":         enq.set_lon(station.coords.lon);
        case "coords":      enq.set_coords(station.coords);
        case "station":     enq.set_station(station);
        case "var":         enq.set_varcode(cur->code());
        case "variable":    enq.set_var(cur->get());
        case "attrs":       enq.set_attrs(cur->get());
        case "context_id":  return;
        default:            enq.search_alias_value(*cur);
    }
}

void CursorData::enq(impl::Enq& enq) const
{
    if (enq.search_b_value(*(cur->var))) return;
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station.report);
        case "report":      enq.set_string(station.report);
        case "ana_id":      enq.set_dballe_int(station.id);
        case "mobile":      enq.set_bool(!station.ident.is_missing());
        case "ident":       enq.set_ident(station.ident);
        case "lat":         enq.set_lat(station.coords.lat);
        case "lon":         enq.set_lon(station.coords.lon);
        case "coords":      enq.set_coords(station.coords);
        case "station":     enq.set_station(station);
        case "datetime":    enq.set_datetime(datetime);
        case "year":        enq.set_int(datetime.year);
        case "month":       enq.set_int(datetime.month);
        case "day":         enq.set_int(datetime.day);
        case "hour":        enq.set_int(datetime.hour);
        case "min":         enq.set_int(datetime.minute);
        case "sec":         enq.set_int(datetime.second);
        case "level":       enq.set_level(cur->level);
        case "leveltype1":  enq.set_dballe_int(cur->level.ltype1);
        case "l1":          enq.set_dballe_int(cur->level.l1);
        case "leveltype2":  enq.set_dballe_int(cur->level.ltype2);
        case "l2":          enq.set_dballe_int(cur->level.l2);
        case "trange":      enq.set_trange(cur->trange);
        case "pindicator":  enq.set_dballe_int(cur->trange.pind);
        case "p1":          enq.set_dballe_int(cur->trange.p1);
        case "p2":          enq.set_dballe_int(cur->trange.p2);
        case "var":         enq.set_varcode(cur->var->code());
        case "variable":    enq.set_var(cur->var->get());
        case "attrs":       enq.set_attrs(cur->var->get());
        case "context_id":  return;
        default:            enq.search_alias_value(*(cur->var));
    }
}

}
}
}
