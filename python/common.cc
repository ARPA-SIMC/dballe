#include "common.h"
#include <Python.h>
#include "dballe/types.h"
#include "dballe/core/var.h"
#include <string>

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

PyObject* string_to_python(const char* str)
{
    return throw_ifnull(PyUnicode_FromString(str));
}

PyObject* string_to_python(const std::string& str)
{
    return throw_ifnull(PyUnicode_FromStringAndSize(str.data(), str.size()));
}

bool pyobject_is_string(PyObject* o)
{
    if (PyUnicode_Check(o) || PyBytes_Check(o))
        return true;
    return false;
}

std::string string_from_python(PyObject* o)
{
    if (PyUnicode_Check(o))
        return throw_ifnull(PyUnicode_AsUTF8(o));
    if (PyBytes_Check(o))
        return throw_ifnull(PyBytes_AsString(o));
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

PyObject* double_to_python(double val)
{
    return throw_ifnull(PyFloat_FromDouble(val));
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
    return string_from_python(repr);
}

std::string build_method_doc(const char* name, const char* signature, const char* returns, const char* summary, const char* doc)
{
    std::string res;
    unsigned doc_indent = 0;
    if (doc)
    {
        // Look up doc indentation
        // Count the leading spaces of the first non-empty line
        unsigned indent = 0;
        for (const char* c = doc; *c; ++c)
        {
            if (isblank(*c))
                ++indent;
            else if (*c == '\n' || *c == '\r')
            {
                // strip empty lines
                doc = c;
                indent = 0;
            }
            else
            {
                doc_indent = indent;
                break;
            }
        }
    }

    // Function name and signature
    res += name;
    res += '(';
    res += signature;
    res += ')';
    if (returns)
    {
        res += " -> ";
        res += returns;
    }
    res += "\n\n";

    // Indented summary
    if (summary)
    {
        for (unsigned i = 0; i < doc_indent; ++i) res += ' ';
        res += summary;
        res += "\n\n";
    }

    // Docstring
    if (doc)
        res += doc;

    // Return a C string with a copy of res
    return res;
}

void common_init()
{
    if (!wrpy)
    {
        wrpy = (wrpy_c_api*)PyCapsule_Import("_wreport._C_API", 0);
        if (!wrpy)
            throw PythonException();

#if 0
        // TODO: reenable when the new wreport has been deployed
        if (wrpy->version_major != 1)
        {
            PyErr_Format(PyExc_RuntimeError, "wreport C API version is %d.%d but only 1.x is supported", wrpy->version_major, wrpy->version_minor);
            throw PythonException();
        }
#endif
    }
}

}
}
