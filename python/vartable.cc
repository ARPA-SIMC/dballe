#include "vartable.h"
#include "varinfo.h"
#include "common.h"
#include "dballe/var.h"
#include <wreport/vartable.h>

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyObject* dpy_Vartable_get(PyTypeObject *type, PyObject *args, PyObject *kw);
static PyObject* dpy_Vartable_query(dpy_Vartable *self, PyObject *args, PyObject *kw);

static PyMethodDef dpy_Vartable_methods[] = {
    {"query", (PyCFunction)dpy_Vartable_query, METH_VARARGS, "Query the table, returning a Varinfo object or raising an exception" },
    {NULL}
};

static int dpy_Vartable_init(dpy_Vartable* self, PyObject* args, PyObject* kw)
{
    const char* table_name = 0;
    if (!PyArg_ParseTuple(args, "s", &table_name))
        return -1;

    try {
        // Make it point to the table we want
        self->table = wreport::Vartable::get(table_name);
        return 0;
    } DBALLE_CATCH_RETURN_INT
}

static PyObject* dpy_Vartable_id(dpy_Vartable* self, void* closure)
{
    return PyUnicode_FromString(self->table->id().c_str());
}

static PyGetSetDef dpy_Vartable_getsetters[] = {
    {"id", (getter)dpy_Vartable_id, NULL, "name of the table", NULL},
    {NULL}
};

static PyObject* dpy_Vartable_str(dpy_Vartable* self)
{
    return PyUnicode_FromString(self->table->id().c_str());
}

static PyObject* dpy_Vartable_repr(dpy_Vartable* self)
{
    return PyUnicode_FromFormat("Vartable('%s')", self->table->id().c_str());
}

static int dpy_Vartable_len(dpy_Vartable* self)
{
    return self->table->size();
}

static PyObject* dpy_Vartable_item(dpy_Vartable* self, Py_ssize_t i)
{
    // We can cast to size_t: since we provide sq_length, i is supposed to
    // always be positive
    if ((size_t)i >= self->table->size())
    {
        PyErr_SetString(PyExc_IndexError, "table index out of range");
        return NULL;
    }
    try {
        return (PyObject*)varinfo_create(Varinfo((*self->table)[i]));
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Vartable_getitem(dpy_Vartable* self, PyObject* key)
{
    if (PyIndex_Check(key)) {
        Py_ssize_t i = PyNumber_AsSsize_t(key, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += self->table->size();
        return dpy_Vartable_item(self, i);
    }

    string varname;
    if (string_from_python(key, varname))
        return NULL;

    try {
        return (PyObject*)varinfo_create(self->table->query(resolve_varcode(varname)));
    } DBALLE_CATCH_RETURN_PYO
}

static int dpy_Vartable_contains(dpy_Vartable* self, PyObject *value)
{
    string varname;
    if (string_from_python(value, varname))
        return -1;
    try {
        return self->table->contains(resolve_varcode(varname)) ? 1 : 0;
    } DBALLE_CATCH_RETURN_INT
}

static PySequenceMethods dpy_Vartable_sequence = {
    (lenfunc)dpy_Vartable_len,        // sq_length
    0,                                // sq_concat
    0,                                // sq_repeat
    (ssizeargfunc)dpy_Vartable_item,  // sq_item
    0,                                // sq_slice
    0,                                // sq_ass_item
    0,                                // sq_ass_slice
    (objobjproc)dpy_Vartable_contains, // sq_contains
};

static PyMappingMethods dpy_Vartable_mapping = {
    (lenfunc)dpy_Vartable_len,         // __len__
    (binaryfunc)dpy_Vartable_getitem,  // __getitem__
    0,                // __setitem__
};

PyTypeObject dpy_Vartable_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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
    &dpy_Vartable_sequence,    // tp_as_sequence
    &dpy_Vartable_mapping,     // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Vartable_str, // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
    R"(
        Collection of Varinfo objects indexed by WMO BUFR/CREX table B code.

        A Vartable is instantiated by the name (without extension) of the table
        file installed in wreport's data directory (normally,
        ``/usr/share/wreport/``)::

            table = dballe.Vartable("B0000000000000023000")
            print(table["B12101"].desc)

            for i in table:
                print(i.code, i.desc)
    )", // tp_doc
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
    (initproc)dpy_Vartable_init, // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

static PyObject* dpy_Vartable_query(dpy_Vartable *self, PyObject *args, PyObject *kw)
{
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
    } DBALLE_CATCH_RETURN_PYO
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
