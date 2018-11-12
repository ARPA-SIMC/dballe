#include "cursor.h"
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

bool Stations::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        default: break;
    }
    r.maybe_search_values(key, cur->values);
    return r.missing();
}

bool Stations::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.dlat());
        case "lon":         r.set(cur->station.coords.dlon());
        default: break;
    }
    r.maybe_search_values(key, cur->values);
    return r.missing();
}

bool Stations::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        default: break;
    }
    r.maybe_search_values(key, cur->values);
    return r.missing();
}

bool Stations::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set_latlon(cur->station.coords.dlat());
        case "lon":         r.set_latlon(cur->station.coords.dlon());
        default: break;
    }
    r.maybe_search_values(key, cur->values);
    return r.missing();
}


/*
 * StationData
 */

bool StationData::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}

bool StationData::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.dlat());
        case "lon":         r.set(cur->station.coords.dlon());
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}

bool StationData::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        case "var":         r.set_varcode(cur->value.code());
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}

bool StationData::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set_latlon(cur->station.coords.dlat());
        case "lon":         r.set_latlon(cur->station.coords.dlon());
        case "var":         r.set_varcode(cur->value.code());
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}


/*
 * Data
 */

bool Data::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        case "year":        r.set((int)cur->datetime.year);
        case "month":       r.set((int)cur->datetime.month);
        case "day":         r.set((int)cur->datetime.day);
        case "hour":        r.set((int)cur->datetime.hour);
        case "min":         r.set((int)cur->datetime.minute);
        case "sec":         r.set((int)cur->datetime.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}

bool Data::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.dlat());
        case "lon":         r.set(cur->station.coords.dlon());
        case "year":        r.set((int)cur->datetime.year);
        case "month":       r.set((int)cur->datetime.month);
        case "day":         r.set((int)cur->datetime.day);
        case "hour":        r.set((int)cur->datetime.hour);
        case "min":         r.set((int)cur->datetime.minute);
        case "sec":         r.set((int)cur->datetime.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}

bool Data::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        case "year":        r.set((int)cur->datetime.year);
        case "month":       r.set((int)cur->datetime.month);
        case "day":         r.set((int)cur->datetime.day);
        case "hour":        r.set((int)cur->datetime.hour);
        case "min":         r.set((int)cur->datetime.minute);
        case "sec":         r.set((int)cur->datetime.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         r.set_varcode(cur->value.code());
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}

bool Data::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set_latlon(cur->station.coords.dlat());
        case "lon":         r.set_latlon(cur->station.coords.dlon());
        case "year":        r.set((int)cur->datetime.year);
        case "month":       r.set((int)cur->datetime.month);
        case "day":         r.set((int)cur->datetime.day);
        case "hour":        r.set((int)cur->datetime.hour);
        case "min":         r.set((int)cur->datetime.minute);
        case "sec":         r.set((int)cur->datetime.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         r.set_varcode(cur->value.code());
        case "context_id":  r.set(cur->value.data_id);
        default: break;
    }
    r.maybe_search_value(key, cur->value);
    return r.missing();
}


/*
 * Summary
 */

bool Summary::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        case "yearmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.year);
        case "yearmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.year);
        case "monthmax":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.month);
        case "monthmin":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.month);
        case "daymax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.day);
        case "daymin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.day);
        case "hourmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.hour);
        case "hourmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.hour);
        case "minumax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.minute);
        case "minumin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.minute);
        case "secmax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.second);
        case "secmin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set((int)cur->count);
        case "count":       r.set((int)cur->count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

bool Summary::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(cur->station.coords.dlat());
        case "lon":         r.set(cur->station.coords.dlon());
        case "yearmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.year);
        case "yearmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.year);
        case "monthmax":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.month);
        case "monthmin":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.month);
        case "daymax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.day);
        case "daymin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.day);
        case "hourmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.hour);
        case "hourmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.hour);
        case "minumax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.minute);
        case "minumin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.minute);
        case "secmax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.second);
        case "secmin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.set((int)cur->count);
        case "count":       r.set((int)cur->count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

bool Summary::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set(cur->station.coords.lat);
        case "lon":         r.set(cur->station.coords.lon);
        case "yearmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.year);
        case "yearmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.year);
        case "monthmax":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.month);
        case "monthmin":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.month);
        case "daymax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.day);
        case "daymin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.day);
        case "hourmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.hour);
        case "hourmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.hour);
        case "minumax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.minute);
        case "minumin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.minute);
        case "secmax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.second);
        case "secmin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         r.set_varcode(cur->code);
        case "context_id":  r.set((int)cur->count);
        case "count":       r.set((int)cur->count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

bool Summary::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.set(tr->repinfo().get_priority(cur->station.report));
        case "rep_memo":    r.set(cur->station.report);
        case "ana_id":      r.set(cur->station.id);
        case "mobile":      r.set(cur->station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(cur->station.ident);
        case "lat":         r.set_latlon(cur->station.coords.dlat());
        case "lon":         r.set_latlon(cur->station.coords.dlon());
        case "yearmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.year);
        case "yearmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.year);
        case "monthmax":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.month);
        case "monthmin":    if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.month);
        case "daymax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.day);
        case "daymin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.day);
        case "hourmax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.hour);
        case "hourmin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.hour);
        case "minumax":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.minute);
        case "minumin":     if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.minute);
        case "secmax":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.max.second);
        case "secmin":      if (cur->dtrange.is_missing()) r.found(); else r.set((int)cur->dtrange.min.second);
        case "leveltype1":  r.maybe_set_int(get_levtr().level.ltype1);
        case "l1":          r.maybe_set_int(get_levtr().level.l1);
        case "leveltype2":  r.maybe_set_int(get_levtr().level.ltype2);
        case "l2":          r.maybe_set_int(get_levtr().level.l2);
        case "pindicator":  r.maybe_set_int(get_levtr().trange.pind);
        case "p1":          r.maybe_set_int(get_levtr().trange.p1);
        case "p2":          r.maybe_set_int(get_levtr().trange.p2);
        case "var":         r.set_varcode(cur->code);
        case "context_id":  r.set((int)cur->count);
        case "count":       r.set((int)cur->count);
        default: break;
    }
    r.throw_if_notfound(key);
    return r.missing();
}

}
}
}
}
