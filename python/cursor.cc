/*
 * python/cursor - DB-All.e DB cursor python bindings
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
#include "cursor.h"
#include "record.h"
#include "common.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyGetSetDef dpy_Cursor_getsetters[] = {
    {NULL}
};


static PyMethodDef dpy_Cursor_methods[] = {
    {NULL}
};

static int dpy_Cursor_init(dpy_Cursor* self, PyObject* args, PyObject* kw)
{
    // People should not invoke Cursor() as a constructor, but if they do,
    // this is better than a segfault later on
    PyErr_SetString(PyExc_NotImplementedError, "Cursor objects cannot be constructed explicitly");
    return -1;
}

static void dpy_Cursor_dealloc(dpy_Cursor* self)
{
    if (self->cur)
        delete self->cur;
}

static PyObject* dpy_Cursor_str(dpy_Cursor* self)
{
    /*
    std::string f = self->var.format("None");
    return PyString_FromString(f.c_str());
    */
    return PyString_FromString("Cursor");
}

static PyObject* dpy_Cursor_repr(dpy_Cursor* self)
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
    return PyString_FromString("Cursor object");
}

PyTypeObject dpy_Cursor_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.Cursor",           // tp_name
    sizeof(dpy_Cursor),        // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_Cursor_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Cursor_repr, // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Cursor_str,  // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e Cursor",         // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_Cursor_methods,        // tp_methods
    0,                         // tp_members
    dpy_Cursor_getsetters,     // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_Cursor_init, // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

void register_cursor(PyObject* m)
{
    PyDateTime_IMPORT;

    dpy_Cursor_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Cursor_Type) < 0)
        return;

    Py_INCREF(&dpy_Cursor_Type);
    PyModule_AddObject(m, "Cursor", (PyObject*)&dpy_Cursor_Type);
}

}
}


