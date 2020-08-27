#include "dballe/db/v7/cursor.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/levtr.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {


/*
 * Stations
 */

void Stations::enq(impl::Enq& enq) const
{
    if (enq.search_b_values(rows.values())) return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(rows.get_priority());
        case "rep_memo":    enq.set_string(row().station.report);
        case "report":      enq.set_string(row().station.report);
        case "ana_id":      enq.set_dballe_int(row().station.id);
        case "mobile":      enq.set_bool(!row().station.ident.is_missing());
        case "ident":       enq.set_ident(row().station.ident);
        case "lat":         enq.set_lat(row().station.coords.lat);
        case "lon":         enq.set_lon(row().station.coords.lon);
        case "coords":      enq.set_coords(row().station.coords);
        case "station":     enq.set_station(row().station);
        default:            enq.search_alias_values(rows.values());
    }
}


/*
 * StationData
 */

void StationData::enq(impl::Enq& enq) const
{
    if (enq.search_b_value(row().value)) return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(rows.get_priority());
        case "rep_memo":    enq.set_string(row().station.report);
        case "report":      enq.set_string(row().station.report);
        case "ana_id":      enq.set_dballe_int(row().station.id);
        case "mobile":      enq.set_bool(!row().station.ident.is_missing());
        case "ident":       enq.set_ident(row().station.ident);
        case "lat":         enq.set_lat(row().station.coords.lat);
        case "lon":         enq.set_lon(row().station.coords.lon);
        case "coords":      enq.set_coords(row().station.coords);
        case "station":     enq.set_station(row().station);
        case "var":         enq.set_varcode(row().value.code());
        case "variable":    enq.set_var(row().value.get());
        case "attrs":       enq.set_attrs(row().value.get());
        case "context_id":  enq.set_dballe_int(row().value.data_id);
        default:            enq.search_alias_value(row().value);
    }
}

/*
 * Data
 */

void Data::enq(impl::Enq& enq) const
{
    if (enq.search_b_value(row().value)) return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(rows.get_priority());
        case "rep_memo":    enq.set_string(row().station.report);
        case "report":      enq.set_string(row().station.report);
        case "ana_id":      enq.set_dballe_int(row().station.id);
        case "mobile":      enq.set_bool(!row().station.ident.is_missing());
        case "ident":       enq.set_ident(row().station.ident);
        case "lat":         enq.set_lat(row().station.coords.lat);
        case "lon":         enq.set_lon(row().station.coords.lon);
        case "coords":      enq.set_coords(row().station.coords);
        case "station":     enq.set_station(row().station);
        case "datetime":    enq.set_datetime(row().datetime);
        case "year":        enq.set_int(row().datetime.year);
        case "month":       enq.set_int(row().datetime.month);
        case "day":         enq.set_int(row().datetime.day);
        case "hour":        enq.set_int(row().datetime.hour);
        case "min":         enq.set_int(row().datetime.minute);
        case "sec":         enq.set_int(row().datetime.second);
        case "level":       enq.set_level(rows.get_levtr().level);
        case "leveltype1":  enq.set_dballe_int(rows.get_levtr().level.ltype1);
        case "l1":          enq.set_dballe_int(rows.get_levtr().level.l1);
        case "leveltype2":  enq.set_dballe_int(rows.get_levtr().level.ltype2);
        case "l2":          enq.set_dballe_int(rows.get_levtr().level.l2);
        case "trange":      enq.set_trange(rows.get_levtr().trange);
        case "pindicator":  enq.set_dballe_int(rows.get_levtr().trange.pind);
        case "p1":          enq.set_dballe_int(rows.get_levtr().trange.p1);
        case "p2":          enq.set_dballe_int(rows.get_levtr().trange.p2);
        case "var":         enq.set_varcode(row().value.code());
        case "variable":    enq.set_var(row().value.get());
        case "attrs":       enq.set_attrs(row().value.get());
        case "context_id":  enq.set_dballe_int(row().value.data_id);
        default:            enq.search_alias_value(row().value);
    }
}

/*
 * Summary
 */

void Summary::enq(impl::Enq& enq) const
{
    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(rows.get_priority());
        case "rep_memo":    enq.set_string(row().station.report);
        case "report":      enq.set_string(row().station.report);
        case "ana_id":      enq.set_dballe_int(row().station.id);
        case "mobile":      enq.set_bool(!row().station.ident.is_missing());
        case "ident":       enq.set_ident(row().station.ident);
        case "lat":         enq.set_lat(row().station.coords.lat);
        case "lon":         enq.set_lon(row().station.coords.lon);
        case "coords":      enq.set_coords(row().station.coords);
        case "station":     enq.set_station(row().station);
        case "datetimemax": if (row().dtrange.is_missing()) return; else enq.set_datetime(row().dtrange.max);
        case "datetimemin": if (row().dtrange.is_missing()) return; else enq.set_datetime(row().dtrange.min);
        case "yearmax":     if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.max.year);
        case "yearmin":     if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.min.year);
        case "monthmax":    if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.max.month);
        case "monthmin":    if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.min.month);
        case "daymax":      if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.max.day);
        case "daymin":      if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.min.day);
        case "hourmax":     if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.max.hour);
        case "hourmin":     if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.min.hour);
        case "minumax":     if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.max.minute);
        case "minumin":     if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.min.minute);
        case "secmax":      if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.max.second);
        case "secmin":      if (row().dtrange.is_missing()) return; else enq.set_int(row().dtrange.min.second);
        case "level":       enq.set_level(rows.get_levtr().level);
        case "leveltype1":  enq.set_dballe_int(rows.get_levtr().level.ltype1);
        case "l1":          enq.set_dballe_int(rows.get_levtr().level.l1);
        case "leveltype2":  enq.set_dballe_int(rows.get_levtr().level.ltype2);
        case "l2":          enq.set_dballe_int(rows.get_levtr().level.l2);
        case "trange":      enq.set_trange(rows.get_levtr().trange);
        case "pindicator":  enq.set_dballe_int(rows.get_levtr().trange.pind);
        case "p1":          enq.set_dballe_int(rows.get_levtr().trange.p1);
        case "p2":          enq.set_dballe_int(rows.get_levtr().trange.p2);
        case "var":         enq.set_varcode(row().code);
        case "context_id":  enq.set_int(row().count);
        case "count":       enq.set_int(row().count);
        default:            wreport::error_notfound::throwf("key %s not found on this query result", key);
    }
}

}
}
}
}
