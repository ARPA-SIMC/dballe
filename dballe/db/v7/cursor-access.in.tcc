#include "cursor.h"
#include "dballe/core/fortran.h"
#include "repinfo.h"
#include "levtr.h"
#include "dballe/core/access-impl.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {


/*
 * Stations
 */

template<typename Enq>
void Stations::enq_generic(Enq& enq) const
{
    if (enq.search_b_values(values())) return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(get_priority());
        case "rep_memo":    enq.set_string(cur->station.report);
        case "ana_id":      enq.set_dballe_int(cur->station.id);
        case "mobile":      enq.set_bool(cur->station.ident.is_missing());
        case "ident":       enq.set_ident(cur->station.ident);
        case "lat":         enq.set_lat(cur->station.coords.lat);
        case "lon":         enq.set_lon(cur->station.coords.lon);
        default:            enq.search_alias_values(values());
    }
}


/*
 * StationData
 */

template<typename Enq>
void StationData::enq_generic(Enq& enq) const
{
    if (enq.search_b_value(cur->value)) return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(get_priority());
        case "rep_memo":    enq.set_string(cur->station.report);
        case "ana_id":      enq.set_dballe_int(cur->station.id);
        case "mobile":      enq.set_bool(cur->station.ident.is_missing());
        case "ident":       enq.set_ident(cur->station.ident);
        case "lat":         enq.set_lat(cur->station.coords.lat);
        case "lon":         enq.set_lon(cur->station.coords.lon);
        case "var":         enq.set_var(cur->value.code());
        case "context_id":  enq.set_dballe_int(cur->value.data_id);
        default:            enq.search_alias_value(cur->value);
    }
}

/*
 * Data
 */

template<typename Enq>
void Data::enq_generic(Enq& enq) const
{
    if (enq.search_b_value(cur->value)) return;

    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(get_priority());
        case "rep_memo":    enq.set_string(cur->station.report);
        case "ana_id":      enq.set_dballe_int(cur->station.id);
        case "mobile":      enq.set_bool(cur->station.ident.is_missing());
        case "ident":       enq.set_ident(cur->station.ident);
        case "lat":         enq.set_lat(cur->station.coords.lat);
        case "lon":         enq.set_lon(cur->station.coords.lon);
        case "year":        enq.set_int(cur->datetime.year);
        case "month":       enq.set_int(cur->datetime.month);
        case "day":         enq.set_int(cur->datetime.day);
        case "hour":        enq.set_int(cur->datetime.hour);
        case "min":         enq.set_int(cur->datetime.minute);
        case "sec":         enq.set_int(cur->datetime.second);
        case "leveltype1":  enq.set_dballe_int(get_levtr().level.ltype1);
        case "l1":          enq.set_dballe_int(get_levtr().level.l1);
        case "leveltype2":  enq.set_dballe_int(get_levtr().level.ltype2);
        case "l2":          enq.set_dballe_int(get_levtr().level.l2);
        case "pindicator":  enq.set_dballe_int(get_levtr().trange.pind);
        case "p1":          enq.set_dballe_int(get_levtr().trange.p1);
        case "p2":          enq.set_dballe_int(get_levtr().trange.p2);
        case "var":         enq.set_var(cur->value.code());
        case "context_id":  enq.set_dballe_int(cur->value.data_id);
        default:            enq.search_alias_value(cur->value);
    }
}

/*
 * Summary
 */

template<typename Enq>
void Summary::enq_generic(Enq& enq) const
{
    const auto key = enq.key;
    const auto len = enq.len;

    switch (key) { // mklookup
        case "priority":    enq.set_int(get_priority());
        case "rep_memo":    enq.set_string(cur->station.report);
        case "ana_id":      enq.set_dballe_int(cur->station.id);
        case "mobile":      enq.set_bool(cur->station.ident.is_missing());
        case "ident":       enq.set_ident(cur->station.ident);
        case "lat":         enq.set_lat(cur->station.coords.lat);
        case "lon":         enq.set_lon(cur->station.coords.lon);
        case "yearmax":     if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.max.year);
        case "yearmin":     if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.min.year);
        case "monthmax":    if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.max.month);
        case "monthmin":    if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.min.month);
        case "daymax":      if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.max.day);
        case "daymin":      if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.min.day);
        case "hourmax":     if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.max.hour);
        case "hourmin":     if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.min.hour);
        case "minumax":     if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.max.minute);
        case "minumin":     if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.min.minute);
        case "secmax":      if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.max.second);
        case "secmin":      if (cur->dtrange.is_missing()) return; else enq.set_int(cur->dtrange.min.second);
        case "leveltype1":  enq.set_dballe_int(get_levtr().level.ltype1);
        case "l1":          enq.set_dballe_int(get_levtr().level.l1);
        case "leveltype2":  enq.set_dballe_int(get_levtr().level.ltype2);
        case "l2":          enq.set_dballe_int(get_levtr().level.l2);
        case "pindicator":  enq.set_dballe_int(get_levtr().trange.pind);
        case "p1":          enq.set_dballe_int(get_levtr().trange.p1);
        case "p2":          enq.set_dballe_int(get_levtr().trange.p2);
        case "var":         enq.set_var(cur->code);
        case "context_id":  enq.set_int(cur->count);
        case "count":       enq.set_int(cur->count);
        default:            wreport::error_notfound::throwf("key %s not found on this query result", key);
    }
}

}
}
}
}
