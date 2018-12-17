#include "summary.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace db {
namespace summary {

namespace {
int get_station_id(const Station& station) { return MISSING_INT; }
int get_station_id(const DBStation& station) { return station.id; }
}

/*
 * Summary
 */

template<typename Station> template<typename Enq>
void Cursor<Station>::enq_generic(Enq& enq) const
{
    const auto key = enq.key;
    const auto len = enq.len;
    switch (key) { // mklookup
        case "priority":    return;
        case "rep_memo":    enq.set_string(cur->station_entry.station.report);
        case "ana_id":      enq.set_dballe_int(get_station_id(cur->station_entry.station));
        case "mobile":      enq.set_bool(!cur->station_entry.station.ident.is_missing());
        case "ident":       enq.set_ident(cur->station_entry.station.ident);
        case "lat":         enq.set_lat(cur->station_entry.station.coords.lat);
        case "lon":         enq.set_lon(cur->station_entry.station.coords.lon);
        case "yearmax":     if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.max.year);
        case "yearmin":     if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.min.year);
        case "monthmax":    if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.max.month);
        case "monthmin":    if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.min.month);
        case "daymax":      if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.max.day);
        case "daymin":      if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.min.day);
        case "hourmax":     if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.max.hour);
        case "hourmin":     if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.min.hour);
        case "minumax":     if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.max.minute);
        case "minumin":     if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.min.minute);
        case "secmax":      if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.max.second);
        case "secmin":      if (cur->var_entry.dtrange.is_missing()) return; else enq.set_int(cur->var_entry.dtrange.min.second);
        case "leveltype1":  enq.set_dballe_int(cur->var_entry.var.level.ltype1);
        case "l1":          enq.set_dballe_int(cur->var_entry.var.level.l1);
        case "leveltype2":  enq.set_dballe_int(cur->var_entry.var.level.ltype2);
        case "l2":          enq.set_dballe_int(cur->var_entry.var.level.l2);
        case "pindicator":  enq.set_dballe_int(cur->var_entry.var.trange.pind);
        case "p1":          enq.set_dballe_int(cur->var_entry.var.trange.p1);
        case "p2":          enq.set_dballe_int(cur->var_entry.var.trange.p2);
        case "var":         enq.set_var(cur->var_entry.var.varcode);
        case "context_id":  enq.set_int(cur->var_entry.count);
        case "count":       enq.set_int(cur->var_entry.count);
        default:            wreport::error_notfound::throwf("key %s not found on this query result", key);
    }
}

}
}
}
