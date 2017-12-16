#include <Python.h>
#include "dballe/types.h"
#include "common.h"
#include "types.h"
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

extern "C" {
PyTypeObject dpy_Level_Type;
PyTypeObject dpy_Trange_Type;
}

namespace {

PyStructSequence_Field dpy_level_fields[] = {
    { "ltype1", "Type of the level or the first layer" },
    { "l1", "Value of the level the first layer" },
    { "ltype2", "Type of the second layer" },
    { "l2", "Value of the second layer" },
    nullptr,
};

PyStructSequence_Desc dpy_level_desc = {
    "Level",
    "DB-All.e level or layer",
    dpy_level_fields,
    4,
};

PyStructSequence_Field dpy_trange_fields[] = {
    { "pind", "Time range type indicator" },
    { "p1", "Time range P1 indicator" },
    { "p2", "Time range P2 indicator" },
    nullptr,
};

PyStructSequence_Desc dpy_trange_desc = {
    "Trange",
    "DB-All.e time range",
    dpy_trange_fields,
    3,
};

/// Convert an integer to Python, returning None if it is MISSING_INT
PyObject* dballe_int_to_python(int val)
{
    if (val == MISSING_INT)
    {
        Py_INCREF(Py_None);
        return Py_None;
    } else
        return PyInt_FromLong(val);
}

/// Convert a Python object to an integer, returning MISSING_INT if it is None
int dballe_int_from_python(PyObject* o, int& out)
{
    if (o == NULL || o == Py_None)
    {
        out = MISSING_INT;
        return 0;
    }

    int res = PyInt_AsLong(o);
    if (res == -1 && PyErr_Occurred())
        return -1;

    out = res;
    return 0;
}

}


namespace dballe {
namespace python {

PyObject* level_to_python(const Level& lev)
{
    if (lev.is_missing())
        Py_RETURN_NONE;

    pyo_unique_ptr res(PyStructSequence_New(&dpy_Level_Type));
    if (!res) return nullptr;

    if (PyObject* v = dballe_int_to_python(lev.ltype1))
        PyStructSequence_SET_ITEM((PyObject*)res, 0, v);
    else
        return nullptr;

    if (PyObject* v = dballe_int_to_python(lev.l1))
        PyStructSequence_SET_ITEM((PyObject*)res, 1, v);
    else
        return nullptr;

    if (PyObject* v = dballe_int_to_python(lev.ltype2))
        PyStructSequence_SET_ITEM((PyObject*)res, 2, v);
    else
        return nullptr;

    if (PyObject* v = dballe_int_to_python(lev.l2))
        PyStructSequence_SET_ITEM((PyObject*)res, 3, v);
    else
        return nullptr;

    return res.release();
}

int level_from_python(PyObject* o, Level& out)
{
    if (o == NULL || o == Py_None)
    {
        out = Level();
        return 0;
    }

    if (Py_TYPE(o) == &dpy_Level_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_Level_Type))
    {
        Level res;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 0), res.ltype1)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 1), res.l1)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 2), res.ltype2)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 3), res.l2)) return err;
        out = res;
        return 0;
    }
    else if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size > 4)
        {
            PyErr_SetString(PyExc_TypeError, "level tuple must have at most 4 elements");
            return -1;
        }

        Level res;
        if (size < 1) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 0), res.ltype1)) return err;
        if (size < 2) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 1), res.l1)) return err;
        if (size < 3) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 2), res.ltype2)) return err;
        if (size < 4) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 3), res.l2)) return err;
        out = res;
        return 0;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "level must be None, a tuple or a Level structseq");
        return -1;
    }
}

PyObject* trange_to_python(const Trange& tr)
{
    if (tr.is_missing())
        Py_RETURN_NONE;

    pyo_unique_ptr res(PyStructSequence_New(&dpy_Trange_Type));
    if (!res) return nullptr;

    if (PyObject* v = dballe_int_to_python(tr.pind))
        PyStructSequence_SET_ITEM((PyObject*)res, 0, v);
    else
        return nullptr;

    if (PyObject* v = dballe_int_to_python(tr.p1))
        PyStructSequence_SET_ITEM((PyObject*)res, 1, v);
    else
        return nullptr;

    if (PyObject* v = dballe_int_to_python(tr.p2))
        PyStructSequence_SET_ITEM((PyObject*)res, 2, v);
    else
        return nullptr;

    return res.release();
}

int trange_from_python(PyObject* o, Trange& out)
{
    if (o == NULL || o == Py_None)
    {
        out = Trange();
        return 0;
    }

    if (Py_TYPE(o) == &dpy_Trange_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_Trange_Type))
    {
        Trange res;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 0), res.pind)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 1), res.p1)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 2), res.p2)) return err;
        out = res;
        return 0;
    }
    else if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size > 3)
        {
            PyErr_SetString(PyExc_TypeError, "time range tuple must have at most 3 elements");
            return -1;
        }

        Trange res;
        if (size < 1) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 0), res.pind)) return err;
        if (size < 2) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 1), res.p1)) return err;
        if (size < 3) { out = res; return 0; }

        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 2), res.p2)) return err;
        out = res;
        return 0;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "time range must be None, a tuple or a Trange structseq");
        return -1;
    }
}


void register_types(PyObject* m)
{
    common_init();

    PyStructSequence_InitType(&dpy_Level_Type, &dpy_level_desc);
    PyStructSequence_InitType(&dpy_Trange_Type, &dpy_trange_desc);

    PyModule_AddObject(m, "Level", (PyObject*)&dpy_Level_Type);
    PyModule_AddObject(m, "Trange", (PyObject*)&dpy_Trange_Type);
}

}
}
