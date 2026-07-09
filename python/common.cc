#include "common.h"
#include "dballe/core/var.h"
#include "dballe/types.h"
#include "utils/values.h"
#include "utils/wreport.h"
#include <cerrno>
#include <string>

using namespace wreport;

namespace dballe {
namespace python {

Wreport wreport_api;

void set_wreport_exception(const wreport::error& e)
{
    switch (e.code())
    {
        case WR_ERR_NONE: PyErr_SetString(PyExc_SystemError, e.what()); break;
        case WR_ERR_NOTFOUND: // Item not found
            PyErr_SetString(PyExc_KeyError, e.what());
            break;
        case WR_ERR_TYPE: // Wrong variable type
            PyErr_SetString(PyExc_TypeError, e.what());
            break;
        case WR_ERR_ALLOC: // Cannot allocate memory
            PyErr_SetString(PyExc_MemoryError, e.what());
            break;
        case WR_ERR_ODBC: // Database error
            PyErr_SetString(PyExc_OSError, e.what());
            break;
        case WR_ERR_HANDLES: // Handle management error
            PyErr_SetString(PyExc_SystemError, e.what());
            break;
        case WR_ERR_TOOLONG: // Buffer is too short to fit data
            PyErr_SetString(PyExc_ValueError, e.what());
            break;
        case WR_ERR_SYSTEM: // Error reported by the system
            PyErr_SetString(PyExc_OSError, e.what());
            break;
        case WR_ERR_CONSISTENCY: // Consistency check failed
            PyErr_SetString(PyExc_RuntimeError, e.what());
            break;
        case WR_ERR_PARSE: // Parse error
            PyErr_SetString(PyExc_ValueError, e.what());
            break;
        case WR_ERR_WRITE: // Write error
            PyErr_SetString(PyExc_RuntimeError, e.what());
            break;
        case WR_ERR_REGEX: // Regular expression error
            PyErr_SetString(PyExc_ValueError, e.what());
            break;
        case WR_ERR_UNIMPLEMENTED: // Feature not implemented
            PyErr_SetString(PyExc_NotImplementedError, e.what());
            break;
        case WR_ERR_DOMAIN: // Value outside acceptable domain
            PyErr_SetString(PyExc_OverflowError, e.what());
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "unhandled exception with code %d: %s", e.code(),
                         e.what());
            break;
    }
}

void set_std_exception(const std::exception& e)
{
    PyErr_SetString(PyExc_RuntimeError, e.what());
}

FILE* check_file_result(FILE* f, const char* filename)
{
    if (!f)
    {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        throw PythonException();
    }
    return f;
}

PyObject* dballe_int_to_python(int val)
{
    if (val == MISSING_INT)
        Py_RETURN_NONE;
    return throw_ifnull(PyLong_FromLong(val));
}

int dballe_int_from_python(PyObject* o)
{
    if (o == NULL || o == Py_None)
        return MISSING_INT;

    int res = PyLong_AsLong(o);
    if (res == -1 && PyErr_Occurred())
        throw PythonException();

    return res;
}

std::string object_repr(PyObject* o)
{
    pyo_unique_ptr repr(throw_ifnull(PyObject_Repr(o)));
    return from_python<std::string>(repr);
}

void common_init() { wreport_api.import(); }

} // namespace python
} // namespace dballe
