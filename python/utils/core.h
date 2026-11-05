// SPDX-License-Identifier: BSD-3-Clause

#ifndef DBALLE_PYTHON_CORE_H
#define DBALLE_PYTHON_CORE_H

/*
 * TODO: set this to the earliest supported python version to see how far we
 * are from the stable API
 */
// #define Py_LIMITED_API 0x03060000
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdexcept>

namespace dballe::python {

#define pass_kwlist(kwlist)                                                    \
    (const_cast<char**>(static_cast<const char**>(kwlist)))

template <typename Obj> class py_unique_ptr;

/**
 * Call repr() on the object.
 *
 * In case a Python exception is raised, it is cleared and the return string is
 * an error message
 */
std::string repr(PyObject* o);

/**
 * Call str() on the object
 *
 * In case a Python exception is raised, it is cleared and the return string is
 * an error message
 */
std::string str(PyObject* o);

/**
 * Wrapper for PyObject_HasAttrStringWithError(), or a polyfill implementation
 * for older pythons
 */
bool hasattr(PyObject* obj, const char* name);

/**
 * Wrapper for PyObject_GetOptionalAttr, or polyfill implementation for older
 * pythons
 */
py_unique_ptr<PyObject> get_optional_attr(PyObject* obj, const char* name);

/**
 * Invoke PyObject_CallNoArgs, or a polyfill if not available
 */
py_unique_ptr<PyObject> call_noargs(PyObject* obj);

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
        return dballe::python::repr(ptr);
    }

    /// Call str() on the object
    std::string str()
    {
        if (!ptr)
            return "(null)";
        return dballe::python::str(ptr);
    }

    /// Call hasattr(self, name)
    bool hasattr(const char* name) { return dballe::python::hasattr(ptr, name); }

    /**
     * Calls getattr, returning nullptr if the object does not have the
     * attribute
     */
    py_unique_ptr<PyObject> get_optional_attr(const char* name)
    {
        return dballe::python::get_optional_attr(ptr, name);
    }

    /**
     * Invoke PyObject_CallNoArgs, or a polyfill if not available
     */
    py_unique_ptr<PyObject> call_noargs() { return dballe::python::call_noargs(ptr); }

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
 * This exception carries no information, because it is all set in the python
 * exception information.
 */
struct PythonException : public std::exception
{
public:
    /**
     * Return a placeholder error message and print the Python exception
     * information on stderr.
     *
     * It is assumed that this exception is never fully handled from C++, but
     * is only used to forward the error condition back to Python. In case it
     * is, such as in test code that uses Python but is not called from Python
     * code, then this can give useful information about what is going on.
     */
    const char* what() const noexcept override;
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

/**
 * RAII version of a python try: … except Exception as exc: … block
 */
class Except
{
protected:
    // This uses PyErr_Fetch to be compatible with older Python: use
    // PyErr_GetRaisedException() from Python 3.12, which will only need to
    // track the exception instance
    PyObject* exc_type      = nullptr;
    PyObject* exc_value     = nullptr;
    PyObject* exc_traceback = nullptr;

public:
    Except();
    Except(const Except&) = delete;
    Except(Except&&)      = delete;
    ~Except();

    Except& operator=(const Except&) = delete;
    Except& operator=(Except&)       = delete;

    /**
     * Access the exception that was caught.
     *
     * This can be nullptr if there was no active exception.
     */
    operator PyObject*() const { return exc_value; }

    /// Explicitly get the exception PyObject
    PyObject* get() const { return exc_value; }

    /**
     * Reraise the exception.
     *
     * Throws PythonException.
     */
    [[noreturn]] void raise();
};

} // namespace dballe::python

#endif
