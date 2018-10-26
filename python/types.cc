#include <Python.h>
#include "dballe/types.h"
#include "dballe/core/var.h"
#include "dballe/core/values.h"
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
PyTypeObject dpy_Station_Type;
PyTypeObject dpy_DBStation_Type;
}

namespace {

#if PY_MAJOR_VERSION >= 3
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

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

PyStructSequence_Field dpy_station_fields[] = {
    { "report", "rep_memo for this station" },
    { "lat", "Station latitude" },
    { "lon", "Station longitude" },
    { "ident", "Mobile station identifier" },
    nullptr,
};

PyStructSequence_Desc dpy_station_desc = {
    "Station",
    "DB-All.e station",
    dpy_station_fields,
    4,
};

PyStructSequence_Field dpy_dbstation_fields[] = {
    { "report", "rep_memo for this station" },
    { "id", "Database ID of the station" },
    { "lat", "Station latitude" },
    { "lon", "Station longitude" },
    { "ident", "Mobile station identifier" },
    nullptr,
};

PyStructSequence_Desc dpy_dbstation_desc = {
    "DBStation",
    "DB-All.e DB station",
    dpy_dbstation_fields,
    5,
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif

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

/// Convert a Python object to a double
int double_from_python(PyObject* o, double& out)
{
    double res = PyFloat_AsDouble(o);
    if (res == -1.0 && PyErr_Occurred())
        return -1;
    out = res;
    return 0;
}

}


namespace dballe {
namespace python {

PyObject* coords_to_python(const Coords& coords)
{
    if (coords.is_missing())
        Py_RETURN_NONE;

    pyo_unique_ptr res(PyTuple_New(2));
    if (!res) return nullptr;

    pyo_unique_ptr lat(PyFloat_FromDouble(coords.dlat()));
    if (!lat) return nullptr;

    pyo_unique_ptr lon(PyFloat_FromDouble(coords.dlon()));
    if (!lat) return nullptr;

    if (PyTuple_SetItem(res, 0, lat.release()) != 0)
        return nullptr;

    if (PyTuple_SetItem(res, 1, lon.release()) != 0)
        return nullptr;

    return res.release();
}

PyObject* ident_to_python(const Ident& ident)
{
    if (ident.is_missing())
        Py_RETURN_NONE;
    return PyUnicode_FromString(ident.get());
}

PyObject* level_to_python(const Level& lev)
{
    if (lev.is_missing())
        Py_RETURN_NONE;

#if PY_MAJOR_VERSION >= 3
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
#else
    pyo_unique_ptr res(PyTuple_New(4));
    if (!res) return NULL;

    if (PyObject* v = dballe_int_to_python(lev.ltype1))
        PyTuple_SET_ITEM((PyObject*)res, 0, v);
    else {
        Py_DECREF(res);
        return NULL;
    }

    if (PyObject* v = dballe_int_to_python(lev.l1))
        PyTuple_SET_ITEM((PyObject*)res, 1, v);
    else {
        Py_DECREF(res);
        return NULL;
    }

    if (PyObject* v = dballe_int_to_python(lev.ltype2))
        PyTuple_SET_ITEM((PyObject*)res, 2, v);
    else {
        Py_DECREF(res);
        return NULL;
    }

    if (PyObject* v = dballe_int_to_python(lev.l2))
        PyTuple_SET_ITEM((PyObject*)res, 3, v);
    else {
        Py_DECREF(res);
        return NULL;
    }
#endif

    return res.release();
}

int level_from_python(PyObject* o, Level& out)
{
    if (o == NULL || o == Py_None)
    {
        out = Level();
        return 0;
    }

#if PY_MAJOR_VERSION >= 3
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
    else
#endif
        if (PyTuple_Check(o))
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

#if PY_MAJOR_VERSION >= 3
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
#else
    pyo_unique_ptr res(PyTuple_New(3));
    if (!res) return NULL;

    if (PyObject* v = dballe_int_to_python(tr.pind))
        PyTuple_SET_ITEM((PyObject*)res, 0, v);
    else {
        Py_DECREF(res);
        return NULL;
    }

    if (PyObject* v = dballe_int_to_python(tr.p1))
        PyTuple_SET_ITEM((PyObject*)res, 1, v);
    else {
        Py_DECREF(res);
        return NULL;
    }

    if (PyObject* v = dballe_int_to_python(tr.p2))
        PyTuple_SET_ITEM((PyObject*)res, 2, v);
    else {
        Py_DECREF(res);
        return NULL;
    }
#endif

    return res.release();
}

int trange_from_python(PyObject* o, Trange& out)
{
    if (o == NULL || o == Py_None)
    {
        out = Trange();
        return 0;
    }

#if PY_MAJOR_VERSION >= 3
    if (Py_TYPE(o) == &dpy_Trange_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_Trange_Type))
    {
        Trange res;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 0), res.pind)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 1), res.p1)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 2), res.p2)) return err;
        out = res;
        return 0;
    }
    else
#endif
        if (PyTuple_Check(o))
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

PyObject* station_to_python(const Station& st)
{
#if PY_MAJOR_VERSION >= 3
    pyo_unique_ptr res(PyStructSequence_New(&dpy_Station_Type));
    if (!res) return nullptr;

    if (PyObject* v = string_to_python(st.report))
        PyStructSequence_SET_ITEM((PyObject*)res, 0, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlat()))
        PyStructSequence_SET_ITEM((PyObject*)res, 1, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlon()))
        PyStructSequence_SET_ITEM((PyObject*)res, 2, v);
    else
        return nullptr;

    pyo_unique_ptr ident(ident_to_python(st.ident));
    if (!ident)
        return nullptr;
    PyStructSequence_SET_ITEM((PyObject*)res, 3, ident.release());
#else
    pyo_unique_ptr res(PyTuple_New(4));
    if (!res) return NULL;

    if (PyObject* v = string_to_python(st.report))
        PyTuple_SET_ITEM((PyObject*)res, 0, v);
    else
        return NULL;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlat()))
        PyTuple_SET_ITEM((PyObject*)res, 1, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlon()))
        PyTuple_SET_ITEM((PyObject*)res, 2, v);
    else
        return nullptr;

    pyo_unique_ptr ident(ident_to_python(st.ident));
    if (!ident)
        return nullptr;
    PyTuple_SET_ITEM((PyObject*)res, 3, ident.release());
#endif

    return res.release();
}

int station_from_python(PyObject* o, Station& out)
{
#if PY_MAJOR_VERSION >= 3
    if (Py_TYPE(o) == &dpy_Station_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_Station_Type))
    {
        Station res;
        if (int err = string_from_python(PyStructSequence_GET_ITEM(o, 0), res.report)) return err;

        double dlat, dlon;
        if (int err = double_from_python(PyStructSequence_GET_ITEM(o, 1), dlat)) return err;
        if (int err = double_from_python(PyStructSequence_GET_ITEM(o, 2), dlon)) return err;
        res.coords.set(dlat, dlon);

        PyObject* ident = PyStructSequence_GET_ITEM(o, 3);
        if (ident != Py_None)
        {
            // TODO: when migrating to python3 only, this can be replaced with a
            // simple call to PyUnicode_AsUTF8. Currently string_from_python is
            // only used to get version-independent string extraction
            std::string ident_val;
            if (int err = string_from_python(ident, ident_val)) return err;
            res.ident = ident_val;
        }
        out = res;
        return 0;
    } else
#endif
        if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size != 4)
        {
            PyErr_SetString(PyExc_TypeError, "station tuple must have exactly 4 elements");
            return -1;
        }

        Station res;
        if (int err = string_from_python(PyTuple_GET_ITEM(o, 0), res.report)) return err;

        double dlat, dlon;
        if (int err = double_from_python(PyTuple_GET_ITEM(o, 1), dlat)) return err;
        if (int err = double_from_python(PyTuple_GET_ITEM(o, 2), dlon)) return err;
        res.coords.set(dlat, dlon);

        PyObject* ident = PyTuple_GET_ITEM(o, 3);
        if (ident != Py_None)
        {
            // TODO: when migrating to python3 only, this can be replaced with a
            // simple call to PyUnicode_AsUTF8. Currently string_from_python is
            // only used to get version-independent string extraction
            std::string ident_val;
            if (int err = string_from_python(ident, ident_val)) return err;
            res.ident = ident_val;
        }
        out = res;
        return 0;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "station must be a 4-tuple or a Station structseq");
        return -1;
    }
}

PyObject* dbstation_to_python(const DBStation& st)
{
#if PY_MAJOR_VERSION >= 3
    pyo_unique_ptr res(PyStructSequence_New(&dpy_DBStation_Type));
    if (!res) return nullptr;

    if (PyObject* v = string_to_python(st.report))
        PyStructSequence_SET_ITEM((PyObject*)res, 0, v);
    else
        return nullptr;

    if (PyObject* v = dballe_int_to_python(st.id))
        PyStructSequence_SET_ITEM((PyObject*)res, 1, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlat()))
        PyStructSequence_SET_ITEM((PyObject*)res, 2, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlon()))
        PyStructSequence_SET_ITEM((PyObject*)res, 3, v);
    else
        return nullptr;

    if (st.ident.is_missing())
    {
        Py_INCREF(Py_None);
        PyStructSequence_SET_ITEM((PyObject*)res, 4, Py_None);
    } else if (PyObject* v = PyUnicode_FromString(st.ident.get())) {
        PyStructSequence_SET_ITEM((PyObject*)res, 4, v);
    } else
        return nullptr;
#else
    pyo_unique_ptr res(PyTuple_New(5));
    if (!res) return NULL;

    if (PyObject* v = string_to_python(st.report))
        PyTuple_SET_ITEM((PyObject*)res, 0, v);
    else
        return NULL;

    if (PyObject* v = dballe_int_to_python(st.id))
        PyTuple_SET_ITEM((PyObject*)res, 1, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlat()))
        PyTuple_SET_ITEM((PyObject*)res, 2, v);
    else
        return nullptr;

    if (PyObject* v = PyFloat_FromDouble(st.coords.dlon()))
        PyTuple_SET_ITEM((PyObject*)res, 3, v);
    else
        return nullptr;

    if (st.ident.is_missing())
    {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM((PyObject*)res, 4, Py_None);
    } else if (PyObject* v = PyUnicode_FromString(st.ident.get())) {
        PyTuple_SET_ITEM((PyObject*)res, 4, v);
    } else
        return nullptr;
#endif

    return res.release();
}

int dbstation_from_python(PyObject* o, DBStation& out)
{
#if PY_MAJOR_VERSION >= 3
    if (Py_TYPE(o) == &dpy_DBStation_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_DBStation_Type))
    {
        DBStation res;
        if (int err = string_from_python(PyStructSequence_GET_ITEM(o, 0), res.report)) return err;
        if (int err = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 1), res.id)) return err;

        double dlat, dlon;
        if (int err = double_from_python(PyStructSequence_GET_ITEM(o, 2), dlat)) return err;
        if (int err = double_from_python(PyStructSequence_GET_ITEM(o, 3), dlon)) return err;
        res.coords.set(dlat, dlon);

        PyObject* ident = PyStructSequence_GET_ITEM(o, 4);
        if (ident != Py_None)
        {
            // TODO: when migrating to python3 only, this can be replaced with a
            // simple call to PyUnicode_AsUTF8. Currently string_from_python is
            // only used to get version-independent string extraction
            std::string ident_val;
            if (int err = string_from_python(ident, ident_val)) return err;
            res.ident = ident_val;
        }
        out = res;
        return 0;
    } else
#endif
        if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size != 5)
        {
            PyErr_SetString(PyExc_TypeError, "dbstation tuple must have exactly 5 elements");
            return -1;
        }

        DBStation res;
        if (int err = string_from_python(PyTuple_GET_ITEM(o, 0), res.report)) return err;
        if (int err = dballe_int_from_python(PyTuple_GET_ITEM(o, 1), res.id)) return err;

        double dlat, dlon;
        if (int err = double_from_python(PyTuple_GET_ITEM(o, 2), dlat)) return err;
        if (int err = double_from_python(PyTuple_GET_ITEM(o, 3), dlon)) return err;
        res.coords.set(dlat, dlon);

        PyObject* ident = PyTuple_GET_ITEM(o, 4);
        if (ident != Py_None)
        {
            // TODO: when migrating to python3 only, this can be replaced with a
            // simple call to PyUnicode_AsUTF8. Currently string_from_python is
            // only used to get version-independent string extraction
            std::string ident_val;
            if (int err = string_from_python(ident, ident_val)) return err;
            res.ident = ident_val;
        }
        out = res;
        return 0;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "station must be a 5-tuple or a DBStation structseq");
        return -1;
    }
}

PyObject* varcode_to_python(wreport::Varcode code)
{
    char buf[7];
    format_code(code, buf);
    return PyUnicode_FromString(buf);
}

#if PY_MAJOR_VERSION >= 3
int varcode_from_python(PyObject* o, wreport::Varcode& code)
{
    try {
        if (PyUnicode_Check(o))
        {
            const char* v = PyUnicode_AsUTF8(o);
            if (v == nullptr) return -1;
            code = resolve_varcode(v);
            return 0;
        }
    } DBALLE_CATCH_RETURN_INT

    PyErr_SetString(PyExc_TypeError, "Expected str");
    return -1;
}
#endif


int register_types(PyObject* m)
{
    if (common_init() != 0)
        return -1;

#if PY_MAJOR_VERSION >= 3
    if (PyStructSequence_InitType2(&dpy_Level_Type, &dpy_level_desc) != 0) return -1;
    if (PyStructSequence_InitType2(&dpy_Trange_Type, &dpy_trange_desc) != 0) return -1;
    if (PyStructSequence_InitType2(&dpy_Station_Type, &dpy_station_desc) != 0) return -1;
    if (PyStructSequence_InitType2(&dpy_DBStation_Type, &dpy_dbstation_desc) != 0) return -1;

    if (PyModule_AddObject(m, "Level", (PyObject*)&dpy_Level_Type) != 0) return -1;
    if (PyModule_AddObject(m, "Trange", (PyObject*)&dpy_Trange_Type) != 0) return -1;
    if (PyModule_AddObject(m, "Station", (PyObject*)&dpy_Station_Type) != 0) return -1;
    if (PyModule_AddObject(m, "DBStation", (PyObject*)&dpy_DBStation_Type) != 0) return -1;
#endif

    return 0;
}

}
}
