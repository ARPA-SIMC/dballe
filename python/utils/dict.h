#ifndef DBALLE_PYTHON_DICT_H
#define DBALLE_PYTHON_DICT_H

#define PY_SSIZE_T_CLEAN
#include "core.h"
#include "values.h"
#include <Python.h>

namespace dballe::python {

template <typename T>
inline void set_dict(PyObject* dict, const char* key, const T& val)
{
    auto pyval = to_python(val);
    if (PyDict_SetItemString(dict, key, pyval))
        throw PythonException();
}

inline void set_dict(PyObject* dict, const char* key, PyObject* val)
{
    if (PyDict_SetItemString(dict, key, val))
        throw PythonException();
}

inline void set_dict(PyObject* dict, const char* key, pyo_unique_ptr& val)
{
    if (PyDict_SetItemString(dict, key, val))
        throw PythonException();
}

} // namespace dballe::python

#endif
