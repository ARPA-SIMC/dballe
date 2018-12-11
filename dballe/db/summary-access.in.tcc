#include "summary.h"
#include "dballe/core/access-impl.h"
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

template<typename Station>
bool Cursor<Station>::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.maybe_set_int(get_station_id(cur->station_entry.station));
        case "mobile":      r.set(cur->station_entry.station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station_entry.station.coords.lat);
        case "lon":         r.set(cur->station_entry.station.coords.lon);
        case "yearmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.year);
        case "yearmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.year);
        case "monthmax":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.month);
        case "monthmin":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.month);
        case "daymax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.day);
        case "daymin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.day);
        case "hourmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.hour);
        case "hourmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.hour);
        case "minumax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.minute);
        case "minumin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.minute);
        case "secmax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.second);
        case "secmin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(cur->var_entry.var.level.ltype1);
        case "l1":          r.maybe_set_int(cur->var_entry.var.level.l1);
        case "leveltype2":  r.maybe_set_int(cur->var_entry.var.level.ltype2);
        case "l2":          r.maybe_set_int(cur->var_entry.var.level.l2);
        case "pindicator":  r.maybe_set_int(cur->var_entry.var.trange.pind);
        case "p1":          r.maybe_set_int(cur->var_entry.var.trange.p1);
        case "p2":          r.maybe_set_int(cur->var_entry.var.trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set((int)cur->var_entry.count);
        case "count":       r.set((int)cur->var_entry.count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

template<typename Station>
bool Cursor<Station>::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.maybe_set_int(get_station_id(cur->station_entry.station));
        case "mobile":      r.set(cur->station_entry.station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station_entry.station.coords.dlat());
        case "lon":         r.set(cur->station_entry.station.coords.dlon());
        case "yearmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.year);
        case "yearmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.year);
        case "monthmax":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.month);
        case "monthmin":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.month);
        case "daymax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.day);
        case "daymin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.day);
        case "hourmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.hour);
        case "hourmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.hour);
        case "minumax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.minute);
        case "minumin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.minute);
        case "secmax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.second);
        case "secmin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(cur->var_entry.var.level.ltype1);
        case "l1":          r.maybe_set_int(cur->var_entry.var.level.l1);
        case "leveltype2":  r.maybe_set_int(cur->var_entry.var.level.ltype2);
        case "l2":          r.maybe_set_int(cur->var_entry.var.level.l2);
        case "pindicator":  r.maybe_set_int(cur->var_entry.var.trange.pind);
        case "p1":          r.maybe_set_int(cur->var_entry.var.trange.p1);
        case "p2":          r.maybe_set_int(cur->var_entry.var.trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set((int)cur->var_entry.count);
        case "count":       r.set((int)cur->var_entry.count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

template<typename Station>
bool Cursor<Station>::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "rep_memo":    r.set(cur->station_entry.station.report);
        case "ana_id":      r.maybe_set_int(get_station_id(cur->station_entry.station));
        case "mobile":      r.set(cur->station_entry.station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station_entry.station.ident);
        case "lat":         r.set(cur->station_entry.station.coords.lat);
        case "lon":         r.set(cur->station_entry.station.coords.lon);
        case "yearmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.year);
        case "yearmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.year);
        case "monthmax":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.month);
        case "monthmin":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.month);
        case "daymax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.day);
        case "daymin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.day);
        case "hourmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.hour);
        case "hourmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.hour);
        case "minumax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.minute);
        case "minumin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.minute);
        case "secmax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.second);
        case "secmin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(cur->var_entry.var.level.ltype1);
        case "l1":          r.maybe_set_int(cur->var_entry.var.level.l1);
        case "leveltype2":  r.maybe_set_int(cur->var_entry.var.level.ltype2);
        case "l2":          r.maybe_set_int(cur->var_entry.var.level.l2);
        case "pindicator":  r.maybe_set_int(cur->var_entry.var.trange.pind);
        case "p1":          r.maybe_set_int(cur->var_entry.var.trange.p1);
        case "p2":          r.maybe_set_int(cur->var_entry.var.trange.p2);
        case "var":         r.set_varcode(cur->var_entry.var.varcode);
        case "context_id":  r.set((int)cur->var_entry.count);
        case "count":       r.set((int)cur->var_entry.count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

template<typename Station>
bool Cursor<Station>::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "rep_memo":    r.set(cur->station_entry.station.report);
        case "ana_id":      r.maybe_set_int(get_station_id(cur->station_entry.station));
        case "mobile":      r.set(cur->station_entry.station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station_entry.station.ident);
        case "lat":         r.set_latlon(cur->station_entry.station.coords.dlat());
        case "lon":         r.set_latlon(cur->station_entry.station.coords.dlon());
        case "yearmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.year);
        case "yearmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.year);
        case "monthmax":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.month);
        case "monthmin":    if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.month);
        case "daymax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.day);
        case "daymin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.day);
        case "hourmax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.hour);
        case "hourmin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.hour);
        case "minumax":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.minute);
        case "minumin":     if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.minute);
        case "secmax":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.max.second);
        case "secmin":      if (cur->var_entry.dtrange.is_missing()) r.found(); else r.set((int)cur->var_entry.dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(cur->var_entry.var.level.ltype1);
        case "l1":          r.maybe_set_int(cur->var_entry.var.level.l1);
        case "leveltype2":  r.maybe_set_int(cur->var_entry.var.level.ltype2);
        case "l2":          r.maybe_set_int(cur->var_entry.var.level.l2);
        case "pindicator":  r.maybe_set_int(cur->var_entry.var.trange.pind);
        case "p1":          r.maybe_set_int(cur->var_entry.var.trange.p1);
        case "p2":          r.maybe_set_int(cur->var_entry.var.trange.p2);
        case "var":         r.set_varcode(cur->var_entry.var.varcode);
        case "context_id":  r.set((int)cur->var_entry.count);
        case "count":       r.set((int)cur->var_entry.count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

}
}
}
