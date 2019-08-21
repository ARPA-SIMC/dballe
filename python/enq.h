#ifndef DBALLE_PYTHON_ENQ_H
#define DBALLE_PYTHON_ENQ_H

#include <dballe/core/enq.h>
#include "common.h"
#include "types.h"
#include "utils/wreport.h"

namespace dballe {
namespace python {

struct Enqs : public impl::Enq
{
    using Enq::Enq;
    std::string res;

    const char* name() const override { return "enqs"; }

    void set_bool(bool val) override
    {
        res = val ? "1" : "0";
        missing = false;
    }

    void set_int(int val) override
    {
        res = std::to_string(val);
        missing = false;
    }

    void set_dballe_int(int val) override
    {
        if (val == MISSING_INT)
            return;
        res = std::to_string(val);
        missing = false;
    }

    void set_string(const std::string& val) override
    {
        res = val;
        missing = false;
    }

    void set_ident(const Ident& ident) override
    {
        if (ident.is_missing())
            return;
        res = ident.get();
        missing = false;
    }

    void set_varcode(wreport::Varcode val) override
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = buf;
        missing = false;
    }

    void set_lat(int lat) override
    {
        if (lat == MISSING_INT)
            return;
        res = std::to_string(lat);
        missing = false;
    }

    void set_lon(int lon) override
    {
        if (lon == MISSING_INT)
            return;
        res = std::to_string(lon);
        missing = false;
    }

    void set_var_value(const wreport::Var& var) override
    {
        missing = false;
        res = var.enqs();
    }
};


struct Enqf : public impl::Enq
{
    using Enq::Enq;
    std::string res;

    const char* name() const override { return "enqf"; }

    void set_bool(bool val) override
    {
        res = val ? "1" : "0";
        missing = false;
    }

    void set_int(int val) override
    {
        res = std::to_string(val);
        missing = false;
    }

    void set_dballe_int(int val) override
    {
        if (val == MISSING_INT)
            return;
        res = std::to_string(val);
        missing = false;
    }

    void set_string(const std::string& val) override
    {
        res = val;
        missing = false;
    }

    void set_ident(const Ident& ident) override
    {
        if (ident.is_missing())
            return;
        res = ident.get();
        missing = false;
    }

    void set_varcode(wreport::Varcode val) override
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = buf;
        missing = false;
    }

    void set_lat(int lat) override
    {
        if (lat == MISSING_INT)
            return;
        char buf[15];
        snprintf(buf, 14, "%.5f", Coords::lat_from_int(lat));
        res = buf;
        missing = false;
    }

    void set_lon(int lon) override
    {
        if (lon == MISSING_INT)
            return;
        char buf[15];
        snprintf(buf, 14, "%.5f", Coords::lon_from_int(lon));
        res = buf;
        missing = false;
    }

    void set_var_value(const wreport::Var& var) override
    {
        missing = false;
        res = var.format();
    }
};


struct Enqpy : public impl::Enq
{
    using Enq::Enq;
    PyObject* res = nullptr;

    const char* name() const override { return "enqpy"; }

    void set_bool(bool val) override
    {
        res = val ? Py_True : Py_False;
        Py_INCREF(res);
        missing = false;
    }

    void set_int(int val) override
    {
        res = throw_ifnull(PyLong_FromLong(val));
        missing = false;
    }

    void set_dballe_int(int val) override
    {
        if (val == MISSING_INT)
        {
            res = Py_None;
            Py_INCREF(res);
        } else
            res = throw_ifnull(PyLong_FromLong(val));
        missing = false;
    }

    void set_string(const std::string& val) override
    {
        res = string_to_python(val);
        missing = false;
    }

    void set_ident(const Ident& ident) override
    {
        if (ident.is_missing())
        {
            res = Py_None;
            Py_INCREF(res);
        } else
            res = throw_ifnull(PyUnicode_FromString(ident.get()));
        missing = false;
    }

    void set_varcode(wreport::Varcode val) override
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = throw_ifnull(PyUnicode_FromStringAndSize(buf, 6));
        missing = false;
    }

    void set_var(const wreport::Var* val) override
    {
        if (!val) return;
        res = (PyObject*)throw_ifnull(wreport_api.var_create(*val));
        missing = false;
    }

    void set_attrs(const wreport::Var* val) override
    {
        if (!val) return;
        res = attrs_to_python(*val);
        missing = false;
    }

    void set_lat(int lat) override
    {
        if (lat == MISSING_INT)
            return;
        res = dballe_int_lat_to_python(lat);
        missing = false;
    }

    void set_lon(int lon) override
    {
        if (lon == MISSING_INT)
            return;
        res = dballe_int_lon_to_python(lon);
        missing = false;
    }

    void set_coords(const Coords& coords) override
    {
        res = coords_to_python(coords);
        missing = false;
    }

    void set_station(const Station& s) override
    {
        res = station_to_python(s);
        missing = false;
    }

    void set_dbstation(const DBStation& s) override
    {
        res = station_to_python(s);
        missing = false;
    }

    void set_datetime(const Datetime& dt) override
    {
        res = datetime_to_python(dt);
        missing = false;
    }

    void set_level(const Level& lev) override
    {
        res = level_to_python(lev);
        missing = false;
    }

    void set_trange(const Trange& tr) override
    {
        res = trange_to_python(tr);
        missing = false;
    }

    void set_var_value(const wreport::Var& var) override
    {
        missing = false;
        res = (PyObject*)throw_ifnull(wreport_api.var_create(var));
    }
};


}
}
#endif
