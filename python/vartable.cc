/*
 * python/vartable - DB-All.e Vartable python bindings
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
#include "vartable.h"
#include "varinfo.h"
#include "common.h"
#include <wreport/vartable.h>

using namespace dballe::python;

extern "C" {

typedef struct {
    PyObject_HEAD
    const wreport::Vartable* table;
} dpy_Vartable;

static PyObject* dpy_Vartable_get(PyTypeObject *type, PyObject *args, PyObject *kw);
static PyObject* dpy_Vartable_query(dpy_Vartable *self, PyObject *args, PyObject *kw);

static PyMethodDef dpy_Vartable_methods[] = {
    {"get",   (PyCFunction)dpy_Vartable_get, METH_VARARGS | METH_CLASS, "Create a Vartable for the table with the given name" },
    {"query", (PyCFunction)dpy_Vartable_query, METH_VARARGS, "Query the table, returning a Varinfo object or raising an exception" },
    {NULL}
};

static PyObject* dpy_Vartable_id(dpy_Vartable* self, void* closure)
{
    if (self->table)
        return PyString_FromString(self->table->id().c_str());
    else
        Py_RETURN_NONE;
}

static PyGetSetDef dpy_Vartable_getsetters[] = {
    {"id", (getter)dpy_Vartable_id, NULL, "name of the table", NULL},
    {NULL}
};

static PyObject* dpy_Vartable_str(dpy_Vartable* self)
{
    if (self->table)
        return PyString_FromString(self->table->id().c_str());
    else
        return PyString_FromString("<empty>");
}

static PyObject* dpy_Vartable_repr(dpy_Vartable* self)
{
    if (self->table)
        return PyString_FromFormat("Vartable('%s')", self->table->id().c_str());
    else
        return PyString_FromString("Vartable()");
}

static PyTypeObject dpy_Vartable_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.Vartable",         // tp_name
    sizeof(dpy_Vartable),  // tp_basicsize
    0,                         // tp_itemsize
    0,                         // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Vartable_repr, // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Vartable_str, // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
    "DB-All.e Vartable object", // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_Vartable_methods,      // tp_methods
    0,                         // tp_members
    dpy_Vartable_getsetters,   // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    0,                         // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

static PyObject* dpy_Vartable_get(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    dpy_Vartable* result = 0;
    const char* table_name = 0;

    if (!PyArg_ParseTuple(args, "s", &table_name))
        return NULL;

    // Create a new Vartable
    result = (dpy_Vartable*)PyObject_CallObject((PyObject*)&dpy_Vartable_Type, NULL);

    // Make it point to the table we want
    result->table = wreport::Vartable::get(table_name);

    return (PyObject*)result;
}

static PyObject* dpy_Vartable_query(dpy_Vartable *self, PyObject *args, PyObject *kw)
{
    dpy_Varinfo* result = 0;
    const char* varname = 0;
    if (!self->table)
    {
        PyErr_SetString(PyExc_KeyError, "table is empty");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "s", &varname))
        return NULL;
    try {
        return (PyObject*)varinfo_create(self->table->query(resolve_varcode(varname)));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

}

namespace dballe {
namespace python {

void register_vartable(PyObject* m)
{
    dpy_Vartable_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Vartable_Type) < 0)
        return;

    Py_INCREF(&dpy_Vartable_Type);
    PyModule_AddObject(m, "Vartable", (PyObject*)&dpy_Vartable_Type);
}

}
}
