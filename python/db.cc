/*
 * python/db - DB-All.e DB python bindings
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
#include <datetime.h>
#include "db.h"
#include "common.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

/*
static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val);
static int dpy_Record_contains(dpy_Record* self, PyObject *value);
static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key);

static PyObject* dpy_Var_code(dpy_Var* self, void* closure) { return format_varcode(self->var.code()); }
static PyObject* dpy_Var_isset(dpy_Var* self, void* closure) {
    if (self->var.isset())
        return Py_True;
    else
        return Py_False;
}
*/

static PyGetSetDef dpy_DB_getsetters[] = {
    //{"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    //{"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {NULL}
};

static PyObject* dpy_DB_connect(PyTypeObject *type, PyObject *args)
{
    const char* dsn;
    const char* user = "";
    const char* pass = "";
    if (!PyArg_ParseTuple(args, "s|ss", &dsn, &user, &pass))
        return NULL;

    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result) return NULL;
    result = (dpy_DB*)PyObject_Init((PyObject*)result, &dpy_DB_Type);
    result->db = DB::connect(dsn, user, pass).release();
    return (PyObject*)result;
}

static PyObject* dpy_DB_connect_from_file(PyTypeObject *type, PyObject *args)
{
    const char* fname;
    if (!PyArg_ParseTuple(args, "s", &fname))
        return NULL;

    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result) return NULL;
    result = (dpy_DB*)PyObject_Init((PyObject*)result, &dpy_DB_Type);
    result->db = DB::connect_from_file(fname).release();
    return (PyObject*)result;
}

static PyObject* dpy_DB_connect_from_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result) return NULL;
    result = (dpy_DB*)PyObject_Init((PyObject*)result, &dpy_DB_Type);
    result->db = DB::connect_from_url(url).release();
    return (PyObject*)result;
}

static PyObject* dpy_DB_connect_test(PyTypeObject *type)
{
    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result) return NULL;
    result = (dpy_DB*)PyObject_Init((PyObject*)result, &dpy_DB_Type);
    result->db = DB::connect_test().release();
    return (PyObject*)result;
}

static PyMethodDef dpy_DB_methods[] = {
    {"connect",           (PyCFunction)dpy_DB_connect, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to an ODBC source" },
    {"connect_from_file", (PyCFunction)dpy_DB_connect_from_file, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to a SQLite file" },
    {"connect_from_url",  (PyCFunction)dpy_DB_connect_from_url, METH_VARARGS | METH_CLASS,
        "Create a DB as defined in an URL-like string" },
    {"connect_test",      (PyCFunction)dpy_DB_connect_test, METH_NOARGS | METH_CLASS,
        "Create a DB for running the test suite, as configured in the test environment" },
    /*
    {"get", (PyCFunction)dpy_Record_get, METH_VARARGS, "lookup a value, returning a fallback value (None by default) if unset" },
    {"copy", (PyCFunction)dpy_Record_copy, METH_NOARGS, "return a copy of the Record" },
    {"keys", (PyCFunction)dpy_Record_keys, METH_NOARGS, "return a sequence with all the varcodes of the variables set on the Record. Note that this does not include keys." },
    {"vars", (PyCFunction)dpy_Record_vars, METH_NOARGS, "return a sequence with all the variables set on the Record. Note that this does not include keys." },
    {"update", (PyCFunction)dpy_Record_update, METH_VARARGS | METH_KEYWORDS, "set many record keys/vars in a single shot, via kwargs" },
    {"date_extremes", (PyCFunction)dpy_Record_date_extremes, METH_NOARGS, "get two datetime objects with the lower and upper bounds of the datetime period in this record" },
    */
    {NULL}
};

static int dpy_DB_init(dpy_DB* self, PyObject* args, PyObject* kw)
{
    // People should not invoke DB() as a constructor, but if they do,
    // this is better than a segfault later on
    PyErr_SetString(PyExc_NotImplementedError, "DB objects cannot be constructed explicitly");
    return -1;
}

static void dpy_DB_dealloc(dpy_DB* self)
{
    if (self->db)
        delete self->db;
}

static PyObject* dpy_DB_str(dpy_DB* self)
{
    /*
    std::string f = self->var.format("None");
    return PyString_FromString(f.c_str());
    */
    return PyString_FromString("DB");
}

static PyObject* dpy_DB_repr(dpy_DB* self)
{
    /*
    string res = "Var('";
    res += varcode_format(self->var.code());
    if (self->var.info()->is_string())
    {
        res += "', '";
        res += self->var.format();
        res += "')";
    } else {
        res += "', ";
        res += self->var.format("None");
        res += ")";
    }
    return PyString_FromString(res.c_str());
    */
    return PyString_FromString("DB object");
}

PyTypeObject dpy_DB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.Record",           // tp_name
    sizeof(dpy_DB),            // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_DB_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_DB_repr,     // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_DB_str,      // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e DB",             // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_DB_methods,            // tp_methods
    0,                         // tp_members
    dpy_DB_getsetters,         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_DB_init,     // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

void register_db(PyObject* m)
{
    PyDateTime_IMPORT;

    dpy_DB_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DB_Type) < 0)
        return;

    Py_INCREF(&dpy_DB_Type);
    PyModule_AddObject(m, "DB", (PyObject*)&dpy_DB_Type);
}

}
}

