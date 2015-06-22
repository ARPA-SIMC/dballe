#ifndef DBALLE_PYTHON_COMMON_H
#define DBALLE_PYTHON_COMMON_H

#include <Python.h>
#include <dballe/types.h>
#include <wreport/error.h>
#include <wreport/varinfo.h>

namespace dballe {
namespace python {

/**
 * unique_ptr-like object that contains PyObject pointers, and that calls
 * Py_DECREF on destruction.
 */
class pyo_unique_ptr
{
protected:
    PyObject* ptr;

public:
    pyo_unique_ptr(PyObject* o) : ptr(o) {}
    pyo_unique_ptr(const pyo_unique_ptr&) = delete;
    pyo_unique_ptr(pyo_unique_ptr&& o) : ptr(o.ptr) { o.ptr = nullptr; }
    ~pyo_unique_ptr() { Py_XDECREF(ptr); }
    pyo_unique_ptr& operator=(const pyo_unique_ptr&) = delete;
    pyo_unique_ptr& operator=(pyo_unique_ptr&& o)
    {
        if (this == &o) return *this;
        Py_XDECREF(ptr);
        ptr = o.ptr;
        o.ptr = nullptr;
        return *this;
    }

    /// Release the reference without calling Py_DECREF
    PyObject* release()
    {
        PyObject* res = ptr;
        ptr = nullptr;
        return res;
    }

    /// Use it as a PyObject
    operator PyObject*() { return ptr; }

    /// Get the pointer (useful for passing to Py_BuildValue)
    PyObject* get() { return ptr; }

    /// Check if ptr is not nullptr
    operator bool() const { return ptr; }
};

/**
 * Return a python string representing a varcode
 */
PyObject* format_varcode(wreport::Varcode code);

/// Given a wreport exception, set the Python error indicator appropriately.
void set_wreport_exception(const wreport::error& e);

/**
 * Given a wreport exception, set the Python error indicator appropriately.
 *
 * @retval
 *   Always returns NULL, so one can do:
 *   try {
 *     // ...code...
 *   } catch (wreport::error& e) {
 *     return raise_wreport_exception(e);
 *   }
 */
PyObject* raise_wreport_exception(const wreport::error& e);

/// Given a generic exception, set the Python error indicator appropriately.
void set_std_exception(const std::exception& e);

/**
 * Given a generic exception, set the Python error indicator appropriately.
 *
 * @retval
 *   Always returns NULL, so one can do:
 *   try {
 *     // ...code...
 *   } catch (std::exception& e) {
 *     return raise_std_exception(e);
 *   }
 */
PyObject* raise_std_exception(const std::exception& e);

#define DBALLE_CATCH_RETURN_PYO \
      catch (wreport::error& e) { \
        set_wreport_exception(e); return nullptr; \
    } catch (std::exception& se) { \
        set_std_exception(se); return nullptr; \
    }

#define DBALLE_CATCH_RETURN_INT \
      catch (wreport::error& e) { \
        set_wreport_exception(e); return -1; \
    } catch (std::exception& se) { \
        set_std_exception(se); return -1; \
    }

/// Convert a Datetime to a python datetime object
PyObject* datetime_to_python(const Datetime& dt);

/// Convert a python datetime object to a Datetime
int datetime_from_python(PyObject* dt, Datetime& out);

/// Convert a Level to a python 4-tuple
PyObject* level_to_python(const Level& lev);

/// Convert a 4-tuple to a Level
int level_from_python(PyObject* o, Level& out);

/// Convert a Trange to a python 3-tuple
PyObject* trange_to_python(const Trange& tr);

/// Convert a 3-tuple to a Trange
int trange_from_python(PyObject* o, Trange& out);

/// Convert a python string, bytes or unicode to an utf8 string
int string_from_python(PyObject* o, std::string& out);

/// Call repr() on \a o, and return the result in \a out
int object_repr(PyObject* o, std::string& out);

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
void common_init();

}
}
#endif
