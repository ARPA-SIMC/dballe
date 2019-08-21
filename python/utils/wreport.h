#ifndef DBALLE_PYTHON_WREPORT_H
#define DBALLE_PYTHON_WREPORT_H

#include <wreport/python.h>
#include "core.h"

#define WREPORT_API1_MIN_VERSION 0

namespace dballe {
namespace python {

class Wreport
{
protected:
    wrpy_c_api* api = nullptr;

public:
    Wreport();
    ~Wreport();

    void import();

    /// Throw a python error if import() was not called
    void require_imported() const;

    /// Check if an object is a wreport.Var
    bool var_check(PyObject* ob) const
    {
        require_imported();
        return Py_TYPE(ob) == api->var_type ||
               PyType_IsSubtype(Py_TYPE(ob), api->var_type);
    }

    /// Create a new unset wreport.Var object
    PyObject* var_create(const wreport::Varinfo& vi) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create(vi));
    }

    /// Create a new wreport.Var object with an integer value
    PyObject* var_create(const wreport::Varinfo& vi, int val) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create_i(vi, val));
    }

    /// Create a new wreport.Var object with a double value
    PyObject* var_create(const wreport::Varinfo& vi, double val) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create_d(vi, val));
    }

    /// Create a new wreport.Var object with a C string value
    PyObject* var_create(const wreport::Varinfo& vi, const char* val) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create_c(vi, val));
    }

    /// Create a new wreport.Var object with a std::string value
    PyObject* var_create(const wreport::Varinfo& vi, const std::string& val) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create_s(vi, val));
    }

    /// Create a new wreport.Var object as a copy of an existing var
    PyObject* var_create(const wreport::Var& var) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create_copy(var));
    }

#if WREPORT_API1_MIN_VERSION >= 1
    /// Create a new wreport.Var object moving an existing var
    PyObject* var_create(wreport::Var&& var) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_create_move(std::move(var)));
    }
#endif

    /// Read the value of a variable as a new Python object
    PyObject* var_value_to_python(const wreport::Var& var) const
    {
        require_imported();
        return (PyObject*)throw_ifnull(api->var_value_to_python(var));
    }

    /// Set the value of a variable from a Python object (borrowed reference)
    void var_value_from_python(PyObject* ob, wreport::Var& var) const
    {
        require_imported();
        if (api->var_value_from_python(ob, var) == -1)
            throw PythonException();
    }
};

}
}

#endif
