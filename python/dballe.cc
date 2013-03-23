/*
 * python/dballe - DB-All.e python bindings
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
#include <Python.h>
#include "config.h"
#include "common.h"
#include "vartable.h"
#include "varinfo.h"
#include "var.h"
#include "record.h"
#ifdef HAVE_DBALLE_DB
#include "db.h"
#include "cursor.h"
#endif
#include "dballe/core/var.h"

using namespace dballe::python;

extern "C" {

static PyObject* dballe_varinfo(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    const char* var_name;
    if (!PyArg_ParseTuple(args, "s", &var_name))
        return NULL;
    return (PyObject*)varinfo_create(dballe::varinfo(resolve_varcode(var_name)));
}

static PyObject* dballe_var(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    const char* var_name;
    PyObject* val = 0;
    if (!PyArg_ParseTuple(args, "s|O", &var_name, &val))
        return NULL;
    if (val)
    {
        if (PyFloat_Check(val))
        {
            double v = PyFloat_AsDouble(val);
            if (v == -1.0 && PyErr_Occurred())
                return NULL;
            return (PyObject*)var_create(dballe::varinfo(resolve_varcode(var_name)), v);
        } else if (PyInt_Check(val)) {
            long v = PyInt_AsLong(val);
            if (v == -1 && PyErr_Occurred())
                return NULL;
            return (PyObject*)var_create(dballe::varinfo(resolve_varcode(var_name)), (int)v);
        } else if (PyString_Check(val)) {
            const char* v = PyString_AsString(val);
            if (v == NULL)
                return NULL;
            return (PyObject*)var_create(dballe::varinfo(resolve_varcode(var_name)), v);
        } else if (val == Py_None) {
            return (PyObject*)var_create(dballe::varinfo(resolve_varcode(var_name)));
        } else {
            PyErr_SetString(PyExc_TypeError, "Expected int, float, str or None");
            return NULL;
        }
    } else
        return (PyObject*)var_create(dballe::varinfo(resolve_varcode(var_name)));
}

static PyMethodDef dballe_methods[] = {
    {"varinfo", (PyCFunction)dballe_varinfo, METH_VARARGS, "Query the DB-All.e variable table returning a Varinfo" },
    {"var", (PyCFunction)dballe_var, METH_VARARGS, "Query the DB-All.e variable table returning an undefined Var" },
    { NULL }
};

PyMODINIT_FUNC initdballe(void)
{
    using namespace dballe::python;

    PyObject* m;

    m = Py_InitModule3("dballe", dballe_methods,
            "DB-All.e Python interface.");

    register_vartable(m);
    register_varinfo(m);
    register_var(m);
    register_record(m);
#ifdef HAVE_DBALLE_DB
    register_db(m);
    register_cursor(m);
#endif
}

}
