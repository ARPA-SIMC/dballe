#include "cursor.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace impl {
namespace msg {

template<typename Enq>
void CursorStation::enq_generic(Enq& enq) const
{
    if (enq.search_b_values(station_values)) return;
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station.report);
        case "ana_id":      enq.set_dballe_int(station.id);
        case "mobile":      enq.set_bool(!station.ident.is_missing());
        case "ident":       enq.set_ident(station.ident);
        case "lat":         enq.set_lat(station.coords.lat);
        case "lon":         enq.set_lon(station.coords.lon);
        default:            enq.search_alias_values(station_values);
    }
}

template<typename Enq>
void CursorStationData::enq_generic(Enq& enq) const
{
    if (enq.search_b_value(*cur)) return;
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station.report);
        case "ana_id":      enq.set_dballe_int(station.id);
        case "mobile":      enq.set_bool(!station.ident.is_missing());
        case "ident":       enq.set_ident(station.ident);
        case "lat":         enq.set_lat(station.coords.lat);
        case "lon":         enq.set_lon(station.coords.lon);
        case "var":         enq.set_var((*cur)->code());
        case "context_id":  return;
        default:            enq.search_alias_value(*cur);
    }
}

template<typename Enq>
void CursorData::enq_generic(Enq& enq) const
{
    if (enq.search_b_value(*(cur->var))) return;
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station.report);
        case "ana_id":      enq.set_dballe_int(station.id);
        case "mobile":      enq.set_bool(!station.ident.is_missing());
        case "ident":       enq.set_ident(station.ident);
        case "lat":         enq.set_lat(station.coords.lat);
        case "lon":         enq.set_lon(station.coords.lon);
        case "year":        enq.set_int(datetime.year);
        case "month":       enq.set_int(datetime.month);
        case "day":         enq.set_int(datetime.day);
        case "hour":        enq.set_int(datetime.hour);
        case "min":         enq.set_int(datetime.minute);
        case "sec":         enq.set_int(datetime.second);
        case "leveltype1":  enq.set_dballe_int(cur->level.ltype1);
        case "l1":          enq.set_dballe_int(cur->level.l1);
        case "leveltype2":  enq.set_dballe_int(cur->level.ltype2);
        case "l2":          enq.set_dballe_int(cur->level.l2);
        case "pindicator":  enq.set_dballe_int(cur->trange.pind);
        case "p1":          enq.set_dballe_int(cur->trange.p1);
        case "p2":          enq.set_dballe_int(cur->trange.p2);
        case "var":         enq.set_var((*(cur->var))->code());
        case "context_id":  return;
        default:            enq.search_alias_value(*(cur->var));
    }
}

}
}
}
