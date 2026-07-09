#ifndef DBALLE_PYTHON_CORE_H
#define DBALLE_PYTHON_CORE_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdexcept>

namespace dballe::python {

#define pass_kwlist(kwlist)                                                    \
    (const_cast<char**>(static_cast<const char**>(kwlist)))

/**
 * unique_ptr-like object that contains PyObject pointers, and that calls
 * Py_DECREF on destruction.
 */
template <typename Obj> class py_unique_ptr
{
protected:
    Obj* ptr;

public:
    py_unique_ptr() : ptr(nullptr) {}
    py_unique_ptr(Obj* o) : ptr(o) {}
    py_unique_ptr(const py_unique_ptr&) = delete;
    py_unique_ptr(py_unique_ptr&& o) : ptr(o.ptr) { o.ptr = nullptr; }
    ~py_unique_ptr() { Py_XDECREF(ptr); }
    py_unique_ptr& operator=(const py_unique_ptr&) = delete;
    py_unique_ptr& operator=(py_unique_ptr&& o)
    {
        if (this == &o)
            return *this;
        Py_XDECREF(ptr);
        ptr   = o.ptr;
        o.ptr = nullptr;
        return *this;
    }

    void incref() { Py_XINCREF(ptr); }
    void decref() { Py_XDECREF(ptr); }

    void clear()
    {
        Py_XDECREF(ptr);
        ptr = nullptr;
    }

    void reset(Obj* o)
    {
        Py_XDECREF(ptr);
        ptr = o;
    }

    /// Release the reference without calling Py_DECREF
    Obj* release()
    {
        Obj* res = ptr;
        ptr      = nullptr;
        return res;
    }

    /// Call repr() on the object
    std::string repr()
    {
        if (!ptr)
            return "(null)";
        py_unique_ptr<PyObject> py_repr(PyObject_Repr(ptr));
        if (!py_repr)
        {
            PyErr_Clear();
            return "(repr failed)";
        }
        Py_ssize_t size;
        const char* res = PyUnicode_AsUTF8AndSize(py_repr, &size);
        return std::string(res, size);
    }

    /// Call str() on the object
    std::string str()
    {
        if (!ptr)
            return "(null)";
        py_unique_ptr<PyObject> py_repr(PyObject_Str(ptr));
        if (!py_repr)
        {
            PyErr_Clear();
            return "(str failed)";
        }
        Py_ssize_t size;
        const char* res = PyUnicode_AsUTF8AndSize(py_repr, &size);
        return std::string(res, size);
    }

    /// Use it as a Obj
    operator Obj*() { return ptr; }

    /// Use it as a Obj
    operator Obj*() const { return ptr; }

    /// Use it as a Obj
    Obj* operator->() { return ptr; }

    /// Use it as a Obj
    Obj* operator->() const { return ptr; }

    /// Get the pointer (useful for passing to Py_BuildValue)
    Obj* get() { return ptr; }

    /// Get the pointer (useful for passing to Py_BuildValue)
    Obj* get() const { return ptr; }
};

typedef py_unique_ptr<PyObject> pyo_unique_ptr;

/**
 * Release the GIL during the lifetime of this object;
 */
class ReleaseGIL
{
    PyThreadState* _save = nullptr;

public:
    ReleaseGIL() { _save = PyEval_SaveThread(); }

    ~ReleaseGIL() { lock(); }

    void lock()
    {
        if (!_save)
            return;
        PyEval_RestoreThread(_save);
        _save = nullptr;
    }
};

/**
 * Acquire the GIL prior to invoking python code.
 *
 * This also works correctly if the GIL is already acquired, so it can be used
 * to wrap invocation of python code regardless of the GIL status.
 */
class AcquireGIL
{
    PyGILState_STATE _state;

public:
    AcquireGIL() { _state = PyGILState_Ensure(); }

    ~AcquireGIL() { PyGILState_Release(_state); }
};

/**
 * Exception raised when a python function returns an error with an exception
 * set.
 *
 * When catching this exception, python exception information is already set,
 * so the only thing to do is to return the appropriate error to the python
 * caller.
 *
 * This exception carries to information, because it is all set in the python
 * exception information.
 */
struct PythonException : public std::exception
{
};

/**
 * Throw PythonException if the given pointer is nullptr.
 *
 * This can be used to wrap Python API invocations, throwing PythonException if
 * the API call returned an error.
 */
template <typename T> inline T* throw_ifnull(T* o)
{
    if (!o)
        throw PythonException();
    return o;
}

} // namespace dballe::python

#endif
