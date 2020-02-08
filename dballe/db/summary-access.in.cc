#include "summary_utils.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace db {
namespace summary {

namespace {
inline int get_station_id(const Station& station) { return MISSING_INT; }
inline int get_station_id(const DBStation& station) { return station.id; }
}

/*
 * Summary
 */

template<typename Station>
void CursorMemory<Station>::enq(impl::Enq& enq) const
{
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(station_entry->station.report);
        case "report":      enq.set_string(station_entry->station.report);
        case "ana_id":      enq.set_dballe_int(get_station_id(station_entry->station));
        case "mobile":      enq.set_bool(!station_entry->station.ident.is_missing());
        case "ident":       enq.set_ident(station_entry->station.ident);
        case "lat":         enq.set_lat(station_entry->station.coords.lat);
        case "lon":         enq.set_lon(station_entry->station.coords.lon);
        case "coords":      enq.set_coords(station_entry->station.coords);
        case "station":     enq.set_station(station_entry->station);
        case "datetimemax": if (var_entry->dtrange.is_missing()) return; else enq.set_datetime(var_entry->dtrange.max.year);
        case "datetimemin": if (var_entry->dtrange.is_missing()) return; else enq.set_datetime(var_entry->dtrange.min.year);
        case "yearmax":     if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.max.year);
        case "yearmin":     if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.min.year);
        case "monthmax":    if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.max.month);
        case "monthmin":    if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.min.month);
        case "daymax":      if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.max.day);
        case "daymin":      if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.min.day);
        case "hourmax":     if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.max.hour);
        case "hourmin":     if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.min.hour);
        case "minumax":     if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.max.minute);
        case "minumin":     if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.min.minute);
        case "secmax":      if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.max.second);
        case "secmin":      if (var_entry->dtrange.is_missing()) return; else enq.set_int(var_entry->dtrange.min.second);
        case "level":       enq.set_level(var_entry->var.level);
        case "leveltype1":  enq.set_dballe_int(var_entry->var.level.ltype1);
        case "l1":          enq.set_dballe_int(var_entry->var.level.l1);
        case "leveltype2":  enq.set_dballe_int(var_entry->var.level.ltype2);
        case "l2":          enq.set_dballe_int(var_entry->var.level.l2);
        case "trange":      enq.set_trange(var_entry->var.trange);
        case "pindicator":  enq.set_dballe_int(var_entry->var.trange.pind);
        case "p1":          enq.set_dballe_int(var_entry->var.trange.p1);
        case "p2":          enq.set_dballe_int(var_entry->var.trange.p2);
        case "var":         enq.set_varcode(var_entry->var.varcode);
        case "context_id":  enq.set_int(var_entry->count);
        case "count":       enq.set_int(var_entry->count);
        default:            wreport::error_notfound::throwf("key %s not found on this query result", key);
    }
}

template void CursorMemory<dballe::Station>::enq(impl::Enq& enq) const;
template void CursorMemory<dballe::DBStation>::enq(impl::Enq& enq) const;

}
}
}
