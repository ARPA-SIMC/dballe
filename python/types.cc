#include <Python.h>
#include <datetime.h>
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
PyTypeObject* dpy_Trange_Type = nullptr;
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
    // Py_RETURN_RICHCOMPARE(a, b, op);  From 3.7
}

namespace level {

struct ltype1 : Getter<dpy_Level>
{
    constexpr static const char* name = "ltype1";
    constexpr static const char* doc = "type of the level or of the first layer";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->val.ltype1);
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
            return dballe_int_to_python(self->val.l1);
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
            return dballe_int_to_python(self->val.ltype2);
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
            return dballe_int_to_python(self->val.l2);
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
        std::string res = self->val.to_string("None");
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _repr(Impl* self)
    {
        std::string res = "dballe.Level(";
        res += self->val.to_string("None");
        res += ")";
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _iter(Impl* self)
    {
        py_unique_ptr<PyTupleObject> res((PyTupleObject*)PyTuple_New(4));
        PyTuple_SET_ITEM(res, 0, dballe_int_to_python(self->val.ltype1));
        PyTuple_SET_ITEM(res, 1, dballe_int_to_python(self->val.l1));
        PyTuple_SET_ITEM(res, 2, dballe_int_to_python(self->val.ltype2));
        PyTuple_SET_ITEM(res, 3, dballe_int_to_python(self->val.l2));
        return PyObject_GetIter((PyObject*)res.get());
    }

    static void _dealloc(Impl* self)
    {
        self->val.~Level();
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

        try {
            new (&(self->val)) Level(
                dballe_int_from_python(py_ltype1),
                dballe_int_from_python(py_l1),
                dballe_int_from_python(py_ltype2),
                dballe_int_from_python(py_l2)
            );
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    static PyObject* _richcompare(dpy_Level *a, PyObject *b, int op)
    {
        try {
            Level lev_b = level_from_python(b);
            return impl_richcompare(a->val, lev_b, op);
        } DBALLE_CATCH_RETURN_PYO
    }

    static Py_hash_t _hash(dpy_Level* self)
    {
        return std::hash<dballe::Level>{}(self->val);
    }
};

Definition* definition = nullptr;

}

namespace trange {

struct pind : Getter<dpy_Trange>
{
    constexpr static const char* name = "pind";
    constexpr static const char* doc = "Time range type indicator";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->val.pind);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct p1 : Getter<dpy_Trange>
{
    constexpr static const char* name = "p1";
    constexpr static const char* doc = "Time range P1 indicator";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->val.p1);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct p2 : Getter<dpy_Trange>
{
    constexpr static const char* name = "p2";
    constexpr static const char* doc = "Time range P2 indicator";

    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->val.p2);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Definition : public Binding<Definition, dpy_Trange>
{
    constexpr static const char* name = "Trange";
    constexpr static const char* qual_name = "dballe.Trange";
    constexpr static const char* doc = "Time range";

    GetSetters<pind, p1, p2> getsetters;
    Methods<> methods;

    static PyObject* _str(Impl* self)
    {
        std::string res = self->val.to_string("None");
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _repr(Impl* self)
    {
        std::string res = "dballe.Trange(";
        res += self->val.to_string("None");
        res += ")";
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _iter(Impl* self)
    {
        py_unique_ptr<PyTupleObject> res((PyTupleObject*)PyTuple_New(3));
        PyTuple_SET_ITEM(res, 0, dballe_int_to_python(self->val.pind));
        PyTuple_SET_ITEM(res, 1, dballe_int_to_python(self->val.p1));
        PyTuple_SET_ITEM(res, 2, dballe_int_to_python(self->val.p2));
        return PyObject_GetIter((PyObject*)res.get());
    }

    static void _dealloc(Impl* self)
    {
        self->val.~Trange();
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "pind", "p1", "p2", nullptr };
        PyObject* py_pind = nullptr;
        PyObject* py_p1 = nullptr;
        PyObject* py_p2 = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|OOO", const_cast<char**>(kwlist), &py_pind, &py_p1, &py_p2))
            return -1;

        try {
            new (&(self->val)) Trange(
                dballe_int_from_python(py_pind),
                dballe_int_from_python(py_p1),
                dballe_int_from_python(py_p2)
            );
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    static PyObject* _richcompare(dpy_Trange *a, PyObject *b, int op)
    {
        try {
            Trange lev_b = trange_from_python(b);
            return impl_richcompare(a->val, lev_b, op);
        } DBALLE_CATCH_RETURN_PYO
    }

    static Py_hash_t _hash(dpy_Trange* self)
    {
        return std::hash<dballe::Trange>{}(self->val);
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

}


namespace dballe {
namespace python {

PyObject* datetime_to_python(const Datetime& dt)
{
    if (dt.is_missing())
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return PyDateTime_FromDateAndTime(
            dt.year, dt.month,  dt.day,
            dt.hour, dt.minute, dt.second, 0);
}

Datetime datetime_from_python(PyObject* dt)
{
    if (dt == NULL || dt == Py_None)
        return Datetime();

    if (!PyDateTime_Check(dt))
    {
        PyErr_SetString(PyExc_TypeError, "value must be an instance of datetime.datetime");
        throw PythonException();
    }

    return Datetime(
        PyDateTime_GET_YEAR((PyDateTime_DateTime*)dt),
        PyDateTime_GET_MONTH((PyDateTime_DateTime*)dt),
        PyDateTime_GET_DAY((PyDateTime_DateTime*)dt),
        PyDateTime_DATE_GET_HOUR((PyDateTime_DateTime*)dt),
        PyDateTime_DATE_GET_MINUTE((PyDateTime_DateTime*)dt),
        PyDateTime_DATE_GET_SECOND((PyDateTime_DateTime*)dt));
}

DatetimeRange datetimerange_from_python(PyObject* val)
{
    if (PySequence_Size(val) != 2)
    {
        PyErr_SetString(PyExc_TypeError, "Expected a 2-tuple of datetime() objects");
        throw PythonException();
    }
    pyo_unique_ptr dtmin(PySequence_GetItem(val, 0));
    pyo_unique_ptr dtmax(PySequence_GetItem(val, 1));

    return DatetimeRange(datetime_from_python(dtmin), datetime_from_python(dtmax));
}

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

Ident ident_from_python(PyObject* o)
{
    if (o == Py_None)
        return Ident();

    // TODO: when migrating to python3 only, this can be replaced with a
    // simple call to PyUnicode_AsUTF8. Currently string_from_python is
    // only used to get version-independent string extraction
    return Ident(string_from_python(o));
}

PyObject* level_to_python(const Level& lev)
{
    if (lev.is_missing())
        Py_RETURN_NONE;

    py_unique_ptr<dpy_Level> res = PyObject_New(dpy_Level, dpy_Level_Type);
    if (!res) return nullptr;
    new (&(res->val)) Level(lev);
    return (PyObject*)res.release();
}

Level level_from_python(PyObject* o)
{
    if (o == NULL || o == Py_None)
        return Level();

    if (Py_TYPE(o) == dpy_Level_Type || PyType_IsSubtype(Py_TYPE(o), dpy_Level_Type))
        return ((dpy_Level*)o)->val;

    if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size > 4)
        {
            PyErr_SetString(PyExc_TypeError, "level tuple must have at most 4 elements");
            throw PythonException();
        }

        Level res;
        if (size < 1) return res;

        res.ltype1 = dballe_int_from_python(PyTuple_GET_ITEM(o, 0));
        if (size < 2) return res;

        res.l1 = dballe_int_from_python(PyTuple_GET_ITEM(o, 1));
        if (size < 3) return res;

        res.ltype2 = dballe_int_from_python(PyTuple_GET_ITEM(o, 2));
        if (size < 4) return res;

        res.l2 = dballe_int_from_python(PyTuple_GET_ITEM(o, 3));
        return res;
    }

    PyErr_SetString(PyExc_TypeError, "level must be None, a tuple or a dballe.Level");
    throw PythonException();
}

PyObject* trange_to_python(const Trange& tr)
{
    if (tr.is_missing())
        Py_RETURN_NONE;

    py_unique_ptr<dpy_Trange> res = PyObject_New(dpy_Trange, dpy_Trange_Type);
    if (!res) return nullptr;
    new (&(res->val)) Trange(tr);
    return (PyObject*)res.release();
}

Trange trange_from_python(PyObject* o)
{
    if (o == NULL || o == Py_None)
        return Trange();

    if (Py_TYPE(o) == dpy_Trange_Type || PyType_IsSubtype(Py_TYPE(o), dpy_Trange_Type))
        return ((dpy_Trange*)o)->val;

    if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size > 3)
        {
            PyErr_SetString(PyExc_TypeError, "time range tuple must have at most 3 elements");
            throw PythonException();
        }

        Trange res;
        if (size < 1) return res;

        res.pind = dballe_int_from_python(PyTuple_GET_ITEM(o, 0));
        if (size < 2) return res;

        res.p1 = dballe_int_from_python(PyTuple_GET_ITEM(o, 1));
        if (size < 3) return res;

        res.p2 = dballe_int_from_python(PyTuple_GET_ITEM(o, 2));
        return res;
    }

    PyErr_SetString(PyExc_TypeError, "time range must be None, a tuple or a Trange structseq");
    throw PythonException();
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

Station station_from_python(PyObject* o)
{
#if PY_MAJOR_VERSION >= 3
    if (Py_TYPE(o) == &dpy_Station_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_Station_Type))
    {
        Station res;
        res.report = string_from_python(PyStructSequence_GET_ITEM(o, 0));
        res.coords.set(
            from_python<double>(PyStructSequence_GET_ITEM(o, 1)),
            from_python<double>(PyStructSequence_GET_ITEM(o, 2))
        );
        res.ident = ident_from_python(PyStructSequence_GET_ITEM(o, 3));
        return res;
    } else
#endif
        if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size != 4)
        {
            PyErr_SetString(PyExc_TypeError, "station tuple must have exactly 4 elements");
            throw PythonException();
        }

        Station res;
        res.report = string_from_python(PyTuple_GET_ITEM(o, 0));
        res.coords.set(
            from_python<double>(PyTuple_GET_ITEM(o, 1)),
            from_python<double>(PyTuple_GET_ITEM(o, 2))
        );
        res.ident = ident_from_python(PyTuple_GET_ITEM(o, 3));
        return res;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "station must be a 4-tuple or a Station structseq");
        throw PythonException();
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

DBStation dbstation_from_python(PyObject* o)
{
#if PY_MAJOR_VERSION >= 3
    if (Py_TYPE(o) == &dpy_DBStation_Type || PyType_IsSubtype(Py_TYPE(o), &dpy_DBStation_Type))
    {
        DBStation res;
        res.report = string_from_python(PyStructSequence_GET_ITEM(o, 0));
        res.id = dballe_int_from_python(PyStructSequence_GET_ITEM(o, 1));
        res.coords.set(
            from_python<double>(PyStructSequence_GET_ITEM(o, 2)),
            from_python<double>(PyStructSequence_GET_ITEM(o, 3))
        );
        res.ident = ident_from_python(PyStructSequence_GET_ITEM(o, 4));
        return res;
    } else
#endif
        if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size != 5)
        {
            PyErr_SetString(PyExc_TypeError, "dbstation tuple must have exactly 5 elements");
            throw PythonException();
        }

        DBStation res;
        res.report = string_from_python(PyTuple_GET_ITEM(o, 0));
        res.id = dballe_int_from_python(PyTuple_GET_ITEM(o, 1));
        res.coords.set(
            from_python<double>(PyTuple_GET_ITEM(o, 2)),
            from_python<double>(PyTuple_GET_ITEM(o, 3))
        );
        res.ident = ident_from_python(PyTuple_GET_ITEM(o, 4));
        return res;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "station must be a 5-tuple or a DBStation structseq");
        throw PythonException();
    }
}

PyObject* varcode_to_python(wreport::Varcode code)
{
    char buf[7];
    format_code(code, buf);
    return PyUnicode_FromString(buf);
}

#if PY_MAJOR_VERSION >= 3
wreport::Varcode varcode_from_python(PyObject* o)
{
    try {
        if (PyUnicode_Check(o))
        {
            const char* v = PyUnicode_AsUTF8(o);
            if (v == nullptr) throw PythonException();
            return resolve_varcode(v);
        }
    } DBALLE_CATCH_RETURN_INT

    PyErr_SetString(PyExc_TypeError, "Expected str");
    throw PythonException();
}
#endif


int register_types(PyObject* m)
{
    /*
     * PyDateTimeAPI, that is used by all the PyDate* and PyTime* macros, is
     * defined as a static variable defaulting to NULL, and it needs to be
     * initialized on each and every C file where it is used.
     *
     * Therefore, we need to have a common_init() to call from all
     * initialization functions. *sigh*
     */
    if (!PyDateTimeAPI)
        PyDateTime_IMPORT;

#if PY_MAJOR_VERSION >= 3
    level::definition = new level::Definition;
    if (!(dpy_Level_Type = level::definition->activate(m)))
        return -1;
    trange::definition = new trange::Definition;
    if (!(dpy_Trange_Type = trange::definition->activate(m)))
        return -1;
    if (PyStructSequence_InitType2(&dpy_Station_Type, &dpy_station_desc) != 0) return -1;
    if (PyStructSequence_InitType2(&dpy_DBStation_Type, &dpy_dbstation_desc) != 0) return -1;

    if (PyModule_AddObject(m, "Station", (PyObject*)&dpy_Station_Type) != 0) return -1;
    if (PyModule_AddObject(m, "DBStation", (PyObject*)&dpy_DBStation_Type) != 0) return -1;
#endif

    return 0;
}

}
}
