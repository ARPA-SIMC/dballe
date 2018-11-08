#include "record.h"
#include "record-access.h"
#include "var.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace core {

namespace {
bool _seti(core::Record& rec, const char* key, unsigned len, int val);
bool _setd(core::Record& rec, const char* key, unsigned len, double val);
bool _setc(core::Record& rec, const char* key, unsigned len, const char* val);
bool _sets(core::Record& rec, const char* key, unsigned len, const std::string& val);
bool _setf(core::Record& rec, const char* key, unsigned len, const char* val);
bool _unset(core::Record& rec, const char* key, unsigned len);
bool _isset(const core::Record& rec, const char* key, unsigned len, bool& res);
int _enqi(const core::Record& rec, const char* key, unsigned len, bool& found);
double _enqd(const core::Record& rec, const char* key, unsigned len, bool& found, bool& missing);
std::string _enqs(const core::Record& rec, const char* key, unsigned len, bool& found, bool& missing);
}


void record_unset(core::Record& rec, const char* name)
{
    if (_unset(rec, name, strlen(name)))
        return;
    rec.unset_var(resolve_varcode(name));
}

void record_seti(core::Record& rec, const char* key, int val)
{
    if (val == MISSING_INT)
    {
        record_unset(rec, key);
        return;
    }

    unsigned len = strlen(key);
    bool done = _seti(rec, key, len, val);
    if (!done)
        rec.obtain(resolve_varcode(key)).seti(val);
}

void record_setd(core::Record& rec, const char* key, double val)
{
    unsigned len = strlen(key);
    bool done = _setd(rec, key, len, val);
    if (!done)
        rec.obtain(resolve_varcode(key)).setd(val);
}

void record_setc(core::Record& rec, const char* key, const char* val)
{
    if (!val)
    {
        record_unset(rec, key);
        return;
    }

    unsigned len = strlen(key);
    bool done = _setc(rec, key, len, val);
    if (!done)
        rec.obtain(resolve_varcode(key)).setc(val);
}

void record_sets(core::Record& rec, const char* key, const std::string& val)
{
    unsigned len = strlen(key);
    bool done = _sets(rec, key, len, val);
    if (!done)
        rec.obtain(resolve_varcode(key)).sets(val);
}

void record_setf(core::Record& rec, const char* key, const char* val)
{
    if (!val || strcmp(val, "-") == 0)
    {
        record_unset(rec, key);
        return;
    }

    unsigned len = strlen(key);
    bool done = _setf(rec, key, len, val);
    if (!done)
        rec.obtain(resolve_varcode(key)).setf(val);
}

int record_enqi(const core::Record& rec, const char* key, int def)
{
    bool found;
    int res = _enqi(rec, key, strlen(key), found);
    if (found) return res == MISSING_INT ? def : res;

    const Var* var = rec.get_var(resolve_varcode(key));
    if (!var) return def;
    return var->enq(def);
}

double record_enqd(const core::Record& rec, const char* key, double def)
{
    bool found, missing;
    double res = _enqd(rec, key, strlen(key), found, missing);
    if (found) return missing ? def : res;

    const Var* var = rec.get_var(resolve_varcode(key));
    if (!var) return def;
    return var->enq(def);
}

bool record_enqdb(const core::Record& rec, const char* key, double& res)
{
    bool found, missing;
    double maybe_res = _enqd(rec, key, strlen(key), found, missing);
    if (found)
    {
        if (missing)
            return false;
        res = maybe_res;
        return true;
    }

    const Var* var = rec.get_var(resolve_varcode(key));
    if (!var) return false;
    if (!var->isset()) return false;
    res = var->enqd();
    return true;
}

std::string record_enqs(const core::Record& rec, const char* key, const std::string& def)
{
    bool found, missing;
    std::string res = _enqs(rec, key, strlen(key), found, missing);
    if (found) return missing ? def : res;

    const Var* var = rec.get_var(resolve_varcode(key));
    if (!var) return def;
    return var->enq(def);
}

bool record_enqsb(const core::Record& rec, const char* key, std::string& res)
{
    bool found, missing;
    std::string maybe_res = _enqs(rec, key, strlen(key), found, missing);
    if (found)
    {
        if (missing)
            return false;
        res = maybe_res;
        return true;
    }

    const Var* var = rec.get_var(resolve_varcode(key));
    if (!var) return false;
    if (!var->isset()) return false;
    res = var->enqs();
    return true;
}

bool record_isset(const core::Record& rec, const char* key)
{
    bool res;
    bool found = _isset(rec, key, strlen(key), res);
    if (found) return res;

    const Var* var = rec.get_var(resolve_varcode(key));
    if (!var) return false;
    if (!var->isset()) return false;
    return true;
}

namespace {

int _enqi(const core::Record& rec, const char* key, unsigned len, bool& found)
{
    found = true;
    switch (key) { // mklookup
        case "priority":    return rec.priomin;
        case "priomax":     return rec.priomax;
        case "priomin":     return rec.priomin;
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      return rec.station.id;
        case "mobile":      return rec.mobile;
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         return rec.station.coords.lat;
        case "lon":         return rec.station.coords.lon;
        case "latmax":      return rec.latrange.imax == LatRange::IMAX ? MISSING_INT : rec.latrange.imax;
        case "latmin":      return rec.latrange.imin == LatRange::IMIN ? MISSING_INT : rec.latrange.imin;
        case "lonmax":      return rec.lonrange.imax;
        case "lonmin":      return rec.lonrange.imin;
        case "year":        return rec.datetime.min.year == 0xffff ? MISSING_INT : rec.datetime.min.year;
        case "month":       return rec.datetime.min.month  == 0xff ? MISSING_INT : rec.datetime.min.month;
        case "day":         return rec.datetime.min.day    == 0xff ? MISSING_INT : rec.datetime.min.day;
        case "hour":        return rec.datetime.min.hour   == 0xff ? MISSING_INT : rec.datetime.min.hour;
        case "min":         return rec.datetime.min.minute == 0xff ? MISSING_INT : rec.datetime.min.minute;
        case "sec":         return rec.datetime.min.second == 0xff ? MISSING_INT : rec.datetime.min.second;
        case "yearmax":     return rec.datetime.max.year == 0xffff ? MISSING_INT : rec.datetime.max.year;
        case "yearmin":     return rec.datetime.min.year == 0xffff ? MISSING_INT : rec.datetime.min.year;
        case "monthmax":    return rec.datetime.max.month  == 0xff ? MISSING_INT : rec.datetime.max.month;
        case "monthmin":    return rec.datetime.min.month  == 0xff ? MISSING_INT : rec.datetime.min.month;
        case "daymax":      return rec.datetime.max.day    == 0xff ? MISSING_INT : rec.datetime.max.day;
        case "daymin":      return rec.datetime.min.day    == 0xff ? MISSING_INT : rec.datetime.min.day;
        case "hourmax":     return rec.datetime.max.hour   == 0xff ? MISSING_INT : rec.datetime.max.hour;
        case "hourmin":     return rec.datetime.min.hour   == 0xff ? MISSING_INT : rec.datetime.min.hour;
        case "minumax":     return rec.datetime.max.minute == 0xff ? MISSING_INT : rec.datetime.max.minute;
        case "minumin":     return rec.datetime.min.minute == 0xff ? MISSING_INT : rec.datetime.min.minute;
        case "secmax":      return rec.datetime.max.second == 0xff ? MISSING_INT : rec.datetime.max.second;
        case "secmin":      return rec.datetime.min.second == 0xff ? MISSING_INT : rec.datetime.min.second;
        case "leveltype1":  return rec.level.ltype1;
        case "l1":          return rec.level.l1;
        case "leveltype2":  return rec.level.ltype2;
        case "l2":          return rec.level.l2;
        case "pindicator":  return rec.trange.pind;
        case "p1":          return rec.trange.p1;
        case "p2":          return rec.trange.p2;
        case "var":         throw error_consistency("cannot enqi var");
        case "varlist":     throw error_consistency("cannot enqi varlist");
        case "context_id":  return rec.count;
        case "query":       throw error_consistency("cannot enqi query");
        case "ana_filter":  throw error_consistency("cannot enqi ana_filter");
        case "data_filter": throw error_consistency("cannot enqi data_filter");
        case "attr_filter": throw error_consistency("cannot enqi attr_filter");
        case "limit":       return rec.count;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: found = false; return MISSING_INT;
    }
}

struct Maybe
{
    bool& _found;
    bool& _missing;

    Maybe(bool& found, bool& missing)
        : _found(found), _missing(missing)
    {
        _missing = true;
        _found = false;
    }

    void found()
    {
        _found = true;
    }

    void notfound()
    {
        _found = false;
    }
};

struct MaybeDouble : public Maybe
{
    using Maybe::Maybe;
    double val;

    void set_int(int val)
    {
        _found = true;
        if (val != MISSING_INT)
        {
            _missing = false;
            this->val = val;
        }
    }

    void set_uint16(uint16_t val)
    {
        _found = true;
        if (val != 0xffff)
        {
            _missing = false;
            this->val = val;
        }
    }

    void set_uint8(uint8_t val)
    {
        _found = true;
        if (val != 0xff)
        {
            _missing = false;
            this->val = val;
        }
    }

    void set(double val)
    {
        _found = true;
        _missing = false;
        this->val = val;
    }
};

struct MaybeString : public Maybe
{
    using Maybe::Maybe;
    std::string val;

    void set_int(int val)
    {
        _found = true;
        if (val != MISSING_INT)
        {
            _missing = false;
            this->val = std::to_string(val);
        }
    }

    void set_uint16(uint16_t val)
    {
        _found = true;
        if (val != 0xffff)
        {
            _missing = false;
            this->val = std::to_string((int)val);
        }
    }

    void set_uint8(uint8_t val)
    {
        _found = true;
        if (val != 0xff)
        {
            _missing = false;
            this->val = std::to_string((int)val);
        }
    }

    void set(const char* val)
    {
        _found = true;
        if (!val)
        {
            _missing = true;
        } else {
            _missing = false;
            this->val = val;
        }
    }

    void set(const std::string& val)
    {
        _found = true;
        if (val.empty())
        {
            _missing = true;
        } else {
            _missing = false;
            this->val = val;
        }
    }

    void set_varlist(const std::set<wreport::Varcode>& val)
    {
        _found = true;
        if (val.empty())
        {
            _missing = true;
        } else {
            _missing = false;
            bool first = true;
            for (const auto& code: val)
            {
                if (first)
                    first = false;
                else
                    first += ",";
                this->val += varcode_format(code);
            }
        }
    }
};

double _enqd(const core::Record& rec, const char* key, unsigned len, bool& found, bool& missing)
{
    MaybeDouble res(found, missing);
    switch (key) { // mklookup
        case "priority":    res.set_int(rec.priomin);
        case "priomax":     res.set_int(rec.priomax);
        case "priomin":     res.set_int(rec.priomin);
        case "rep_memo":    throw error_consistency("cannot enqd rep_memo");
        case "ana_id":      res.set_int(rec.station.id);
        case "mobile":      res.set_int(rec.mobile);
        case "ident":       throw error_consistency("cannot enqd ident");
        case "lat":         res.found(); if (rec.station.coords.lat != MISSING_INT) res.set(rec.station.coords.dlat());
        case "lon":         res.found(); if (rec.station.coords.lon != MISSING_INT) res.set(rec.station.coords.dlon());
        case "latmax":      res.found(); if (rec.latrange.imax   != LatRange::IMAX) res.set(rec.latrange.dmax());
        case "latmin":      res.found(); if (rec.latrange.imin   != LatRange::IMIN) res.set(rec.latrange.dmin());
        case "lonmax":      res.found(); if (rec.lonrange.imax      != MISSING_INT) res.set(rec.lonrange.dmax());
        case "lonmin":      res.found(); if (rec.lonrange.imin      != MISSING_INT) res.set(rec.lonrange.dmin());
        case "year":        res.set_uint16(rec.datetime.min.year);
        case "month":       res.set_uint8(rec.datetime.min.month);
        case "day":         res.set_uint8(rec.datetime.min.day);
        case "hour":        res.set_uint8(rec.datetime.min.hour);
        case "min":         res.set_uint8(rec.datetime.min.minute);
        case "sec":         res.set_uint8(rec.datetime.min.second);
        case "yearmax":     res.set_uint16(rec.datetime.max.year);
        case "yearmin":     res.set_uint16(rec.datetime.min.year);
        case "monthmax":    res.set_uint8(rec.datetime.max.month);
        case "monthmin":    res.set_uint8(rec.datetime.min.month);
        case "daymax":      res.set_uint8(rec.datetime.max.day);
        case "daymin":      res.set_uint8(rec.datetime.min.day);
        case "hourmax":     res.set_uint8(rec.datetime.max.hour);
        case "hourmin":     res.set_uint8(rec.datetime.min.hour);
        case "minumax":     res.set_uint8(rec.datetime.max.minute);
        case "minumin":     res.set_uint8(rec.datetime.min.minute);
        case "secmax":      res.set_uint8(rec.datetime.max.second);
        case "secmin":      res.set_uint8(rec.datetime.min.second);
        case "leveltype1":  res.set_int(rec.level.ltype1);
        case "l1":          res.set_int(rec.level.l1);
        case "leveltype2":  res.set_int(rec.level.ltype2);
        case "l2":          res.set_int(rec.level.l2);
        case "pindicator":  res.set_int(rec.trange.pind);
        case "p1":          res.set_int(rec.trange.p1);
        case "p2":          res.set_int(rec.trange.p2);
        case "var":         throw error_consistency("cannot enqd var");
        case "varlist":     throw error_consistency("cannot enqd varlist");
        case "context_id":  res.set_int(rec.count);
        case "query":       throw error_consistency("cannot enqd query");
        case "ana_filter":  throw error_consistency("cannot enqd ana_filter");
        case "data_filter": throw error_consistency("cannot enqd data_filter");
        case "attr_filter": throw error_consistency("cannot enqd attr_filter");
        case "limit":       res.set_int(rec.count);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default:            res.notfound();
    }
    return res.val;
}

std::string _enqs(const core::Record& rec, const char* key, unsigned len, bool& found, bool& missing)
{
    MaybeString res(found, missing);
    switch (key) { // mklookup
        case "priority":    res.set_int(rec.priomin);
        case "priomax":     res.set_int(rec.priomax);
        case "priomin":     res.set_int(rec.priomin);
        case "rep_memo":    res.set(rec.station.report);
        case "ana_id":      res.set_int(rec.station.id);
        case "mobile":      res.set_int(rec.mobile);
        case "ident":       res.set(rec.station.ident.get());
        case "lat":         res.set_int(rec.station.coords.lat);
        case "lon":         res.set_int(rec.station.coords.lon);
        case "latmax":      res.found(); if (rec.latrange.imax != LatRange::IMAX) res.set(std::to_string(rec.latrange.dmax()));
        case "latmin":      res.found(); if (rec.latrange.imin != LatRange::IMIN) res.set(std::to_string(rec.latrange.dmin()));
        case "lonmax":      res.set_int(rec.lonrange.imax);
        case "lonmin":      res.set_int(rec.lonrange.imin);
        case "year":        res.set_uint16(rec.datetime.min.year);
        case "month":       res.set_uint8(rec.datetime.min.month);
        case "day":         res.set_uint8(rec.datetime.min.day);
        case "hour":        res.set_uint8(rec.datetime.min.hour);
        case "min":         res.set_uint8(rec.datetime.min.minute);
        case "sec":         res.set_uint8(rec.datetime.min.second);
        case "yearmax":     res.set_uint16(rec.datetime.max.year);
        case "yearmin":     res.set_uint16(rec.datetime.min.year);
        case "monthmax":    res.set_uint8(rec.datetime.max.month);
        case "monthmin":    res.set_uint8(rec.datetime.min.month);
        case "daymax":      res.set_uint8(rec.datetime.max.day);
        case "daymin":      res.set_uint8(rec.datetime.min.day);
        case "hourmax":     res.set_uint8(rec.datetime.max.hour);
        case "hourmin":     res.set_uint8(rec.datetime.min.hour);
        case "minumax":     res.set_uint8(rec.datetime.max.minute);
        case "minumin":     res.set_uint8(rec.datetime.min.minute);
        case "secmax":      res.set_uint8(rec.datetime.max.second);
        case "secmin":      res.set_uint8(rec.datetime.min.second);
        case "leveltype1":  res.set_int(rec.level.ltype1);
        case "l1":          res.set_int(rec.level.l1);
        case "leveltype2":  res.set_int(rec.level.ltype2);
        case "l2":          res.set_int(rec.level.l2);
        case "pindicator":  res.set_int(rec.trange.pind);
        case "p1":          res.set_int(rec.trange.p1);
        case "p2":          res.set_int(rec.trange.p2);
        case "var":         if (rec.var == 0) res.found(); else res.set(varcode_format(rec.var));
        case "varlist":     res.set_varlist(rec.varlist);
        case "context_id":  res.set_int(rec.count);
        case "query":       res.set(rec.query);
        case "ana_filter":  res.set(rec.ana_filter);
        case "data_filter": res.set(rec.data_filter);
        case "attr_filter": res.set(rec.attr_filter);
        case "limit":       res.set_int(rec.count);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default:            res.notfound();
    }
    return res.val;
}


bool _seti(core::Record& rec, const char* key, unsigned len, int val)
{
    switch (key) { // mklookup
        case "priority":    rec.priomin = rec.priomax = val;
        case "priomax":     rec.priomax = val;
        case "priomin":     rec.priomin = val;
        case "rep_memo":    throw error_consistency("cannot seti rep_memo");
        case "ana_id":      rec.station.id = val;
        case "mobile":      rec.mobile = val;
        case "ident":       throw error_consistency("cannot seti ident");
        case "lat":         rec.station.coords.set_lat(val); rec.latrange.set(val, val);
        case "lon":         rec.station.coords.set_lon(val); rec.lonrange.set(val, val);
        case "latmax":      rec.latrange.imax = val == MISSING_INT ? LatRange::IMAX : val;
        case "latmin":      rec.latrange.imin = val == MISSING_INT ? LatRange::IMIN : val;
        case "lonmax":      rec.lonrange.imax = val;
        case "lonmin":      rec.lonrange.imin = val;
        case "year":        rec.datetime.min.year   = rec.datetime.max.year = val;
        case "month":       rec.datetime.min.month  = rec.datetime.max.month = val;
        case "day":         rec.datetime.min.day    = rec.datetime.max.day = val;
        case "hour":        rec.datetime.min.hour   = rec.datetime.max.hour = val;
        case "min":         rec.datetime.min.minute = rec.datetime.max.minute = val;
        case "sec":         rec.datetime.min.second = rec.datetime.max.second = val;
        case "yearmax":     rec.datetime.max.year   = val;
        case "yearmin":     rec.datetime.min.year   = val;
        case "monthmax":    rec.datetime.max.month  = val;
        case "monthmin":    rec.datetime.min.month  = val;
        case "daymax":      rec.datetime.max.day    = val;
        case "daymin":      rec.datetime.min.day    = val;
        case "hourmax":     rec.datetime.max.hour   = val;
        case "hourmin":     rec.datetime.min.hour   = val;
        case "minumax":     rec.datetime.max.minute = val;
        case "minumin":     rec.datetime.min.minute = val;
        case "secmax":      rec.datetime.max.second = val;
        case "secmin":      rec.datetime.min.second = val;
        case "leveltype1":  rec.level.ltype1 = val;
        case "l1":          rec.level.l1 = val;
        case "leveltype2":  rec.level.ltype2 = val;
        case "l2":          rec.level.l2 = val;
        case "pindicator":  rec.trange.pind = val;
        case "p1":          rec.trange.p1 = val;
        case "p2":          rec.trange.p2 = val;
        case "var":         throw error_consistency("cannot seti var");
        case "varlist":     throw error_consistency("cannot seti varlist");
        case "context_id":  rec.count = val;
        case "query":       throw error_consistency("cannot seti query");
        case "ana_filter":  throw error_consistency("cannot seti ana_filter");
        case "data_filter": throw error_consistency("cannot seti data_filter");
        case "attr_filter": throw error_consistency("cannot seti attr_filter");
        case "limit":       rec.count = val;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool _setd(core::Record& rec, const char* key, unsigned len, double val)
{
    switch (key) { // mklookup
        case "priority":    rec.priomin = rec.priomax = val;
        case "priomax":     rec.priomax = val;
        case "priomin":     rec.priomin = val;
        case "rep_memo":    throw error_consistency("cannot setd rep_memo");
        case "ana_id":      rec.station.id = val;
        case "mobile":      rec.mobile = val;
        case "ident":       throw error_consistency("cannot setd ident");
        case "lat":         rec.station.coords.set_lat(val); rec.latrange.set(val, val);
        case "lon":         rec.station.coords.set_lon(val); rec.lonrange.set(val, val);
        case "latmax":      rec.latrange.imax = Coords::lat_to_int(val);
        case "latmin":      rec.latrange.imin = Coords::lat_to_int(val);
        case "lonmax":      rec.lonrange.imax = Coords::lon_to_int(val);
        case "lonmin":      rec.lonrange.imin = Coords::lon_to_int(val);
        case "year":        rec.datetime.min.year   = rec.datetime.max.year = val;
        case "month":       rec.datetime.min.month  = rec.datetime.max.month = val;
        case "day":         rec.datetime.min.day    = rec.datetime.max.day = val;
        case "hour":        rec.datetime.min.hour   = rec.datetime.max.hour = val;
        case "min":         rec.datetime.min.minute = rec.datetime.max.minute = val;
        case "sec":         rec.datetime.min.second = rec.datetime.max.second = val;
        case "yearmax":     rec.datetime.max.year = val;
        case "yearmin":     rec.datetime.min.year = val;
        case "monthmax":    rec.datetime.max.month = val;
        case "monthmin":    rec.datetime.min.month = val;
        case "daymax":      rec.datetime.max.day = val;
        case "daymin":      rec.datetime.min.day = val;
        case "hourmax":     rec.datetime.max.hour = val;
        case "hourmin":     rec.datetime.min.hour = val;
        case "minumax":     rec.datetime.max.minute = val;
        case "minumin":     rec.datetime.min.minute = val;
        case "secmax":      rec.datetime.max.second = val;
        case "secmin":      rec.datetime.min.second = val;
        case "leveltype1":  rec.level.ltype1 = val;
        case "l1":          rec.level.l1 = val;
        case "leveltype2":  rec.level.ltype2 = val;
        case "l2":          rec.level.l2 = val;
        case "pindicator":  rec.trange.pind = val;
        case "p1":          rec.trange.p1 = val;
        case "p2":          rec.trange.p2 = val;
        case "var":         throw error_consistency("cannot setd var");
        case "varlist":     throw error_consistency("cannot setd varlist");
        case "context_id":  rec.count = val;
        case "query":       throw error_consistency("cannot setd query");
        case "ana_filter":  throw error_consistency("cannot setd ana_filter");
        case "data_filter": throw error_consistency("cannot setd data_filter");
        case "attr_filter": throw error_consistency("cannot setd attr_filter");
        case "limit":       rec.count = val;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool _setc(core::Record& rec, const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    rec.priomin = rec.priomax = strtol(val, nullptr, 10);
        case "priomax":     rec.priomax = strtol(val, nullptr, 10);
        case "priomin":     rec.priomin = strtol(val, nullptr, 10);
        case "rep_memo":    rec.station.report = val;
        case "ana_id":      rec.station.id = strtol(val, nullptr, 10);
        case "mobile":      rec.mobile = strtol(val, nullptr, 10);
        case "ident":       rec.station.ident = val;
        case "lat":         { int ival = strtol(val, nullptr, 10); rec.station.coords.set_lat(ival); rec.latrange.set(ival, ival); }
        case "lon":         { int ival = strtol(val, nullptr, 10); rec.station.coords.set_lon(ival); rec.lonrange.set(ival, ival); }
        case "latmax":      rec.latrange.imax = strtol(val, nullptr, 10);
        case "latmin":      rec.latrange.imin = strtol(val, nullptr, 10);
        case "lonmax":      rec.lonrange.imax = strtol(val, nullptr, 10);
        case "lonmin":      rec.lonrange.imin = strtol(val, nullptr, 10);
        case "year":        rec.datetime.min.year   = rec.datetime.max.year = strtol(val, nullptr, 10);
        case "month":       rec.datetime.min.month  = rec.datetime.max.month = strtol(val, nullptr, 10);
        case "day":         rec.datetime.min.day    = rec.datetime.max.day = strtol(val, nullptr, 10);
        case "hour":        rec.datetime.min.hour   = rec.datetime.max.hour = strtol(val, nullptr, 10);
        case "min":         rec.datetime.min.minute = rec.datetime.max.minute = strtol(val, nullptr, 10);
        case "sec":         rec.datetime.min.second = rec.datetime.max.second = strtol(val, nullptr, 10);
        case "yearmax":     rec.datetime.max.year = strtol(val, nullptr, 10);
        case "yearmin":     rec.datetime.min.year = strtol(val, nullptr, 10);
        case "monthmax":    rec.datetime.max.month = strtol(val, nullptr, 10);
        case "monthmin":    rec.datetime.min.month = strtol(val, nullptr, 10);
        case "daymax":      rec.datetime.max.day = strtol(val, nullptr, 10);
        case "daymin":      rec.datetime.min.day = strtol(val, nullptr, 10);
        case "hourmax":     rec.datetime.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     rec.datetime.min.hour = strtol(val, nullptr, 10);
        case "minumax":     rec.datetime.max.minute = strtol(val, nullptr, 10);
        case "minumin":     rec.datetime.min.minute = strtol(val, nullptr, 10);
        case "secmax":      rec.datetime.max.second = strtol(val, nullptr, 10);
        case "secmin":      rec.datetime.min.second = strtol(val, nullptr, 10);
        case "leveltype1":  rec.level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          rec.level.l1 = strtol(val, nullptr, 10);
        case "leveltype2":  rec.level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          rec.level.l2 = strtol(val, nullptr, 10);
        case "pindicator":  rec.trange.pind = strtol(val, nullptr, 10);
        case "p1":          rec.trange.p1 = strtol(val, nullptr, 10);
        case "p2":          rec.trange.p2 = strtol(val, nullptr, 10);
        case "var":         rec.var = resolve_varcode(val);
        case "varlist":     resolve_varlist(val, rec.varlist);
        case "context_id":  rec.count = strtol(val, nullptr, 10);
        case "query":       rec.query = val;
        case "ana_filter":  rec.ana_filter = val;
        case "data_filter": rec.data_filter = val;
        case "attr_filter": rec.attr_filter = val;
        case "limit":       rec.count = strtol(val, nullptr, 10);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool _sets(core::Record& rec, const char* key, unsigned len, const std::string& val)
{
    switch (key) { // mklookup
        case "priority":    rec.priomin = rec.priomax = std::stoi(val);
        case "priomax":     rec.priomax = std::stoi(val);
        case "priomin":     rec.priomin = std::stoi(val);
        case "rep_memo":    rec.station.report = val;
        case "ana_id":      rec.station.id = std::stoi(val);
        case "mobile":      rec.mobile = std::stoi(val);
        case "ident":       rec.station.ident = val;
        case "lat":         { int ival = std::stoi(val); rec.station.coords.set_lat(ival); rec.latrange.set(ival, ival); }
        case "lon":         { int ival = std::stoi(val); rec.station.coords.set_lon(ival); rec.lonrange.set(ival, ival); }
        case "latmax":      rec.latrange.imax = std::stoi(val);
        case "latmin":      rec.latrange.imin = std::stoi(val);
        case "lonmax":      rec.lonrange.imax = std::stoi(val);
        case "lonmin":      rec.lonrange.imin = std::stoi(val);
        case "year":        rec.datetime.min.year   = rec.datetime.max.year = std::stoi(val);
        case "month":       rec.datetime.min.month  = rec.datetime.max.month = std::stoi(val);
        case "day":         rec.datetime.min.day    = rec.datetime.max.day = std::stoi(val);
        case "hour":        rec.datetime.min.hour   = rec.datetime.max.hour = std::stoi(val);
        case "min":         rec.datetime.min.minute = rec.datetime.max.minute = std::stoi(val);
        case "sec":         rec.datetime.min.second = rec.datetime.max.second = std::stoi(val);
        case "yearmax":     rec.datetime.max.year = std::stoi(val);
        case "yearmin":     rec.datetime.min.year = std::stoi(val);
        case "monthmax":    rec.datetime.max.month = std::stoi(val);
        case "monthmin":    rec.datetime.min.month = std::stoi(val);
        case "daymax":      rec.datetime.max.day = std::stoi(val);
        case "daymin":      rec.datetime.min.day = std::stoi(val);
        case "hourmax":     rec.datetime.max.hour = std::stoi(val);
        case "hourmin":     rec.datetime.min.hour = std::stoi(val);
        case "minumax":     rec.datetime.max.minute = std::stoi(val);
        case "minumin":     rec.datetime.min.minute = std::stoi(val);
        case "secmax":      rec.datetime.max.second = std::stoi(val);
        case "secmin":      rec.datetime.min.second = std::stoi(val);
        case "leveltype1":  rec.level.ltype1 = std::stoi(val);
        case "l1":          rec.level.l1 = std::stoi(val);
        case "leveltype2":  rec.level.ltype2 = std::stoi(val);
        case "l2":          rec.level.l2 = std::stoi(val);
        case "pindicator":  rec.trange.pind = std::stoi(val);
        case "p1":          rec.trange.p1 = std::stoi(val);
        case "p2":          rec.trange.p2 = std::stoi(val);
        case "var":         rec.var = resolve_varcode(val);
        case "varlist":     resolve_varlist(val, rec.varlist);
        case "context_id":  rec.count = std::stoi(val);
        case "query":       rec.query = val;
        case "ana_filter":  rec.ana_filter = val;
        case "data_filter": rec.data_filter = val;
        case "attr_filter": rec.attr_filter = val;
        case "limit":       rec.count = std::stoi(val);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool _setf(core::Record& rec, const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    rec.priomin = rec.priomax = strtol(val, nullptr, 10);
        case "priomax":     rec.priomax = strtol(val, nullptr, 10);
        case "priomin":     rec.priomin = strtol(val, nullptr, 10);
        case "rep_memo":    rec.station.report = val;
        case "ana_id":      rec.station.id = strtol(val, nullptr, 10);
        case "mobile":      rec.mobile = strtol(val, nullptr, 10);
        case "ident":       rec.station.ident = val;
        case "lat":         { double dval = strtod(val, nullptr); rec.station.coords.set_lat(dval); rec.latrange.set(dval, dval); }
        case "lon":         { double dval = strtod(val, nullptr); rec.station.coords.set_lon(dval); rec.lonrange.set(dval, dval); }
        case "latmax":      rec.latrange.imax = Coords::lat_to_int(strtod(val, nullptr));
        case "latmin":      rec.latrange.imin = Coords::lat_to_int(strtod(val, nullptr));
        case "lonmax":      rec.lonrange.imax = Coords::lon_to_int(strtod(val, nullptr));
        case "lonmin":      rec.lonrange.imin = Coords::lon_to_int(strtod(val, nullptr));
        case "year":        rec.datetime.min.year   = rec.datetime.max.year = strtol(val, nullptr, 10);
        case "month":       rec.datetime.min.month  = rec.datetime.max.month = strtol(val, nullptr, 10);
        case "day":         rec.datetime.min.day    = rec.datetime.max.day = strtol(val, nullptr, 10);
        case "hour":        rec.datetime.min.hour   = rec.datetime.max.hour = strtol(val, nullptr, 10);
        case "min":         rec.datetime.min.minute = rec.datetime.max.minute = strtol(val, nullptr, 10);
        case "sec":         rec.datetime.min.second = rec.datetime.max.second = strtol(val, nullptr, 10);
        case "yearmax":     rec.datetime.max.year = strtol(val, nullptr, 10);
        case "yearmin":     rec.datetime.min.year = strtol(val, nullptr, 10);
        case "monthmax":    rec.datetime.max.month = strtol(val, nullptr, 10);
        case "monthmin":    rec.datetime.min.month = strtol(val, nullptr, 10);
        case "daymax":      rec.datetime.max.day = strtol(val, nullptr, 10);
        case "daymin":      rec.datetime.min.day = strtol(val, nullptr, 10);
        case "hourmax":     rec.datetime.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     rec.datetime.min.hour = strtol(val, nullptr, 10);
        case "minumax":     rec.datetime.max.minute = strtol(val, nullptr, 10);
        case "minumin":     rec.datetime.min.minute = strtol(val, nullptr, 10);
        case "secmax":      rec.datetime.max.second = strtol(val, nullptr, 10);
        case "secmin":      rec.datetime.min.second = strtol(val, nullptr, 10);
        case "leveltype1":  rec.level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          rec.level.l1 = strtol(val, nullptr, 10);
        case "leveltype2":  rec.level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          rec.level.l2 = strtol(val, nullptr, 10);
        case "pindicator":  rec.trange.pind = strtol(val, nullptr, 10);
        case "p1":          rec.trange.p1 = strtol(val, nullptr, 10);
        case "p2":          rec.trange.p2 = strtol(val, nullptr, 10);
        case "var":         rec.var = resolve_varcode(val);
        case "varlist":     resolve_varlist(val, rec.varlist);
        case "context_id":  rec.count = strtol(val, nullptr, 10);
        case "query":       rec.query = val;
        case "ana_filter":  rec.ana_filter = val;
        case "data_filter": rec.data_filter = val;
        case "attr_filter": rec.attr_filter = val;
        case "limit":       rec.count = strtol(val, nullptr, 10);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool _unset(core::Record& rec, const char* key, unsigned len)
{
    switch (key) { // mklookup
        case "priority":    rec.priomin = rec.priomax = MISSING_INT;
        case "priomax":     rec.priomax = MISSING_INT;
        case "priomin":     rec.priomin = MISSING_INT;
        case "rep_memo":    rec.station.report.clear();
        case "ana_id":      rec.station.id = MISSING_INT;
        case "mobile":      rec.mobile = MISSING_INT;
        case "ident":       rec.station.ident.clear();
        case "lat":         rec.station.coords = Coords(); rec.latrange = LatRange();
        case "lon":         rec.station.coords = Coords(); rec.lonrange = LonRange();
        case "latmax":      rec.latrange.imax = LatRange::IMAX;
        case "latmin":      rec.latrange.imin = LatRange::IMIN;
        case "lonmax":      rec.lonrange.imax = MISSING_INT;
        case "lonmin":      rec.lonrange.imin = MISSING_INT;
        case "year":        rec.datetime.min.year   = rec.datetime.max.year = 0xffff;
        case "month":       rec.datetime.min.month  = rec.datetime.max.month = 0xff;
        case "day":         rec.datetime.min.day    = rec.datetime.max.day = 0xff;
        case "hour":        rec.datetime.min.hour   = rec.datetime.max.hour = 0xff;
        case "min":         rec.datetime.min.minute = rec.datetime.max.minute = 0xff;
        case "sec":         rec.datetime.min.second = rec.datetime.max.second = 0xff;
        case "yearmax":     rec.datetime.max.year = 0xffff;
        case "yearmin":     rec.datetime.min.year = 0xffff;
        case "monthmax":    rec.datetime.max.month = 0xff;
        case "monthmin":    rec.datetime.min.month = 0xff;
        case "daymax":      rec.datetime.max.day = 0xff;
        case "daymin":      rec.datetime.min.day = 0xff;
        case "hourmax":     rec.datetime.max.hour = 0xff;
        case "hourmin":     rec.datetime.min.hour = 0xff;
        case "minumax":     rec.datetime.max.minute = 0xff;
        case "minumin":     rec.datetime.min.minute = 0xff;
        case "secmax":      rec.datetime.max.second = 0xff;
        case "secmin":      rec.datetime.min.second = 0xff;
        case "leveltype1":  rec.level.ltype1 = MISSING_INT;
        case "l1":          rec.level.l1 = MISSING_INT;
        case "leveltype2":  rec.level.ltype2 = MISSING_INT;
        case "l2":          rec.level.l2 = MISSING_INT;
        case "pindicator":  rec.trange.pind = MISSING_INT;
        case "p1":          rec.trange.p1 = MISSING_INT;
        case "p2":          rec.trange.p2 = MISSING_INT;
        case "var":         rec.var = 0;
        case "varlist":     rec.varlist.clear();
        case "context_id":  rec.count = MISSING_INT;
        case "query":       rec.query.clear();
        case "ana_filter":  rec.ana_filter.clear();
        case "data_filter": rec.data_filter.clear();
        case "attr_filter": rec.attr_filter.clear();
        case "limit":       rec.count = MISSING_INT;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool _isset(const core::Record& rec, const char* key, unsigned len, bool& res)
{
    switch (key) { // mklookup
        case "priority":    res = rec.priomin != MISSING_INT;
        case "priomax":     res = rec.priomax != MISSING_INT;
        case "priomin":     res = rec.priomin != MISSING_INT;
        case "rep_memo":    res = !rec.station.report.empty();
        case "ana_id":      res = rec.station.id != MISSING_INT;
        case "mobile":      res = rec.mobile != MISSING_INT;
        case "ident":       res = !rec.station.ident.is_missing();
        case "lat":         res = rec.station.coords.lat != MISSING_INT;
        case "lon":         res = rec.station.coords.lon != MISSING_INT;
        case "latmax":      res = rec.latrange.imax != LatRange::IMAX;
        case "latmin":      res = rec.latrange.imin != LatRange::IMIN;
        case "lonmax":      res = rec.lonrange.imax != MISSING_INT;
        case "lonmin":      res = rec.lonrange.imin != MISSING_INT;
        case "year":        res = rec.datetime.min.year != 0xffff;
        case "month":       res = rec.datetime.min.month != 0xff;
        case "day":         res = rec.datetime.min.day != 0xff;
        case "hour":        res = rec.datetime.min.hour != 0xff;
        case "min":         res = rec.datetime.min.minute != 0xff;
        case "sec":         res = rec.datetime.min.second != 0xff;
        case "yearmax":     res = rec.datetime.max.year != 0xffff;
        case "yearmin":     res = rec.datetime.min.year != 0xffff;
        case "monthmax":    res = rec.datetime.max.month != 0xff;
        case "monthmin":    res = rec.datetime.min.month != 0xff;
        case "daymax":      res = rec.datetime.max.day != 0xff;
        case "daymin":      res = rec.datetime.min.day != 0xff;
        case "hourmax":     res = rec.datetime.max.hour != 0xff;
        case "hourmin":     res = rec.datetime.min.hour != 0xff;
        case "minumax":     res = rec.datetime.max.minute != 0xff;
        case "minumin":     res = rec.datetime.min.minute != 0xff;
        case "secmax":      res = rec.datetime.max.second != 0xff;
        case "secmin":      res = rec.datetime.min.second != 0xff;
        case "leveltype1":  res = rec.level.ltype1 != MISSING_INT;
        case "l1":          res = rec.level.l1 != MISSING_INT;
        case "leveltype2":  res = rec.level.ltype2 != MISSING_INT;
        case "l2":          res = rec.level.l2 != MISSING_INT;
        case "pindicator":  res = rec.trange.pind != MISSING_INT;
        case "p1":          res = rec.trange.p1 != MISSING_INT;
        case "p2":          res = rec.trange.p2 != MISSING_INT;
        case "var":         res = rec.var != 0;
        case "varlist":     res = !rec.varlist.empty();
        case "context_id":  res = rec.count != MISSING_INT;
        case "query":       res = !rec.query.empty();
        case "ana_filter":  res = !rec.ana_filter.empty();
        case "data_filter": res = !rec.data_filter.empty();
        case "attr_filter": res = !rec.attr_filter.empty();
        case "limit":       res = rec.count != MISSING_INT;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

}

}
}
