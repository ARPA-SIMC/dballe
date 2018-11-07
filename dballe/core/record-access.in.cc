#include "record.h"
#include "var.h"
#include <cstring>

using namespace wreport;

namespace dballe {
namespace core {

int Record::_enqi(const char* key, unsigned len, bool& found) const
{
    found = true;
    switch (key) { // mklookup
        case "priority":    return priomin;
        case "priomax":     return priomax;
        case "priomin":     return priomin;
        case "rep_memo":    throw error_consistency("cannot enqi rep_memo");
        case "ana_id":      return station.id;
        case "mobile":      return mobile;
        case "ident":       throw error_consistency("cannot enqi ident");
        case "lat":         return station.coords.lat;
        case "lon":         return station.coords.lon;
        case "latmax":      return latrange.imax == LatRange::IMAX ? MISSING_INT : latrange.imax;
        case "latmin":      return latrange.imin == LatRange::IMIN ? MISSING_INT : latrange.imin;
        case "lonmax":      return lonrange.imax;
        case "lonmin":      return lonrange.imin;
        case "year":        return datetime.min.year == 0xffff ? MISSING_INT : datetime.min.year;
        case "month":       return datetime.min.month == 0xff ? MISSING_INT : datetime.min.month;
        case "day":         return datetime.min.day == 0xff ? MISSING_INT : datetime.min.day;
        case "hour":        return datetime.min.hour == 0xff ? MISSING_INT : datetime.min.hour;
        case "min":         return datetime.min.minute == 0xff ? MISSING_INT : datetime.min.minute;
        case "sec":         return datetime.min.second == 0xff ? MISSING_INT : datetime.min.second;
        case "yearmax":     return datetime.max.year == 0xffff ? MISSING_INT : datetime.max.year;
        case "yearmin":     return datetime.min.year == 0xffff ? MISSING_INT : datetime.min.year;
        case "monthmax":    return datetime.max.month == 0xff ? MISSING_INT : datetime.max.month;
        case "monthmin":    return datetime.min.month == 0xff ? MISSING_INT : datetime.min.month;
        case "daymax":      return datetime.max.day == 0xff ? MISSING_INT : datetime.max.day;
        case "daymin":      return datetime.min.day == 0xff ? MISSING_INT : datetime.min.day;
        case "hourmax":     return datetime.max.hour == 0xff ? MISSING_INT : datetime.max.hour;
        case "hourmin":     return datetime.min.hour == 0xff ? MISSING_INT : datetime.min.hour;
        case "minumax":     return datetime.max.minute == 0xff ? MISSING_INT : datetime.max.minute;
        case "minumin":     return datetime.min.minute == 0xff ? MISSING_INT : datetime.min.minute;
        case "secmax":      return datetime.max.second == 0xff ? MISSING_INT : datetime.max.second;
        case "secmin":      return datetime.min.second == 0xff ? MISSING_INT : datetime.min.second;
        case "leveltype1":  return level.ltype1;
        case "l1":          return level.l1;
        case "leveltype2":  return level.ltype2;
        case "l2":          return level.l2;
        case "pindicator":  return trange.pind;
        case "p1":          return trange.p1;
        case "p2":          return trange.p2;
        case "var":         throw error_consistency("cannot enqi var");
        case "varlist":     throw error_consistency("cannot enqi varlist");
        case "context_id":  return count;
        case "query":       throw error_consistency("cannot enqi query");
        case "ana_filter":  throw error_consistency("cannot enqi ana_filter");
        case "data_filter": throw error_consistency("cannot enqi data_filter");
        case "attr_filter": throw error_consistency("cannot enqi attr_filter");
        case "limit":       return count;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: found = false; return MISSING_INT;
    }
}

namespace {

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

}

double Record::_enqd(const char* key, unsigned len, bool& found, bool& missing) const
{
    MaybeDouble res(found, missing);
    switch (key) { // mklookup
        case "priority":    res.set_int(priomin);
        case "priomax":     res.set_int(priomax);
        case "priomin":     res.set_int(priomin);
        case "rep_memo":    throw error_consistency("cannot enqd rep_memo");
        case "ana_id":      res.set_int(station.id);
        case "mobile":      res.set_int(mobile);
        case "ident":       throw error_consistency("cannot enqd ident");
        case "lat":         res.found(); if (station.coords.lat != MISSING_INT) res.set(station.coords.dlat());
        case "lon":         res.found(); if (station.coords.lon != MISSING_INT) res.set(station.coords.dlon());
        case "latmax":      res.found(); if (latrange.imax != LatRange::IMAX) res.set(latrange.dmax());
        case "latmin":      res.found(); if (latrange.imin != LatRange::IMIN) res.set(latrange.dmin());
        case "lonmax":      res.found(); if (lonrange.imax != MISSING_INT) res.set(lonrange.dmax());
        case "lonmin":      res.found(); if (lonrange.imin != MISSING_INT) res.set(lonrange.dmin());
        case "year":        res.set_uint16(datetime.min.year);
        case "month":       res.set_uint8(datetime.min.month);
        case "day":         res.set_uint8(datetime.min.day);
        case "hour":        res.set_uint8(datetime.min.hour);
        case "min":         res.set_uint8(datetime.min.minute);
        case "sec":         res.set_uint8(datetime.min.second);
        case "yearmax":     res.set_uint16(datetime.max.year);
        case "yearmin":     res.set_uint16(datetime.min.year);
        case "monthmax":    res.set_uint8(datetime.max.month);
        case "monthmin":    res.set_uint8(datetime.min.month);
        case "daymax":      res.set_uint8(datetime.max.day);
        case "daymin":      res.set_uint8(datetime.min.day);
        case "hourmax":     res.set_uint8(datetime.max.hour);
        case "hourmin":     res.set_uint8(datetime.min.hour);
        case "minumax":     res.set_uint8(datetime.max.minute);
        case "minumin":     res.set_uint8(datetime.min.minute);
        case "secmax":      res.set_uint8(datetime.max.second);
        case "secmin":      res.set_uint8(datetime.min.second);
        case "leveltype1":  res.set_int(level.ltype1);
        case "l1":          res.set_int(level.l1);
        case "leveltype2":  res.set_int(level.ltype2);
        case "l2":          res.set_int(level.l2);
        case "pindicator":  res.set_int(trange.pind);
        case "p1":          res.set_int(trange.p1);
        case "p2":          res.set_int(trange.p2);
        case "var":         throw error_consistency("cannot enqd var");
        case "varlist":     throw error_consistency("cannot enqd varlist");
        case "context_id":  res.set_int(count);
        case "query":       throw error_consistency("cannot enqd query");
        case "ana_filter":  throw error_consistency("cannot enqd ana_filter");
        case "data_filter": throw error_consistency("cannot enqd data_filter");
        case "attr_filter": throw error_consistency("cannot enqd attr_filter");
        case "limit":       res.set_int(count);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default:            res.notfound();
    }
    return res.val;
}

std::string Record::_enqs(const char* key, unsigned len, bool& found, bool& missing) const
{
    MaybeString res(found, missing);
    switch (key) { // mklookup
        case "priority":    res.set_int(priomin);
        case "priomax":     res.set_int(priomax);
        case "priomin":     res.set_int(priomin);
        case "rep_memo":    res.set(station.report);
        case "ana_id":      res.set_int(station.id);
        case "mobile":      res.set_int(mobile);
        case "ident":       res.set(station.ident.get());
        case "lat":         res.set_int(station.coords.lat);
        case "lon":         res.set_int(station.coords.lon);
        case "latmax":      res.found(); if (latrange.imax != LatRange::IMAX) res.set(std::to_string(latrange.dmax()));
        case "latmin":      res.found(); if (latrange.imin != LatRange::IMIN) res.set(std::to_string(latrange.dmin()));
        case "lonmax":      res.set_int(lonrange.imax);
        case "lonmin":      res.set_int(lonrange.imin);
        case "year":        res.set_uint16(datetime.min.year);
        case "month":       res.set_uint8(datetime.min.month);
        case "day":         res.set_uint8(datetime.min.day);
        case "hour":        res.set_uint8(datetime.min.hour);
        case "min":         res.set_uint8(datetime.min.minute);
        case "sec":         res.set_uint8(datetime.min.second);
        case "yearmax":     res.set_uint16(datetime.max.year);
        case "yearmin":     res.set_uint16(datetime.min.year);
        case "monthmax":    res.set_uint8(datetime.max.month);
        case "monthmin":    res.set_uint8(datetime.min.month);
        case "daymax":      res.set_uint8(datetime.max.day);
        case "daymin":      res.set_uint8(datetime.min.day);
        case "hourmax":     res.set_uint8(datetime.max.hour);
        case "hourmin":     res.set_uint8(datetime.min.hour);
        case "minumax":     res.set_uint8(datetime.max.minute);
        case "minumin":     res.set_uint8(datetime.min.minute);
        case "secmax":      res.set_uint8(datetime.max.second);
        case "secmin":      res.set_uint8(datetime.min.second);
        case "leveltype1":  res.set_int(level.ltype1);
        case "l1":          res.set_int(level.l1);
        case "leveltype2":  res.set_int(level.ltype2);
        case "l2":          res.set_int(level.l2);
        case "pindicator":  res.set_int(trange.pind);
        case "p1":          res.set_int(trange.p1);
        case "p2":          res.set_int(trange.p2);
        case "var":         res.set(varcode_format(var));
        case "varlist":     res.set_varlist(varlist);
        case "context_id":  res.set_int(count);
        case "query":       res.set(query);
        case "ana_filter":  res.set(ana_filter);
        case "data_filter": res.set(data_filter);
        case "attr_filter": res.set(attr_filter);
        case "limit":       res.set_int(count);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default:            res.notfound();
    }
    return res.val;
}


bool Record::_seti(const char* key, unsigned len, int val)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = val;
        case "priomax":     priomax = val;
        case "priomin":     priomin = val;
        case "rep_memo":    throw error_consistency("cannot seti rep_memo");
        case "ana_id":      station.id = val;
        case "mobile":      mobile = val;
        case "ident":       throw error_consistency("cannot seti ident");
        case "lat":         station.coords.set_lat(val); latrange.set(val, val);
        case "lon":         station.coords.set_lon(val); lonrange.set(val, val);
        case "latmax":      latrange.imax = val == MISSING_INT ? LatRange::IMAX : val;
        case "latmin":      latrange.imin = val == MISSING_INT ? LatRange::IMIN : val;
        case "lonmax":      lonrange.imax = val;
        case "lonmin":      lonrange.imin = val;
        case "year":        datetime.min.year = datetime.max.year = val;
        case "month":       datetime.min.month = datetime.max.month = val;
        case "day":         datetime.min.day = datetime.max.day = val;
        case "hour":        datetime.min.hour = datetime.max.hour = val;
        case "min":         datetime.min.minute = datetime.max.minute = val;
        case "sec":         datetime.min.second = datetime.max.second = val;
        case "yearmax":     datetime.max.year = val;
        case "yearmin":     datetime.min.year = val;
        case "monthmax":    datetime.max.month = val;
        case "monthmin":    datetime.min.month = val;
        case "daymax":      datetime.max.day = val;
        case "daymin":      datetime.min.day = val;
        case "hourmax":     datetime.max.hour = val;
        case "hourmin":     datetime.min.hour = val;
        case "minumax":     datetime.max.minute = val;
        case "minumin":     datetime.min.minute = val;
        case "secmax":      datetime.max.second = val;
        case "secmin":      datetime.min.second = val;
        case "leveltype1":  level.ltype1 = val;
        case "l1":          level.l1 = val;
        case "leveltype2":  level.ltype2 = val;
        case "l2":          level.l2 = val;
        case "pindicator":  trange.pind = val;
        case "p1":          trange.p1 = val;
        case "p2":          trange.p2 = val;
        case "var":         throw error_consistency("cannot seti var");
        case "varlist":     throw error_consistency("cannot seti varlist");
        case "context_id":  count = val;
        case "query":       throw error_consistency("cannot seti query");
        case "ana_filter":  throw error_consistency("cannot seti ana_filter");
        case "data_filter": throw error_consistency("cannot seti data_filter");
        case "attr_filter": throw error_consistency("cannot seti attr_filter");
        case "limit":       count = val;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool Record::_setd(const char* key, unsigned len, double val)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = val;
        case "priomax":     priomax = val;
        case "priomin":     priomin = val;
        case "rep_memo":    throw error_consistency("cannot setd rep_memo");
        case "ana_id":      station.id = val;
        case "mobile":      mobile = val;
        case "ident":       throw error_consistency("cannot setd ident");
        case "lat":         station.coords.set_lat(val); latrange.set(val, val);
        case "lon":         station.coords.set_lon(val); lonrange.set(val, val);
        case "latmax":      latrange.imax = Coords::lat_to_int(val);
        case "latmin":      latrange.imin = Coords::lat_to_int(val);
        case "lonmax":      lonrange.imax = Coords::lon_to_int(val);
        case "lonmin":      lonrange.imin = Coords::lon_to_int(val);
        case "year":        datetime.min.year = datetime.max.year = val;
        case "month":       datetime.min.month = datetime.max.month = val;
        case "day":         datetime.min.day = datetime.max.day = val;
        case "hour":        datetime.min.hour = datetime.max.hour = val;
        case "min":         datetime.min.minute = datetime.max.minute = val;
        case "sec":         datetime.min.second = datetime.max.second = val;
        case "yearmax":     datetime.max.year = val;
        case "yearmin":     datetime.min.year = val;
        case "monthmax":    datetime.max.month = val;
        case "monthmin":    datetime.min.month = val;
        case "daymax":      datetime.max.day = val;
        case "daymin":      datetime.min.day = val;
        case "hourmax":     datetime.max.hour = val;
        case "hourmin":     datetime.min.hour = val;
        case "minumax":     datetime.max.minute = val;
        case "minumin":     datetime.min.minute = val;
        case "secmax":      datetime.max.second = val;
        case "secmin":      datetime.min.second = val;
        case "leveltype1":  level.ltype1 = val;
        case "l1":          level.l1 = val;
        case "leveltype2":  level.ltype2 = val;
        case "l2":          level.l2 = val;
        case "pindicator":  trange.pind = val;
        case "p1":          trange.p1 = val;
        case "p2":          trange.p2 = val;
        case "var":         throw error_consistency("cannot setd var");
        case "varlist":     throw error_consistency("cannot setd varlist");
        case "context_id":  count = val;
        case "query":       throw error_consistency("cannot setd query");
        case "ana_filter":  throw error_consistency("cannot setd ana_filter");
        case "data_filter": throw error_consistency("cannot setd data_filter");
        case "attr_filter": throw error_consistency("cannot setd attr_filter");
        case "limit":       count = val;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool Record::_setc(const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = strtol(val, nullptr, 10);
        case "priomax":     priomax = strtol(val, nullptr, 10);
        case "priomin":     priomin = strtol(val, nullptr, 10);
        case "rep_memo":    station.report = val;
        case "ana_id":      station.id = strtol(val, nullptr, 10);
        case "mobile":      mobile = strtol(val, nullptr, 10);
        case "ident":       station.ident = val;
        case "lat":         { int ival = strtol(val, nullptr, 10); station.coords.set_lat(ival); latrange.set(ival, ival); }
        case "lon":         { int ival = strtol(val, nullptr, 10); station.coords.set_lon(ival); lonrange.set(ival, ival); }
        case "latmax":      latrange.imax = strtol(val, nullptr, 10);
        case "latmin":      latrange.imin = strtol(val, nullptr, 10);
        case "lonmax":      lonrange.imax = strtol(val, nullptr, 10);
        case "lonmin":      lonrange.imin = strtol(val, nullptr, 10);
        case "year":        datetime.min.year = datetime.max.year = strtol(val, nullptr, 10);
        case "month":       datetime.min.month = datetime.max.month = strtol(val, nullptr, 10);
        case "day":         datetime.min.day = datetime.max.day = strtol(val, nullptr, 10);
        case "hour":        datetime.min.hour = datetime.max.hour = strtol(val, nullptr, 10);
        case "min":         datetime.min.minute = datetime.max.minute = strtol(val, nullptr, 10);
        case "sec":         datetime.min.second = datetime.max.second = strtol(val, nullptr, 10);
        case "yearmax":     datetime.max.year = strtol(val, nullptr, 10);
        case "yearmin":     datetime.min.year = strtol(val, nullptr, 10);
        case "monthmax":    datetime.max.month = strtol(val, nullptr, 10);
        case "monthmin":    datetime.min.month = strtol(val, nullptr, 10);
        case "daymax":      datetime.max.day = strtol(val, nullptr, 10);
        case "daymin":      datetime.min.day = strtol(val, nullptr, 10);
        case "hourmax":     datetime.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     datetime.min.hour = strtol(val, nullptr, 10);
        case "minumax":     datetime.max.minute = strtol(val, nullptr, 10);
        case "minumin":     datetime.min.minute = strtol(val, nullptr, 10);
        case "secmax":      datetime.max.second = strtol(val, nullptr, 10);
        case "secmin":      datetime.min.second = strtol(val, nullptr, 10);
        case "leveltype1":  level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          level.l1 = strtol(val, nullptr, 10);
        case "leveltype2":  level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          level.l2 = strtol(val, nullptr, 10);
        case "pindicator":  trange.pind = strtol(val, nullptr, 10);
        case "p1":          trange.p1 = strtol(val, nullptr, 10);
        case "p2":          trange.p2 = strtol(val, nullptr, 10);
        case "var":         var = resolve_varcode(val);
        case "varlist":     resolve_varlist(val, varlist);
        case "context_id":  count = strtol(val, nullptr, 10);
        case "query":       query = val;
        case "ana_filter":  ana_filter = val;
        case "data_filter": data_filter = val;
        case "attr_filter": attr_filter = val;
        case "limit":       count = strtol(val, nullptr, 10);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool Record::_sets(const char* key, unsigned len, const std::string& val)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = std::stoi(val);
        case "priomax":     priomax = std::stoi(val);
        case "priomin":     priomin = std::stoi(val);
        case "rep_memo":    station.report = val;
        case "ana_id":      station.id = std::stoi(val);
        case "mobile":      mobile = std::stoi(val);
        case "ident":       station.ident = val;
        case "lat":         { int ival = std::stoi(val); station.coords.set_lat(ival); latrange.set(ival, ival); }
        case "lon":         { int ival = std::stoi(val); station.coords.set_lon(ival); lonrange.set(ival, ival); }
        case "latmax":      latrange.imax = std::stoi(val);
        case "latmin":      latrange.imin = std::stoi(val);
        case "lonmax":      lonrange.imax = std::stoi(val);
        case "lonmin":      lonrange.imin = std::stoi(val);
        case "year":        datetime.min.year = datetime.max.year = std::stoi(val);
        case "month":       datetime.min.month = datetime.max.month = std::stoi(val);
        case "day":         datetime.min.day = datetime.max.day = std::stoi(val);
        case "hour":        datetime.min.hour = datetime.max.hour = std::stoi(val);
        case "min":         datetime.min.minute = datetime.max.minute = std::stoi(val);
        case "sec":         datetime.min.second = datetime.max.second = std::stoi(val);
        case "yearmax":     datetime.max.year = std::stoi(val);
        case "yearmin":     datetime.min.year = std::stoi(val);
        case "monthmax":    datetime.max.month = std::stoi(val);
        case "monthmin":    datetime.min.month = std::stoi(val);
        case "daymax":      datetime.max.day = std::stoi(val);
        case "daymin":      datetime.min.day = std::stoi(val);
        case "hourmax":     datetime.max.hour = std::stoi(val);
        case "hourmin":     datetime.min.hour = std::stoi(val);
        case "minumax":     datetime.max.minute = std::stoi(val);
        case "minumin":     datetime.min.minute = std::stoi(val);
        case "secmax":      datetime.max.second = std::stoi(val);
        case "secmin":      datetime.min.second = std::stoi(val);
        case "leveltype1":  level.ltype1 = std::stoi(val);
        case "l1":          level.l1 = std::stoi(val);
        case "leveltype2":  level.ltype2 = std::stoi(val);
        case "l2":          level.l2 = std::stoi(val);
        case "pindicator":  trange.pind = std::stoi(val);
        case "p1":          trange.p1 = std::stoi(val);
        case "p2":          trange.p2 = std::stoi(val);
        case "var":         var = resolve_varcode(val);
        case "varlist":     resolve_varlist(val, varlist);
        case "context_id":  count = std::stoi(val);
        case "query":       query = val;
        case "ana_filter":  ana_filter = val;
        case "data_filter": data_filter = val;
        case "attr_filter": attr_filter = val;
        case "limit":       count = std::stoi(val);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool Record::_setf(const char* key, unsigned len, const char* val)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = strtol(val, nullptr, 10);
        case "priomax":     priomax = strtol(val, nullptr, 10);
        case "priomin":     priomin = strtol(val, nullptr, 10);
        case "rep_memo":    station.report = val;
        case "ana_id":      station.id = strtol(val, nullptr, 10);
        case "mobile":      mobile = strtol(val, nullptr, 10);
        case "ident":       station.ident = val;
        case "lat":         { double dval = strtod(val, nullptr); station.coords.set_lat(dval); latrange.set(dval, dval); }
        case "lon":         { double dval = strtod(val, nullptr); station.coords.set_lon(dval); lonrange.set(dval, dval); }
        case "latmax":      latrange.imax = Coords::lat_to_int(strtod(val, nullptr));
        case "latmin":      latrange.imin = Coords::lat_to_int(strtod(val, nullptr));
        case "lonmax":      lonrange.imax = Coords::lon_to_int(strtod(val, nullptr));
        case "lonmin":      lonrange.imin = Coords::lon_to_int(strtod(val, nullptr));
        case "year":        datetime.min.year = datetime.max.year = strtol(val, nullptr, 10);
        case "month":       datetime.min.month = datetime.max.month = strtol(val, nullptr, 10);
        case "day":         datetime.min.day = datetime.max.day = strtol(val, nullptr, 10);
        case "hour":        datetime.min.hour = datetime.max.hour = strtol(val, nullptr, 10);
        case "min":         datetime.min.minute = datetime.max.minute = strtol(val, nullptr, 10);
        case "sec":         datetime.min.second = datetime.max.second = strtol(val, nullptr, 10);
        case "yearmax":     datetime.max.year = strtol(val, nullptr, 10);
        case "yearmin":     datetime.min.year = strtol(val, nullptr, 10);
        case "monthmax":    datetime.max.month = strtol(val, nullptr, 10);
        case "monthmin":    datetime.min.month = strtol(val, nullptr, 10);
        case "daymax":      datetime.max.day = strtol(val, nullptr, 10);
        case "daymin":      datetime.min.day = strtol(val, nullptr, 10);
        case "hourmax":     datetime.max.hour = strtol(val, nullptr, 10);
        case "hourmin":     datetime.min.hour = strtol(val, nullptr, 10);
        case "minumax":     datetime.max.minute = strtol(val, nullptr, 10);
        case "minumin":     datetime.min.minute = strtol(val, nullptr, 10);
        case "secmax":      datetime.max.second = strtol(val, nullptr, 10);
        case "secmin":      datetime.min.second = strtol(val, nullptr, 10);
        case "leveltype1":  level.ltype1 = strtol(val, nullptr, 10);
        case "l1":          level.l1 = strtol(val, nullptr, 10);
        case "leveltype2":  level.ltype2 = strtol(val, nullptr, 10);
        case "l2":          level.l2 = strtol(val, nullptr, 10);
        case "pindicator":  trange.pind = strtol(val, nullptr, 10);
        case "p1":          trange.p1 = strtol(val, nullptr, 10);
        case "p2":          trange.p2 = strtol(val, nullptr, 10);
        case "var":         var = resolve_varcode(val);
        case "varlist":     resolve_varlist(val, varlist);
        case "context_id":  count = strtol(val, nullptr, 10);
        case "query":       query = val;
        case "ana_filter":  ana_filter = val;
        case "data_filter": data_filter = val;
        case "attr_filter": attr_filter = val;
        case "limit":       count = strtol(val, nullptr, 10);
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}


bool Record::_unset(const char* key, unsigned len)
{
    switch (key) { // mklookup
        case "priority":    priomin = priomax = MISSING_INT;
        case "priomax":     priomax = MISSING_INT;
        case "priomin":     priomin = MISSING_INT;
        case "rep_memo":    station.report.clear();
        case "ana_id":      station.id = MISSING_INT;
        case "mobile":      mobile = MISSING_INT;
        case "ident":       station.ident.clear();
        case "lat":         station.coords = Coords(); latrange = LatRange();
        case "lon":         station.coords = Coords(); lonrange = LonRange();
        case "latmax":      latrange.imax = LatRange::IMAX;
        case "latmin":      latrange.imin = LatRange::IMIN;
        case "lonmax":      lonrange.imax = MISSING_INT;
        case "lonmin":      lonrange.imin = MISSING_INT;
        case "year":        datetime.min.year = datetime.max.year = 0xffff;
        case "month":       datetime.min.month = datetime.max.month = 0xff;
        case "day":         datetime.min.day = datetime.max.day = 0xff;
        case "hour":        datetime.min.hour = datetime.max.hour = 0xff;
        case "min":         datetime.min.minute = datetime.max.minute = 0xff;
        case "sec":         datetime.min.second = datetime.max.second = 0xff;
        case "yearmax":     datetime.max.year = 0xffff;
        case "yearmin":     datetime.min.year = 0xffff;
        case "monthmax":    datetime.max.month = 0xff;
        case "monthmin":    datetime.min.month = 0xff;
        case "daymax":      datetime.max.day = 0xff;
        case "daymin":      datetime.min.day = 0xff;
        case "hourmax":     datetime.max.hour = 0xff;
        case "hourmin":     datetime.min.hour = 0xff;
        case "minumax":     datetime.max.minute = 0xff;
        case "minumin":     datetime.min.minute = 0xff;
        case "secmax":      datetime.max.second = 0xff;
        case "secmin":      datetime.min.second = 0xff;
        case "leveltype1":  level.ltype1 = MISSING_INT;
        case "l1":          level.l1 = MISSING_INT;
        case "leveltype2":  level.ltype2 = MISSING_INT;
        case "l2":          level.l2 = MISSING_INT;
        case "pindicator":  trange.pind = MISSING_INT;
        case "p1":          trange.p1 = MISSING_INT;
        case "p2":          trange.p2 = MISSING_INT;
        case "var":         throw error_consistency("cannot seti var");
        case "varlist":     throw error_consistency("cannot seti varlist");
        case "context_id":  count = MISSING_INT;
        case "query":       throw error_consistency("cannot seti query");
        case "ana_filter":  throw error_consistency("cannot seti ana_filter");
        case "data_filter": throw error_consistency("cannot seti data_filter");
        case "attr_filter": throw error_consistency("cannot seti attr_filter");
        case "limit":       count = MISSING_INT;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}

bool Record::_isset(const char* key, unsigned len, bool& res) const
{
    switch (key) { // mklookup
        case "priority":    res = priomin != MISSING_INT;
        case "priomax":     res = priomax != MISSING_INT;
        case "priomin":     res = priomin != MISSING_INT;
        case "rep_memo":    res = !station.report.empty();
        case "ana_id":      res = station.id != MISSING_INT;
        case "mobile":      res = mobile != MISSING_INT;
        case "ident":       res = !station.ident.is_missing();
        case "lat":         res = station.coords.lat != MISSING_INT;
        case "lon":         res = station.coords.lon != MISSING_INT;
        case "latmax":      res = latrange.imax != LatRange::IMAX;
        case "latmin":      res = latrange.imin != LatRange::IMIN;
        case "lonmax":      res = lonrange.imax != MISSING_INT;
        case "lonmin":      res = lonrange.imin != MISSING_INT;
        case "year":        res = datetime.min.year != 0xffff;
        case "month":       res = datetime.min.month != 0xff;
        case "day":         res = datetime.min.day != 0xff;
        case "hour":        res = datetime.min.hour != 0xff;
        case "min":         res = datetime.min.minute != 0xff;
        case "sec":         res = datetime.min.second != 0xff;
        case "yearmax":     res = datetime.max.year != 0xffff;
        case "yearmin":     res = datetime.min.year != 0xffff;
        case "monthmax":    res = datetime.max.month != 0xff;
        case "monthmin":    res = datetime.min.month != 0xff;
        case "daymax":      res = datetime.max.day != 0xff;
        case "daymin":      res = datetime.min.day != 0xff;
        case "hourmax":     res = datetime.max.hour != 0xff;
        case "hourmin":     res = datetime.min.hour != 0xff;
        case "minumax":     res = datetime.max.minute != 0xff;
        case "minumin":     res = datetime.min.minute != 0xff;
        case "secmax":      res = datetime.max.second != 0xff;
        case "secmin":      res = datetime.min.second != 0xff;
        case "leveltype1":  res = level.ltype1 != MISSING_INT;
        case "l1":          res = level.l1 != MISSING_INT;
        case "leveltype2":  res = level.ltype2 != MISSING_INT;
        case "l2":          res = level.l2 != MISSING_INT;
        case "pindicator":  res = trange.pind != MISSING_INT;
        case "p1":          res = trange.p1 != MISSING_INT;
        case "p2":          res = trange.p2 != MISSING_INT;
        case "var":         res = var != 0;
        case "varlist":     res = !varlist.empty();
        case "context_id":  res = count != MISSING_INT;
        case "query":       res = !query.empty();
        case "ana_filter":  res = !ana_filter.empty();
        case "data_filter": res = !data_filter.empty();
        case "attr_filter": res = !attr_filter.empty();
        case "limit":       res = count != MISSING_INT;
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: return false;
    }
    return true;
}


}
}
