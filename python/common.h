#ifndef DBALLE_PYTHON_COMMON_H
#define DBALLE_PYTHON_COMMON_H

#include <Python.h>
#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/python.h>
#include <wreport/error.h>
#include <wreport/varinfo.h>

namespace dballe {
namespace python {

extern wrpy_c_api* wrpy;

/**
 * unique_ptr-like object that contains PyObject pointers, and that calls
 * Py_DECREF on destruction.
 */
template<typename Obj>
class py_unique_ptr
{
protected:
    Obj* ptr;

public:
    py_unique_ptr(Obj* o) : ptr(o) {}
    py_unique_ptr(const py_unique_ptr&) = delete;
    py_unique_ptr(py_unique_ptr&& o) : ptr(o.ptr) { o.ptr = nullptr; }
    ~py_unique_ptr() { Py_XDECREF(ptr); }
    py_unique_ptr& operator=(const py_unique_ptr&) = delete;
    py_unique_ptr& operator=(py_unique_ptr&& o)
    {
        if (this == &o) return *this;
        Py_XDECREF(ptr);
        ptr = o.ptr;
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

    /// Release the reference without calling Py_DECREF
    Obj* release()
    {
        Obj* res = ptr;
        ptr = nullptr;
        return res;
    }

    /// Use it as a Obj
    operator Obj*() { return ptr; }

    /// Use it as a Obj
    Obj* operator->() { return ptr; }

    /// Get the pointer (useful for passing to Py_BuildValue)
    Obj* get() { return ptr; }

    /// Check if ptr is not nullptr
    operator bool() const { return ptr; }
};

typedef py_unique_ptr<PyObject> pyo_unique_ptr;


/**
 * Release the GIL during the lifetime of this object;
 */
struct ReleaseGIL
{
    PyThreadState *_save = nullptr;

    ReleaseGIL()
    {
        _save = PyEval_SaveThread();
    }

    ~ReleaseGIL()
    {
        lock();
    }

    void lock()
    {
        if (!_save) return;
        PyEval_RestoreThread(_save);
        _save = nullptr;
    }
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
struct PythonException : public std::exception {};

/// Given a wreport exception, set the Python error indicator appropriately.
void set_wreport_exception(const wreport::error& e);

/// Given a generic exception, set the Python error indicator appropriately.
void set_std_exception(const std::exception& e);

#define DBALLE_CATCH_RETURN_PYO \
      catch (PythonException&) { \
        return nullptr; \
    } catch (wreport::error& e) { \
        set_wreport_exception(e); return nullptr; \
    } catch (std::exception& se) { \
        set_std_exception(se); return nullptr; \
    }

#define DBALLE_CATCH_RETURN_INT \
      catch (PythonException&) { \
        return -1; \
    } catch (wreport::error& e) { \
        set_wreport_exception(e); return -1; \
    } catch (std::exception& se) { \
        set_std_exception(se); return -1; \
    }

/// Convert an utf8 string to a python str object
PyObject* string_to_python(const std::string& str);

/// Convert a python string, bytes or unicode to an utf8 string
std::string string_from_python(PyObject* o);

/// Convert a Python object to a double
double double_from_python(PyObject* o);

/// Check if a python object is a string
bool pyobject_is_string(PyObject* o);

/// Call repr() on \a o, and return the result as a string
std::string object_repr(PyObject* o);

/**
 * If val is MISSING_INT, returns None, else return it as a PyLong
 */
PyObject* dballe_int_to_python(int val);

/// Convert a Python object to an integer, returning MISSING_INT if it is None
int dballe_int_from_python(PyObject* o);

/*
 * to_python shortcuts
 */

inline PyObject* to_python(const std::string& s) { return string_to_python(s); }

/*
 * from_python shortcuts
 */
template<typename T> inline T from_python(PyObject*) { throw std::runtime_error("method not implemented"); }
template<> inline std::string from_python<std::string>(PyObject* o) { return string_from_python(o); }
template<> inline double from_python<double>(PyObject* o) { return double_from_python(o); }

/**
 * call o.fileno() and return its result.
 *
 * In case of AttributeError and IOError (parent of UnsupportedOperation, not
 * available from C), it clear the error indicator.
 *
 * Returns -1 if fileno() was not available or some other exception happened.
 * Use PyErr_Occurred to tell between the two.
 */
int file_get_fileno(PyObject* o);

/**
 * call o.data() and return its result, both as a PyObject and as a buffer.
 *
 * The data returned in buf and len will be valid as long as the returned
 * object stays valid.
 */
PyObject* file_get_data(PyObject* o, char*&buf, Py_ssize_t& len);

/**
 * Initialize the python bits to use used by the common functions.
 *
 * This can be called multiple times and will execute only once.
 */
int common_init();

}
}
#endif
