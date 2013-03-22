/*
 * python/varinfo - DB-All.e Varinfo python bindings
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
#include "varinfo.h"
#include "common.h"

using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyMethodDef dpy_Varinfo_methods[] = {
    {NULL}
};

static PyObject* dpy_Varinfo_is_string(dpy_Varinfo *self, void* closure)
{
    if (self->info->is_string())
        return Py_True;
    else
        return Py_False;
}
static PyObject* dpy_Varinfo_var(dpy_Varinfo *self, void* closure) { return format_varcode(self->info->var); }
static PyObject* dpy_Varinfo_len(dpy_Varinfo* self, void* closure) { return PyInt_FromLong(self->info->len); }
static PyObject* dpy_Varinfo_unit(dpy_Varinfo* self, void* closure) { return PyString_FromString(self->info->unit); }
static PyObject* dpy_Varinfo_desc(dpy_Varinfo* self, void* closure) { return PyString_FromString(self->info->desc); }
static PyObject* dpy_Varinfo_scale(dpy_Varinfo* self, void* closure) { return PyInt_FromLong(self->info->scale); }
static PyObject* dpy_Varinfo_ref(dpy_Varinfo* self, void* closure) { return PyInt_FromLong(self->info->ref); }


static PyGetSetDef dpy_Varinfo_getsetters[] = {
    {"is_string", (getter)dpy_Varinfo_is_string, NULL, "true if the value is a string", NULL },
    {"var", (getter)dpy_Varinfo_var, NULL, "variable code", NULL },
    {"len", (getter)dpy_Varinfo_len, NULL, "number of significant digits", NULL},
    {"scale", (getter)dpy_Varinfo_scale, NULL, "scale of the value as a power of 10", NULL},
    {"ref", (getter)dpy_Varinfo_ref, NULL, "reference value added after scaling", NULL},
    {"unit", (getter)dpy_Varinfo_unit, NULL, "measurement unit", NULL},
    {"desc", (getter)dpy_Varinfo_desc, NULL, "description", NULL},
    {NULL}
};

static int dpy_Varinfo_init(dpy_Varinfo* self, PyObject* args, PyObject* kw)
{
    // People should not invoke Varinfo() as a constructor, but if they do,
    // this is better than a segfault later on
    PyErr_SetString(PyExc_NotImplementedError, "Varinfo objects cannot be constructed explicitly");
    return -1;
}

static PyObject* dpy_Varinfo_str(dpy_Varinfo* self)
{
    return format_varcode(self->info->var);
}

static PyObject* dpy_Varinfo_repr(dpy_Varinfo* self)
{
    wreport::Varcode code = self->info->var;
    char buf[20];
    snprintf(buf, 20, "Varinfo('%c%02d%03d')",
            WR_VAR_F(code) == 0 ? 'B' :
            WR_VAR_F(code) == 1 ? 'R' :
            WR_VAR_F(code) == 2 ? 'C' :
            WR_VAR_F(code) == 3 ? 'D' : '?',
            WR_VAR_X(code), WR_VAR_Y(code));
    return PyString_FromString(buf);
}


static PyTypeObject dpy_Varinfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.Varinfo",         // tp_name
    sizeof(dpy_Varinfo),  // tp_basicsize
    0,                         // tp_itemsize
    0,                         // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Varinfo_repr, // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Varinfo_str, // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
    "DB-All.e Varinfo object", // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_Varinfo_methods,       // tp_methods
    0,                         // tp_members
    dpy_Varinfo_getsetters,    // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_Varinfo_init, // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

dpy_Varinfo* varinfo_create(const Varinfo& v)
{
    dpy_Varinfo* result = PyObject_New(dpy_Varinfo, &dpy_Varinfo_Type);
    if (!result) return NULL;
    result = (dpy_Varinfo*)PyObject_Init((PyObject*)result, &dpy_Varinfo_Type);
    new (&result->info) Varinfo(v);
    return result;
}

void register_varinfo(PyObject* m)
{
    dpy_Varinfo_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Varinfo_Type) < 0)
        return;

    Py_INCREF(&dpy_Varinfo_Type);
    PyModule_AddObject(m, "Varinfo", (PyObject*)&dpy_Varinfo_Type);
}

}
}
