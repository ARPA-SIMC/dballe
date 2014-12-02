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
#include "db.h"
#include "common.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyObject* dpy_Cursor_remaining(dpy_Cursor* self, void* closure) { return PyInt_FromLong(self->cur->remaining()); }

static PyGetSetDef dpy_Cursor_getsetters[] = {
    {"remaining", (getter)dpy_Cursor_remaining, NULL, "number of results still to be returned", NULL },
    {NULL}
};

static PyObject* dpy_Cursor_query_attrs(dpy_Cursor* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "attrs", NULL };
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", kwlist, &attrs))
        return NULL;

    // Read the attribute list, if provided
    db::AttrList codes;
    if (!db_read_attrlist(attrs, codes))
        return NULL;

    try {
        self->cur->query_attrs(codes, self->db->attr_rec->rec);
        Py_INCREF(self->db->attr_rec);
        return (PyObject*)self->db->attr_rec;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}


static PyMethodDef dpy_Cursor_methods[] = {
    {"query_attrs",       (PyCFunction)dpy_Cursor_query_attrs, METH_VARARGS | METH_KEYWORDS,
        "Query attributes for the current variable" },
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
    delete self->cur;
    Py_DECREF(self->rec);
    Py_DECREF(self->db);
}

static PyObject* dpy_Cursor_iter(dpy_Cursor* self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

static PyObject* dpy_Cursor_iternext(dpy_Cursor* self)
{
    if (self->cur->next())
    {
        self->cur->to_record(self->rec->rec);
        Py_INCREF(self->rec);
        return (PyObject*)self->rec;
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER, // tp_flags
    "DB-All.e Cursor",         // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)dpy_Cursor_iter,      // tp_iter
    (iternextfunc)dpy_Cursor_iternext, // tp_iternext
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

dpy_Cursor* cursor_create(dpy_DB* db, std::unique_ptr<db::Cursor> cur)
{
    dpy_Cursor* result = PyObject_New(dpy_Cursor, &dpy_Cursor_Type);
    if (!result) return NULL;
    result = (dpy_Cursor*)PyObject_Init((PyObject*)result, &dpy_Cursor_Type);
    Py_INCREF(db);
    result->db = db;
    result->cur = cur.release();
    result->rec = record_create();
    return result;
}

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


