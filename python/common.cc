#include "common.h"
#include <Python.h>
#include <datetime.h>

using namespace wreport;

namespace dballe {
namespace python {

PyObject* format_varcode(wreport::Varcode code)
{
    char buf[7];
    snprintf(buf, 7, "%c%02d%03d",
            WR_VAR_F(code) == 0 ? 'B' :
            WR_VAR_F(code) == 1 ? 'R' :
            WR_VAR_F(code) == 2 ? 'C' :
            WR_VAR_F(code) == 3 ? 'D' : '?',
            WR_VAR_X(code), WR_VAR_Y(code));
    return PyString_FromString(buf);
}

PyObject* raise_wreport_exception(const wreport::error& e)
{
    switch (e.code())
    {
        case WR_ERR_NONE:
            PyErr_SetString(PyExc_SystemError, e.what());
            return NULL;
        case WR_ERR_NOTFOUND:    // Item not found
            PyErr_SetString(PyExc_KeyError, e.what());
            return NULL;
        case WR_ERR_TYPE:        // Wrong variable type
            PyErr_SetString(PyExc_TypeError, e.what());
            return NULL;
        case WR_ERR_ALLOC:       // Cannot allocate memory
            PyErr_SetString(PyExc_MemoryError, e.what());
            return NULL;
        case WR_ERR_ODBC:        // ODBC error
            PyErr_SetString(PyExc_OSError, e.what());
            return NULL;
        case WR_ERR_HANDLES:     // Handle management error
            PyErr_SetString(PyExc_SystemError, e.what());
            return NULL;
        case WR_ERR_TOOLONG:     // Buffer is too short to fit data
            PyErr_SetString(PyExc_ValueError, e.what());
            return NULL;
        case WR_ERR_SYSTEM:      // Error reported by the system
            PyErr_SetString(PyExc_OSError, e.what());
            return NULL;
        case WR_ERR_CONSISTENCY: // Consistency check failed
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        case WR_ERR_PARSE:       // Parse error
            PyErr_SetString(PyExc_ValueError, e.what());
            return NULL;
        case WR_ERR_WRITE:       // Write error
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        case WR_ERR_REGEX:       // Regular expression error
            PyErr_SetString(PyExc_ValueError, e.what());
            return NULL;
        case WR_ERR_UNIMPLEMENTED: // Feature not implemented
            PyErr_SetString(PyExc_NotImplementedError, e.what());
            return NULL;
        case WR_ERR_DOMAIN:      // Value outside acceptable domain
            PyErr_SetString(PyExc_OverflowError, e.what());
            return NULL;
        default:
            PyErr_Format(PyExc_SystemError, "unhandled exception with code %d: %s", e.code(), e.what());
            return NULL;
    }
}

PyObject* raise_std_exception(const std::exception& e)
{
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
}

PyObject* datetime_to_python(const Datetime& dt)
{
    if (dt.is_missing())
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return PyDateTime_FromDateAndTime(
            dt.year, dt.month,  dt.day,
            dt.hour, dt.minute, dt.second, 0);
}

int datetime_from_python(PyObject* dt, Datetime& out)
{
    if (dt == NULL || dt == Py_None)
    {
        out = Datetime();
        return 0;
    }

    if (!PyDateTime_Check(dt))
    {
        PyErr_SetString(PyExc_TypeError, "value must be an instance of datetime.datetime");
        return -1;
    }

    out = Datetime(
        PyDateTime_GET_YEAR((PyDateTime_DateTime*)dt),
        PyDateTime_GET_MONTH((PyDateTime_DateTime*)dt),
        PyDateTime_GET_DAY((PyDateTime_DateTime*)dt),
        PyDateTime_DATE_GET_HOUR((PyDateTime_DateTime*)dt),
        PyDateTime_DATE_GET_MINUTE((PyDateTime_DateTime*)dt),
        PyDateTime_DATE_GET_SECOND((PyDateTime_DateTime*)dt));
    return 0;
}

void common_init()
{
    /*
     * PyDateTimeAPI, that is used by all the PyDate* and PyTime* macros, is
     * defined as a static variable defaulting to NULL, and it needs to be
     * initialized on each and every C file where it is used.
     *
     * Therefore, we need to have a common_init() to call from all
     * initialization functions. *sigh*
     */
    if (PyDateTimeAPI)
        return;
    PyDateTime_IMPORT;
}

}
}
