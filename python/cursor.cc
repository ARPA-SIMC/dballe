#include <Python.h>
#include <dballe/core/record.h>
#include "cursor.h"
#include "record.h"
#include "db.h"
#include "common.h"
#include <algorithm>
#include "config.h"

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_Check PyLong_Check
    #define PyInt_Type PyLong_Type
    #define Py_TPFLAGS_HAVE_ITER 0
#endif

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyObject* dpy_Cursor_remaining(dpy_Cursor* self, void* closure)
{
    if (self->cur == nullptr)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot access a cursor after the with block where it is used");
        return nullptr;
    }
    return PyInt_FromLong(self->cur->remaining());
}

static PyGetSetDef dpy_Cursor_getsetters[] = {
    {"remaining", (getter)dpy_Cursor_remaining, nullptr, "number of results still to be returned", nullptr },
    {nullptr}
};

static PyObject* dpy_Cursor_query_attrs(dpy_Cursor* self, PyObject* args, PyObject* kw)
{
    if (self->cur == nullptr)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot access a cursor after the with block where it is used");
        return nullptr;
    }
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Cursor.attr_query, DB.attr_query_station or DB.attr_query_data instead of Cursor.query_attrs", 1))
        return NULL;

    static const char* kwlist[] = { "attrs", NULL };
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &attrs))
        return NULL;

    // Read the attribute list, if provided
    db::AttrList codes;
    if (db_read_attrlist(attrs, codes))
        return NULL;

    py_unique_ptr<dpy_Record> rec(record_create());
    try {
        if (auto c = dynamic_cast<const db::CursorStationData*>(self->cur))
            c->get_transaction()->attr_query_station(c->attr_reference_id(), [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                rec->rec->set(move(var));
            });
        else if (auto c = dynamic_cast<const db::CursorData*>(self->cur))
            c->get_transaction()->attr_query_data(c->attr_reference_id(), [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                rec->rec->set(move(var));
            });
        else
        {
            PyErr_SetString(PyExc_ValueError, "the cursor does ont come from DB.query_station_data or DB.query_data");
            return NULL;
        }
        return (PyObject*)rec.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Cursor_attr_query(dpy_Cursor* self)
{
    if (self->cur == nullptr)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot access a cursor after the with block where it is used");
        return nullptr;
    }
    py_unique_ptr<dpy_Record> rec(record_create());
    try {
        if (auto c = dynamic_cast<const db::CursorStationData*>(self->cur))
            c->get_transaction()->attr_query_station(c->attr_reference_id(), [&](unique_ptr<Var>&& var) {
                rec->rec->set(move(var));
            });
        else if (auto c = dynamic_cast<const db::CursorData*>(self->cur))
            c->get_transaction()->attr_query_data(c->attr_reference_id(), [&](unique_ptr<Var>&& var) {
                rec->rec->set(move(var));
            });
        else
        {
            PyErr_SetString(PyExc_ValueError, "the cursor does ont come from DB.query_station_data or DB.query_data");
            return NULL;
        }
        return (PyObject*)rec.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Cursor_enter(dpy_Cursor* self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

static PyObject* dpy_Cursor_exit(dpy_Cursor* self, PyObject* args)
{
    PyObject* exc_type;
    PyObject* exc_val;
    PyObject* exc_tb;
    if (!PyArg_ParseTuple(args, "OOO", &exc_type, &exc_val, &exc_tb))
        return nullptr;

    try {
        ReleaseGIL gil;
        delete self->cur;
        self->cur = nullptr;
    } DBALLE_CATCH_RETURN_PYO

    Py_RETURN_NONE;
}

static PyMethodDef dpy_Cursor_methods[] = {
    {"query_attrs",      (PyCFunction)dpy_Cursor_query_attrs, METH_VARARGS | METH_KEYWORDS,
        "Query attributes for the current variable" },
    {"attr_query",       (PyCFunction)dpy_Cursor_attr_query, METH_NOARGS,
        "Query attributes for the current variable" },
    {"__enter__",         (PyCFunction)dpy_Cursor_enter, METH_NOARGS, "Context manager __enter__" },
    {"__exit__",          (PyCFunction)dpy_Cursor_exit, METH_VARARGS, "Context manager __exit__" },
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
    Py_TYPE(self)->tp_free(self);
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
        try {
            self->cur->to_record(*self->rec->rec);
            Py_INCREF(self->rec);
            return (PyObject*)self->rec;
        } catch (wreport::error& e) {
            return raise_wreport_exception(e);
        } catch (std::exception& se) {
            return raise_std_exception(se);
        }
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

static PyObject* dpy_Cursor_str(dpy_Cursor* self)
{
    /*
    std::string f = self->var.format("None");
    return PyUnicode_FromString(f.c_str());
    */
    return PyUnicode_FromString("Cursor");
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
    return PyUnicode_FromString(res.c_str());
    */
    return PyUnicode_FromString("Cursor object");
}

PyTypeObject dpy_Cursor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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

dpy_Cursor* cursor_create(std::unique_ptr<db::Cursor> cur)
{
    dpy_Cursor* result = PyObject_New(dpy_Cursor, &dpy_Cursor_Type);
    if (!result) return NULL;
    result->cur = cur.release();
    result->rec = record_create();
    return result;
}

int register_cursor(PyObject* m)
{
    if (common_init() != 0) return -1;

    dpy_Cursor_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Cursor_Type) < 0)
        return -1;

    Py_INCREF(&dpy_Cursor_Type);
    if (PyModule_AddObject(m, "Cursor", (PyObject*)&dpy_Cursor_Type) != 0) return -1;

    return 0;
}

}
}
