#include "common.h"
#include <Python.h>
#include "dballe/types.h"
#include "dballe/core/var.h"
#include "config.h"

#if PY_MAJOR_VERSION <= 2
    #define PyLong_AsLong PyInt_AsLong
    #define PyLong_Type PyInt_Type
#endif

using namespace wreport;

namespace dballe {
namespace python {

wrpy_c_api* wrpy = 0;

void set_wreport_exception(const wreport::error& e)
{
    switch (e.code())
    {
        case WR_ERR_NONE:
            PyErr_SetString(PyExc_SystemError, e.what());
            break;
        case WR_ERR_NOTFOUND:    // Item not found
            PyErr_SetString(PyExc_KeyError, e.what());
            break;
        case WR_ERR_TYPE:        // Wrong variable type
            PyErr_SetString(PyExc_TypeError, e.what());
            break;
        case WR_ERR_ALLOC:       // Cannot allocate memory
            PyErr_SetString(PyExc_MemoryError, e.what());
            break;
        case WR_ERR_ODBC:        // Database error
            PyErr_SetString(PyExc_OSError, e.what());
            break;
        case WR_ERR_HANDLES:     // Handle management error
            PyErr_SetString(PyExc_SystemError, e.what());
            break;
        case WR_ERR_TOOLONG:     // Buffer is too short to fit data
            PyErr_SetString(PyExc_ValueError, e.what());
            break;
        case WR_ERR_SYSTEM:      // Error reported by the system
            PyErr_SetString(PyExc_OSError, e.what());
            break;
        case WR_ERR_CONSISTENCY: // Consistency check failed
            PyErr_SetString(PyExc_RuntimeError, e.what());
            break;
        case WR_ERR_PARSE:       // Parse error
            PyErr_SetString(PyExc_ValueError, e.what());
            break;
        case WR_ERR_WRITE:       // Write error
            PyErr_SetString(PyExc_RuntimeError, e.what());
            break;
        case WR_ERR_REGEX:       // Regular expression error
            PyErr_SetString(PyExc_ValueError, e.what());
            break;
        case WR_ERR_UNIMPLEMENTED: // Feature not implemented
            PyErr_SetString(PyExc_NotImplementedError, e.what());
            break;
        case WR_ERR_DOMAIN:      // Value outside acceptable domain
            PyErr_SetString(PyExc_OverflowError, e.what());
            break;
        default:
            PyErr_Format(PyExc_SystemError, "unhandled exception with code %d: %s", e.code(), e.what());
            break;
    }
}

void set_std_exception(const std::exception& e)
{
    PyErr_SetString(PyExc_RuntimeError, e.what());
}

PyObject* string_to_python(const std::string& str)
{
    return PyUnicode_FromStringAndSize(str.data(), str.size());
}

bool pyobject_is_string(PyObject* o)
{
#if PY_MAJOR_VERSION >= 3
    if (PyBytes_Check(o))
        return true;
#else
    if (PyString_Check(o))
        return true;
#endif
    if (PyUnicode_Check(o))
        return true;
    return false;
}

std::string string_from_python(PyObject* o)
{
#if PY_MAJOR_VERSION >= 3
    if (PyBytes_Check(o)) {
        const char* v = PyBytes_AsString(o);
        if (!v) throw PythonException();
        return v;
    }
#else
    if (PyString_Check(o)) {
        const char* v = PyString_AsString(o);
        if (!v) throw PythonException();
        return v;
    }
#endif
    if (PyUnicode_Check(o)) {
#if PY_MAJOR_VERSION >= 3
        const char* v = PyUnicode_AsUTF8(o);
        if (!v) throw PythonException();
        return v;
#else
        pyo_unique_ptr utf8(PyUnicode_AsUTF8String(o));
        const char* v = PyString_AsString(utf8);
        if (!v) throw PythonException();
        return v;
#endif
    }
    PyErr_SetString(PyExc_TypeError, "value must be an instance of str, bytes or unicode");
    throw PythonException();
}

double double_from_python(PyObject* o)
{
    double res = PyFloat_AsDouble(o);
    if (res == -1.0 && PyErr_Occurred())
        throw PythonException();
    return res;
}

int file_get_fileno(PyObject* o)
{
    // fileno_value = obj.fileno()
    pyo_unique_ptr fileno_meth(PyObject_GetAttrString(o, "fileno"));
    if (!fileno_meth) return -1;
    pyo_unique_ptr fileno_args(Py_BuildValue("()"));
    if (!fileno_args) return -1;
    PyObject* fileno_value = PyObject_Call(fileno_meth, fileno_args, NULL);
    if (!fileno_value)
    {
        if (PyErr_ExceptionMatches(PyExc_AttributeError) || PyErr_ExceptionMatches(PyExc_IOError))
            PyErr_Clear();
        return -1;
    }

    // fileno = int(fileno_value)
    if (!PyObject_TypeCheck(fileno_value, &PyLong_Type)) {
        PyErr_SetString(PyExc_ValueError, "fileno() function must return an integer");
        return -1;
    }

    return PyLong_AsLong(fileno_value);
}

PyObject* file_get_data(PyObject* o, char*&buf, Py_ssize_t& len)
{
    // Use read() instead
    pyo_unique_ptr read_meth(PyObject_GetAttrString(o, "read"));
    pyo_unique_ptr read_args(Py_BuildValue("()"));
    pyo_unique_ptr data(PyObject_Call(read_meth, read_args, NULL));
    if (!data) return nullptr;

#if PY_MAJOR_VERSION >= 3
    if (!PyObject_TypeCheck(data, &PyBytes_Type)) {
        PyErr_SetString(PyExc_ValueError, "read() function must return a bytes object");
        return nullptr;
    }
    if (PyBytes_AsStringAndSize(data, &buf, &len))
        return nullptr;
#else
    if (!PyObject_TypeCheck(data, &PyString_Type)) {
        Py_DECREF(data);
        PyErr_SetString(PyExc_ValueError, "read() function must return a string object");
        return nullptr;
    }
    if (PyString_AsStringAndSize(data, &buf, &len))
        return nullptr;
#endif

    return data.release();
}


std::string object_repr(PyObject* o)
{
    pyo_unique_ptr repr(PyObject_Repr(o));
    if (!repr) throw PythonException();
    return string_from_python(repr);
}

PyObject* dballe_int_to_python(int val)
{
    if (val == MISSING_INT)
        Py_RETURN_NONE;
    return PyLong_FromLong(val);
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


int common_init()
{
    if (!wrpy)
    {
        wrpy = (wrpy_c_api*)PyCapsule_Import("_wreport._C_API", 0);
        if (!wrpy)
            return -1;

        if (wrpy->version_major != 1)
        {
            PyErr_Format(PyExc_RuntimeError, "wreport C API version is %d.%d but only 1.x is supported", wrpy->version_major, wrpy->version_minor);
            return -1;
        }
    }

    return 0;
}

}
}
