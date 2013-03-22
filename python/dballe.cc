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
#include "common.h"
#include "vartable.h"
#include "varinfo.h"
#include "var.h"
#include "record.h"
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

static PyMethodDef dballe_methods[] = {
    {"varinfo", (PyCFunction)dballe_varinfo, METH_VARARGS, "Query the DB-All.e variable table returning a Varinfo" },
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
}

}
