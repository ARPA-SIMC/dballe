#ifndef DBALLE_PYTHON_COMMON_H
#define DBALLE_PYTHON_COMMON_H

#include <Python.h>
#include <dballe/types.h>
#include <wreport/error.h>
#include <wreport/varinfo.h>

namespace dballe {
namespace python {

/**
 * Scope-managed python reference: calls Py_DECREF when exiting the scope
 */
struct OwnedPyObject
{
    PyObject* o;
    OwnedPyObject(PyObject* o) : o(o) {}
    ~OwnedPyObject() { Py_XDECREF(o); }

    /**
     * Release the reference without calling Py_DECREF
     */
    PyObject* release()
    {
        PyObject* res = o;
        o = NULL;
        return res;
    }

    // Use it as a PyObject
    operator PyObject*() { return o; }

    // Get the pointer (useful for passing to Py_BuildValue)
    PyObject* get() { return o; }

private:
    // Disable copy for now
    OwnedPyObject(const OwnedPyObject&);
    OwnedPyObject& operator=(const OwnedPyObject&);
};

/**
 * Return a python string representing a varcode
 */
PyObject* format_varcode(wreport::Varcode code);

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

/**
 * Initialize the python bits to use used by the common functions.
 *
 * This can be called multiple times and will execute only once.
 */
void common_init();

}
}
#endif
