#include <Python.h>
#include <dballe/core/record.h>
#include <dballe/core/record-access.h>
#include <dballe/core/defs.h>
#include <dballe/core/query.h>
#include <dballe/core/data.h>
#include "record.h"
#include "common.h"
#include "types.h"
#include <vector>
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

namespace {

/**
 * Set key=val in rec.
 *
 * Returns 0 on success, -1 on failures with a python exception set
 */
static void setpy(dballe::core::Record& rec, PyObject* key, PyObject* val)
{
    string name = string_from_python(key);

    // Check for shortcut keys
    if (name == "datetime" || name == "date")
    {
        if (name == "date")
            if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] instead of rec[\"date\"]", 1))
                throw PythonException();

        if (val && PySequence_Check(val))
            rec.set(datetimerange_from_python(val));
        else
            rec.set(datetime_from_python(val));
        return;
    }

    if (name == "datemin") {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] = (min, max) instead of rec[\"datemin\"]", 1))
            throw PythonException();
        DatetimeRange dtr = core::Record::downcast(rec).get_datetimerange();
        dtr.min = datetime_from_python(val);
        rec.set(dtr);
        return;
    }

    if (name == "datemax") {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] = (min, max) instead of rec[\"datemax\"]", 1))
            throw PythonException();
        DatetimeRange dtr = core::Record::downcast(rec).get_datetimerange();
        dtr.max = datetime_from_python(val);
        rec.set(dtr);
        return;
    }

    if (name == "level")
    {
        rec.set(level_from_python(val));
        return;
    }

    if (name == "trange" || name == "timerange")
    {
        if (name == "timerange")
            if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"trange\"] instead of rec[\"timerange\"]", 1))
                throw PythonException();
        rec.set(trange_from_python(val));
        return;
    }

    if (!val)
    {
        // del rec[val]
        record_unset(rec, name.c_str());
        return;
    }

    // TODO: name is a string, we have the length, could it be useful?
    // Worth making a version of the enq/set functions that take std::string as key?
    if (PyFloat_Check(val))
    {
        double v = PyFloat_AsDouble(val);
        if (v == -1.0 && PyErr_Occurred())
            throw PythonException();
        record_setd(rec, name.c_str(), v);
    } else if (PyLong_Check(val)) {
        long v = PyLong_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            throw PythonException();
        record_seti(rec, name.c_str(), (int)v);
    } else if (PyUnicode_Check(val) || PyBytes_Check(val)) {
        string value = string_from_python(val);
        record_sets(rec, name.c_str(), value);
    } else if (val == Py_None) {
        record_unset(rec, name.c_str());
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
        throw PythonException();
    }
}

static void setpy(dballe::core::Values& values, PyObject* key, PyObject* val)
{
    wreport::Varcode code = varcode_from_python(key);

    if (!val)
    {
        // del rec[val]
        values.unset(code);
        return;
    }

    if (PyFloat_Check(val))
    {
        double v = PyFloat_AsDouble(val);
        if (v == -1.0 && PyErr_Occurred())
            throw PythonException();
        values.set(code, v);
    } else if (PyLong_Check(val)) {
        long v = PyLong_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            throw PythonException();
        values.set(code, (int)v);
    } else if (PyUnicode_Check(val) || PyBytes_Check(val)) {
        values.set(code, string_from_python(val));
    } else if (val == Py_None) {
        values.unset(code);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
        throw PythonException();
    }
}

}

namespace dballe {
namespace python {

void read_query(PyObject* from_python, dballe::core::Query& query)
{
    if (!from_python || from_python == Py_None)
    {
        query.clear();
        return;
    }

    if (PyDict_Check(from_python))
    {
        dballe::core::Record rec;
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
            setpy(rec, key, value);
        query.set_from_record(rec);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

void read_data(PyObject* from_python, dballe::core::Data& data)
{
    if (!from_python || from_python == Py_None)
    {
        data.clear();
        return;
    }

    if (PyDict_Check(from_python))
    {
        dballe::core::Record rec;
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
            setpy(rec, key, value);
        rec.to_data(data);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

void read_values(PyObject* from_python, dballe::core::Values& values)
{
    if (!from_python || from_python == Py_None)
    {
        values.clear();
        return;
    }

    if (PyDict_Check(from_python))
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
            setpy(values, key, value);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

void set_var(PyObject* dict, const wreport::Var& var)
{
    char bcode[7];
    format_bcode(var.code(), bcode);
    pyo_unique_ptr pyvar((PyObject*)throw_ifnull(wrpy->var_create_copy(var)));
    if (PyDict_SetItemString(dict, bcode, pyvar))
        throw PythonException();
}

}
}
