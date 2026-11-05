// SPDX-License-Identifier: BSD-3-Clause

#include "core.h"

namespace dballe::python {

std::string repr(PyObject* o)
{
    if (!o)
        return "(null)";
    py_unique_ptr<PyObject> py_repr(PyObject_Repr(o));
    if (!py_repr)
    {
        PyErr_Clear();
        return "(repr failed)";
    }
    Py_ssize_t size;
    const char* res = PyUnicode_AsUTF8AndSize(py_repr, &size);
    return std::string(res, size);
}

std::string str(PyObject* o)
{
    if (!o)
        return "(null)";
    py_unique_ptr<PyObject> py_repr(PyObject_Str(o));
    if (!py_repr)
    {
        PyErr_Clear();
        return "(str failed)";
    }
    Py_ssize_t size;
    const char* res = PyUnicode_AsUTF8AndSize(py_repr, &size);
    return std::string(res, size);
}

bool hasattr(PyObject* obj, const char* name)
{
#if PY_VERSION_HEX >= 0x030d0000
    int res = PyObject_HasAttrStringWithError(obj, name);
    if (res == -1)
        throw PythonException();
    return res == 1;
#else
    return PyObject_HasAttrString(obj, name);
#endif
}

py_unique_ptr<PyObject> get_optional_attr(PyObject* obj, const char* name)
{
    // TODO: use this from Python 3.13
#if PY_VERSION_HEX >= 0x030d0000
    PyObject* res_untracked;
    if (PyObject_GetOptionalAttrString(obj, name, &res_untracked) == -1)
        throw PythonException();
    return pyo_unique_ptr(res_untracked);
#else
    if (!hasattr(obj, name))
        return pyo_unique_ptr();
    return pyo_unique_ptr(throw_ifnull(PyObject_GetAttrString(obj, name)));
#endif
}

py_unique_ptr<PyObject> call_noargs(PyObject* obj)
{
#if PY_VERSION_HEX >= 0x03090000
    return throw_ifnull(PyObject_CallNoArgs(obj));
#else
    pyo_unique_ptr empty_tuple(throw_ifnull(PyTuple_New(0)));
    return throw_ifnull(PyObject_Call(obj, empty_tuple, nullptr));
#endif
}

const char* PythonException::what() const noexcept
{
    AcquireGIL gil;
    // TODO: use PyErr_FormatUnraisable from Python 3.13
    fprintf(stderr, "Python exception caught from C++\n");
    PyErr_WriteUnraisable(nullptr);
    return "Python exception caught from C++";
}

Except::Except()
{
    // TODO: use PyErr_GetRaisedException() from Python 3.12
    // exc = PyErr_GetRaisedException()
    PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
}

Except::~Except()
{
    Py_XDECREF(exc_type);
    Py_XDECREF(exc_value);
    Py_XDECREF(exc_traceback);
}

[[noreturn]] void Except::raise()
{
    // TODO: use PyErr_SetRaisedException() from Python 3.12
    // PyErr_SetRaisedException(exc);
    PyErr_Restore(exc_type, exc_value, exc_traceback);
    exc_type      = nullptr;
    exc_value     = nullptr;
    exc_traceback = nullptr;
    throw PythonException();
}

} // namespace dballe::python
