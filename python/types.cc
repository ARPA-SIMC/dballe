#include <Python.h>
#include "dballe/types.h"
#include "dballe/core/var.h"
#include "dballe/core/values.h"
#include "common.h"
#include "types.h"
#include "config.h"
#include "impl-utils.h"

#if PY_MAJOR_VERSION <= 2
    #define PyLong_AsLong PyInt_AsLong
    #define PyLong_FromLong PyInt_FromLong
#endif

using namespace std;
using namespace dballe;
using namespace dballe::python;

extern "C" {
PyTypeObject* dpy_Level_Type = nullptr;
PyTypeObject dpy_Trange_Type;
PyTypeObject dpy_Station_Type;
PyTypeObject dpy_DBStation_Type;
}

namespace {

template<typename T>
PyObject* impl_richcompare(const T& a, const T& b, int op)
{
    switch (op)
    {
        case Py_LT: if (a <  b) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        case Py_LE: if (a <= b) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        case Py_EQ: if (a == b) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        case Py_NE: if (a != b) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        case Py_GT: if (a >  b) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        case Py_GE: if (a >= b) Py_RETURN_TRUE; else Py_RETURN_FALSE;
        default:
            PyErr_SetString(PyExc_TypeError, "Unsupported comparison");
            return nullptr;
    }
    // Py_RETURN_RICHCOMPARE(a->level, lev_b, op);  From 3.7
}

namespace level {

struct ltype1 : Getter<dpy_Level>
{
    constexpr static const char* name = "ltype1";
    constexpr static const char* doc = "type of the level or of the first layer";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->level.ltype1);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct l1 : Getter<dpy_Level>
{
    constexpr static const char* name = "l1";
    constexpr static const char* doc = "value of the level or of the first layer";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->level.l1);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct ltype2 : Getter<dpy_Level>
{
    constexpr static const char* name = "ltype2";
    constexpr static const char* doc = "type of the second layer";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->level.ltype2);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct l2 : Getter<dpy_Level>
{
    constexpr static const char* name = "l2";
    constexpr static const char* doc = "value of the second layer";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->level.l2);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Definition : public Binding<Definition, dpy_Level>
{
    constexpr static const char* name = "Level";
    constexpr static const char* qual_name = "dballe.Level";
    constexpr static const char* doc = "Level or layer";

    GetSetters<ltype1, l1, ltype2, l2> getsetters;
    Methods<> methods;

    static PyObject* _str(Impl* self)
    {
        std::string res = self->level.to_string("None");
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _repr(Impl* self)
    {
        std::string res = "dballe.Level(";
        res += self->level.to_string("None");
        res += ")";
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _iter(Impl* self)
    {
        py_unique_ptr<PyTupleObject> res((PyTupleObject*)PyTuple_New(4));
        PyTuple_SET_ITEM(res, 0, dballe_int_to_python(self->level.ltype1));
        PyTuple_SET_ITEM(res, 1, dballe_int_to_python(self->level.l1));
        PyTuple_SET_ITEM(res, 2, dballe_int_to_python(self->level.ltype2));
        PyTuple_SET_ITEM(res, 3, dballe_int_to_python(self->level.l2));
        return PyObject_GetIter((PyObject*)res.get());
    }

    static void _dealloc(Impl* self)
    {
        self->level.~Level();
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "ltype1", "l1", "ltype2", "l2", nullptr };
        PyObject* py_ltype1 = nullptr;
        PyObject* py_l1 = nullptr;
        PyObject* py_ltype2 = nullptr;
        PyObject* py_l2 = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|OOOO", const_cast<char**>(kwlist), &py_ltype1, &py_l1, &py_ltype2, &py_l2))
            return -1;

        int ltype1, l1, ltype2, l2;
        if (dballe_int_from_python(py_ltype1, ltype1) != 0) return -1;
        if (dballe_int_from_python(py_l1, l1) != 0) return -1;
        if (dballe_int_from_python(py_ltype2, ltype2) != 0) return -1;
        if (dballe_int_from_python(py_l2, l2) != 0) return -1;

        try {
            new (&(self->level)) Level(ltype1, l1, ltype2, l2);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    static PyObject* _richcompare(dpy_Level *a, PyObject *b, int op)
    {
        Level lev_b;
        if (level_from_python(b, lev_b) != 0)
            return nullptr;
        return impl_richcompare(a->level, lev_b, op);
    }

    static Py_hash_t _hash(dpy_Level* self)
    {
        Py_hash_t res = 0;
        if (self->level.ltype1 != MISSING_INT) res += self->level.ltype1;
        if (self->level.l1 != MISSING_INT) res += self->level.l1;
        if (self->level.ltype2 != MISSING_INT) res += self->level.ltype2 << 8;
        if (self->level.l2 != MISSING_INT) res += self->level.l2;
        return res;
    }
};

Definition* definition = nullptr;

}

#if PY_MAJOR_VERSION >= 3
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

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

    py_unique_ptr<dpy_Level> res = PyObject_New(dpy_Level, dpy_Level_Type);
    if (!res) return nullptr;
    new (&(res->level)) Level(lev);
    return (PyObject*)res.release();
}

int level_from_python(PyObject* o, Level& out)
{
    if (o == NULL || o == Py_None)
    {
        out = Level();
        return 0;
    }

    if (Py_TYPE(o) == dpy_Level_Type || PyType_IsSubtype(Py_TYPE(o), dpy_Level_Type))
    {
        out = ((dpy_Level*)o)->level;
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
        PyErr_SetString(PyExc_TypeError, "level must be None, a tuple or a dballe.Level");
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
    level::definition = new level::Definition;
    if (!(dpy_Level_Type = level::definition->activate(m)))
        return -1;
    if (PyStructSequence_InitType2(&dpy_Trange_Type, &dpy_trange_desc) != 0) return -1;
    if (PyStructSequence_InitType2(&dpy_Station_Type, &dpy_station_desc) != 0) return -1;
    if (PyStructSequence_InitType2(&dpy_DBStation_Type, &dpy_dbstation_desc) != 0) return -1;

    if (PyModule_AddObject(m, "Trange", (PyObject*)&dpy_Trange_Type) != 0) return -1;
    if (PyModule_AddObject(m, "Station", (PyObject*)&dpy_Station_Type) != 0) return -1;
    if (PyModule_AddObject(m, "DBStation", (PyObject*)&dpy_DBStation_Type) != 0) return -1;
#endif

    return 0;
}

}
}
