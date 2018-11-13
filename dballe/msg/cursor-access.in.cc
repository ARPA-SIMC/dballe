#include "cursor.h"
#include "dballe/core/access-impl.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace msg {

/*
 * Stations
 */

bool CursorStation::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(station.coords.lat);
        case "lon":         r.set(station.coords.lon);
        default: break;
    }
    r.maybe_search_values(key, station_ctx);
    return !r.missing();
}

bool CursorStation::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(station.coords.dlat());
        case "lon":         r.set(station.coords.dlon());
        default: break;
    }
    r.maybe_search_values(key, station_ctx);
    return !r.missing();
}

bool CursorStation::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    r.set(station.report);
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(station.ident);
        case "lat":         r.set(station.coords.lat);
        case "lon":         r.set(station.coords.lon);
        default: break;
    }
    r.maybe_search_values(key, station_ctx);
    return !r.missing();
}

bool CursorStation::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    r.set(station.report);
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(station.ident);
        case "lat":         r.set_latlon(station.coords.dlat());
        case "lon":         r.set_latlon(station.coords.dlon());
        default: break;
    }
    r.maybe_search_values(key, station_ctx);
    return !r.missing();
}


/*
 * StationData
 */

bool CursorStationData::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(station.coords.lat);
        case "lon":         r.set(station.coords.lon);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *cur);
    return !r.missing();
}

bool CursorStationData::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(station.coords.dlat());
        case "lon":         r.set(station.coords.dlon());
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *cur);
    return !r.missing();
}

bool CursorStationData::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    r.set(station.report);
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(station.ident);
        case "lat":         r.set(station.coords.lat);
        case "lon":         r.set(station.coords.lon);
        case "var":         r.set_varcode((*cur)->code());
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *cur);
    return !r.missing();
}

bool CursorStationData::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    r.set(station.report);
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(station.ident);
        case "lat":         r.set_latlon(station.coords.dlat());
        case "lon":         r.set_latlon(station.coords.dlon());
        case "var":         r.set_varcode((*cur)->code());
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *cur);
    return !r.missing();
}


/*
 * Data
 */

bool CursorData::enqi(const char* key, unsigned len, int& res) const
{
    Maybe<Int> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(station.coords.lat);
        case "lon":         r.set(station.coords.lon);
        case "year":        r.set((int)datetime.year);
        case "month":       r.set((int)datetime.month);
        case "day":         r.set((int)datetime.day);
        case "hour":        r.set((int)datetime.hour);
        case "min":         r.set((int)datetime.minute);
        case "sec":         r.set((int)datetime.second);
        case "leveltype1":  r.maybe_set_int(cur->ctx->level.ltype1);
        case "l1":          r.maybe_set_int(cur->ctx->level.l1);
        case "leveltype2":  r.maybe_set_int(cur->ctx->level.ltype2);
        case "l2":          r.maybe_set_int(cur->ctx->level.l2);
        case "pindicator":  r.maybe_set_int(cur->ctx->trange.pind);
        case "p1":          r.maybe_set_int(cur->ctx->trange.p1);
        case "p2":          r.maybe_set_int(cur->ctx->trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *(cur->var));
    return !r.missing();
}

bool CursorData::enqd(const char* key, unsigned len, double& res) const
{
    Maybe<Double> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         r.set(station.coords.dlat());
        case "lon":         r.set(station.coords.dlon());
        case "year":        r.set((int)datetime.year);
        case "month":       r.set((int)datetime.month);
        case "day":         r.set((int)datetime.day);
        case "hour":        r.set((int)datetime.hour);
        case "min":         r.set((int)datetime.minute);
        case "sec":         r.set((int)datetime.second);
        case "leveltype1":  r.maybe_set_int(cur->ctx->level.ltype1);
        case "l1":          r.maybe_set_int(cur->ctx->level.l1);
        case "leveltype2":  r.maybe_set_int(cur->ctx->level.ltype2);
        case "l2":          r.maybe_set_int(cur->ctx->level.l2);
        case "pindicator":  r.maybe_set_int(cur->ctx->trange.pind);
        case "p1":          r.maybe_set_int(cur->ctx->trange.p1);
        case "p2":          r.maybe_set_int(cur->ctx->trange.p2);
        case "var":         throw error_consistency("cannot enqi var");
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *(cur->var));
    return !r.missing();
}

bool CursorData::enqs(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    r.set(station.report);
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(station.ident);
        case "lat":         r.set(station.coords.lat);
        case "lon":         r.set(station.coords.lon);
        case "year":        r.set((int)datetime.year);
        case "month":       r.set((int)datetime.month);
        case "day":         r.set((int)datetime.day);
        case "hour":        r.set((int)datetime.hour);
        case "min":         r.set((int)datetime.minute);
        case "sec":         r.set((int)datetime.second);
        case "leveltype1":  r.maybe_set_int(cur->ctx->level.ltype1);
        case "l1":          r.maybe_set_int(cur->ctx->level.l1);
        case "leveltype2":  r.maybe_set_int(cur->ctx->level.ltype2);
        case "l2":          r.maybe_set_int(cur->ctx->level.l2);
        case "pindicator":  r.maybe_set_int(cur->ctx->trange.pind);
        case "p1":          r.maybe_set_int(cur->ctx->trange.p1);
        case "p2":          r.maybe_set_int(cur->ctx->trange.p2);
        case "var":         r.set_varcode((*(cur->var))->code());
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *(cur->var));
    return !r.missing();
}

bool CursorData::enqf(const char* key, unsigned len, std::string& res) const
{
    Maybe<String> r(res);
    switch (key) { // mklookup
        case "priority":    r.found();
        case "rep_memo":    r.set(station.report);
        case "ana_id":      r.set(station.id);
        case "mobile":      r.set(station.ident.is_missing() ? 0 : 1);
        case "ident":       r.set_ident(station.ident);
        case "lat":         r.set_latlon(station.coords.dlat());
        case "lon":         r.set_latlon(station.coords.dlon());
        case "year":        r.set((int)datetime.year);
        case "month":       r.set((int)datetime.month);
        case "day":         r.set((int)datetime.day);
        case "hour":        r.set((int)datetime.hour);
        case "min":         r.set((int)datetime.minute);
        case "sec":         r.set((int)datetime.second);
        case "leveltype1":  r.maybe_set_int(cur->ctx->level.ltype1);
        case "l1":          r.maybe_set_int(cur->ctx->level.l1);
        case "leveltype2":  r.maybe_set_int(cur->ctx->level.ltype2);
        case "l2":          r.maybe_set_int(cur->ctx->level.l2);
        case "pindicator":  r.maybe_set_int(cur->ctx->trange.pind);
        case "p1":          r.maybe_set_int(cur->ctx->trange.p1);
        case "p2":          r.maybe_set_int(cur->ctx->trange.p2);
        case "var":         r.set_varcode((*(cur->var))->code());
        case "context_id":  r.found();
        default: break;
    }
    r.maybe_search_value(key, *(cur->var));
    return !r.missing();
}

}
}
