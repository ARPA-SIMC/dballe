/*
 * python/common - Common functions for DB-All.e python bindings
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#ifndef DBALLE_PYTHON_COMMON_H
#define DBALLE_PYTHON_COMMON_H

#include <Python.h>
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
 * Resolve a varcode name to a varcode proper.
 */
wreport::Varcode resolve_varcode(const char* name);

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


}
}

#endif
