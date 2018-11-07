#include <dballe/core/record.h>
#include <dballe/core/var.h>
#include "common.h"
#include <Python.h>

namespace dballe {
namespace python {

namespace {

struct Result
{
    bool found = false;
    PyObject*& res;

    Result(PyObject*& res)
        : res(res)
    {
    }

    void add_none()
    {
        found = true;
        Py_INCREF(Py_None);
        res = Py_None;
    }

    void add(const std::string& val)
    {
        found = true;
        res = throw_ifnull(PyUnicode_FromStringAndSize(val.data(), val.size()));
    }

    void add(double val)
    {
        found = true;
        res = throw_ifnull(PyFloat_FromDouble(val));
    }

    void add(int val)
    {
        found = true;
        res = throw_ifnull(PyLong_FromLong(val));
    }

    void add_maybe_int(int val)
    {
        if (val == MISSING_INT)
            add_none();
        else
            add(val);
    }

    void add_maybe_bool(int val)
    {
        if (val == MISSING_INT)
            add_none();
        else {
            found = true;
            if (val)
            {
                Py_INCREF(Py_True);
                res = Py_True;
            } else {
                Py_INCREF(Py_False);
                res = Py_False;
            }
        }
    }

    void add_maybe_string(const std::string& val)
    {
        if (val.empty())
            add_none();
        else
            add(val);
    }

    void add_varcode(wreport::Varcode code)
    {
        found = true;
        char buf[8];
        format_bcode(code, buf);
        res = throw_ifnull(PyUnicode_FromStringAndSize(buf, 6));
    }

    void add_maybe_varlist(const std::set<wreport::Varcode>& varlist)
    {
        if (varlist.empty())
            add_none();
        else {
            std::string formatted;
            bool first = true;
            for (const auto& code: varlist)
            {
                if (first)
                    first = false;
                else
                    formatted += ",";
                char buf[8];
                format_bcode(code, buf);
                formatted += buf;
            }
            add(formatted);
        }
    }
};

}

bool _record_enqpython(const dballe::core::Record& rec, const char* key, unsigned len, PyObject*& result)
{
    Result res(result);
    switch (key) { // mklookup
        case "priority":    res.add_maybe_int(rec.priomin);
        case "priomax":     res.add_maybe_int(rec.priomax);
        case "priomin":     res.add_maybe_int(rec.priomin);
        case "rep_memo":    res.add_maybe_string(rec.station.report);
        case "ana_id":      res.add_maybe_int(rec.station.id);
        case "mobile":      res.add_maybe_bool(rec.mobile);
        case "ident":       if (rec.station.ident.is_missing()) res.add_none(); else res.add(rec.station.ident);
        case "lat":         if (rec.station.coords.lat == MISSING_INT) res.add_none(); else res.add(rec.station.coords.dlat());
        case "lon":         if (rec.station.coords.lon == MISSING_INT) res.add_none(); else res.add(rec.station.coords.dlon());
        case "latmax":      if (rec.latrange.imax == LatRange::IMAX) res.add_none(); else res.add(rec.latrange.dmax());
        case "latmin":      if (rec.latrange.imin == LatRange::IMIN) res.add_none(); else res.add(rec.latrange.dmin());
        case "lonmax":      if (rec.lonrange.imax == MISSING_INT) res.add_none(); else res.add(rec.lonrange.dmax());
        case "lonmin":      if (rec.lonrange.imin == MISSING_INT) res.add_none(); else res.add(rec.lonrange.dmin());
        case "year":        if (rec.datetime.min.year == 0xffff) res.add_none(); else res.add((int)rec.datetime.min.year);
        case "month":       if (rec.datetime.min.month == 0xff)  res.add_none(); else res.add((int)rec.datetime.min.month);
        case "day":         if (rec.datetime.min.day == 0xff)    res.add_none(); else res.add((int)rec.datetime.min.day);
        case "hour":        if (rec.datetime.min.hour == 0xff)   res.add_none(); else res.add((int)rec.datetime.min.hour);
        case "min":         if (rec.datetime.min.minute == 0xff) res.add_none(); else res.add((int)rec.datetime.min.minute);
        case "sec":         if (rec.datetime.min.second == 0xff) res.add_none(); else res.add((int)rec.datetime.min.second);
        case "yearmax":     if (rec.datetime.max.year == 0xffff) res.add_none(); else res.add((int)rec.datetime.max.year);
        case "yearmin":     if (rec.datetime.min.year == 0xffff) res.add_none(); else res.add((int)rec.datetime.min.year);
        case "monthmax":    if (rec.datetime.max.month == 0xff)  res.add_none(); else res.add((int)rec.datetime.max.month);
        case "monthmin":    if (rec.datetime.min.month == 0xff)  res.add_none(); else res.add((int)rec.datetime.min.month);
        case "daymax":      if (rec.datetime.max.day == 0xff)    res.add_none(); else res.add((int)rec.datetime.max.day);
        case "daymin":      if (rec.datetime.min.day == 0xff)    res.add_none(); else res.add((int)rec.datetime.min.day);
        case "hourmax":     if (rec.datetime.max.hour == 0xff)   res.add_none(); else res.add((int)rec.datetime.max.hour);
        case "hourmin":     if (rec.datetime.min.hour == 0xff)   res.add_none(); else res.add((int)rec.datetime.min.hour);
        case "minumax":     if (rec.datetime.max.minute == 0xff) res.add_none(); else res.add((int)rec.datetime.max.minute);
        case "minumin":     if (rec.datetime.min.minute == 0xff) res.add_none(); else res.add((int)rec.datetime.min.minute);
        case "secmax":      if (rec.datetime.max.second == 0xff) res.add_none(); else res.add((int)rec.datetime.max.second);
        case "secmin":      if (rec.datetime.min.second == 0xff) res.add_none(); else res.add((int)rec.datetime.min.second);
        case "leveltype1":  res.add_maybe_int(rec.level.ltype1);
        case "l1":          res.add_maybe_int(rec.level.l1);
        case "leveltype2":  res.add_maybe_int(rec.level.ltype2);
        case "l2":          res.add_maybe_int(rec.level.l2);
        case "pindicator":  res.add_maybe_int(rec.trange.pind);
        case "p1":          res.add_maybe_int(rec.trange.p1);
        case "p2":          res.add_maybe_int(rec.trange.p2);
        case "var":         if (rec.var == 0) res.add_none(); else res.add_varcode(rec.var);
        case "varlist":     res.add_maybe_varlist(rec.varlist);
        case "context_id":  res.add_maybe_int(rec.count);
        case "query":       res.add_maybe_string(rec.query);
        case "ana_filter":  res.add_maybe_string(rec.ana_filter);
        case "data_filter": res.add_maybe_string(rec.data_filter);
        case "attr_filter": res.add_maybe_string(rec.attr_filter);
        case "limit":       res.add_maybe_int(rec.count);
        // TODO: add the other python keys (date, time, datetime, level, trange, ...)
//        case "var_related": return DBA_KEY_VAR_RELATED;
        default: break;
    }
    return res.found;
}

}
}
