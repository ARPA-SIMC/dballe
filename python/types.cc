#include <Python.h>
#include <datetime.h>
#include "dballe/types.h"
#include "dballe/values.h"
#include "dballe/core/var.h"
#include "dballe/core/query.h"
#include "dballe/core/data.h"
#include "dballe/db/v7/cursor.h"
#include "common.h"
#include "types.h"
#include "cursor.h"
#include "data.h"
#include "config.h"
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;

extern "C" {
PyTypeObject* dpy_Level_Type = nullptr;
PyTypeObject* dpy_Trange_Type = nullptr;
PyTypeObject* dpy_Station_Type = nullptr;
PyTypeObject* dpy_DBStation_Type = nullptr;
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
        default: Py_RETURN_NOTIMPLEMENTED;
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
    constexpr static const char* doc = R"(
Level or layer.

Constructor: Level(ltype1: int=None, l1: int=None, ltype2: int=None, l2: int=None)
)";

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
    constexpr static const char* doc = R"(
Time range.

Constructor: Trange(pind: int=None, p1: int=None, p2: int=None)
)";

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

namespace station {

template<typename Station>
struct StationImplTraits {};

template<>
struct StationImplTraits<Station>
{
    typedef dpy_Station Impl;
};

template<>
struct StationImplTraits<DBStation>
{
    typedef dpy_DBStation Impl;
};

template<typename Station>
struct report : Getter<typename StationImplTraits<Station>::Impl>
{
    typedef typename StationImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "report";
    constexpr static const char* doc = "report for this station";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            if (self->val.report.empty())
                Py_RETURN_NONE;
            else
                return to_python(self->val.report);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct id : Getter<dpy_DBStation>
{
    constexpr static const char* name = "id";
    constexpr static const char* doc = "database ID for this station";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return dballe_int_to_python(self->val.id);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct lat : Getter<typename StationImplTraits<Station>::Impl>
{
    typedef typename StationImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "lat";
    constexpr static const char* doc = "station latitude";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            if (self->val.coords.lat == MISSING_INT)
                Py_RETURN_NONE;
            else
                return PyFloat_FromDouble(self->val.coords.dlat());
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct lon : Getter<typename StationImplTraits<Station>::Impl>
{
    typedef typename StationImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "lon";
    constexpr static const char* doc = "station longitude";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            if (self->val.coords.lon == MISSING_INT)
                Py_RETURN_NONE;
            else
                return PyFloat_FromDouble(self->val.coords.dlon());
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct ident : Getter<typename StationImplTraits<Station>::Impl>
{
    typedef typename StationImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "ident";
    constexpr static const char* doc = "mobile station identifier";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return to_python(self->val.ident);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Base, typename Station>
struct BaseDefinition : public Binding<Base, typename StationImplTraits<Station>::Impl>
{
    typedef typename StationImplTraits<Station>::Impl Impl;

    Methods<> methods;

    static PyObject* _str(Impl* self)
    {
        std::string res = self->val.to_string("None");
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static PyObject* _repr(Impl* self)
    {
        std::string res = Base::qual_name;
        res += "(";
        res += self->val.to_string("None");
        res += ")";
        return PyUnicode_FromStringAndSize(res.data(), res.size());
    }

    static void _dealloc(Impl* self)
    {
        self->val.~Station();
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            new (&(self->val)) Station(Base::from_args(args, kw));
            return 0;
        } DBALLE_CATCH_RETURN_INT
    }

    static PyObject* _richcompare(Impl *a, PyObject *b, int op)
    {
        try {
            Station st_b = from_python<Station>(b);
            return impl_richcompare(a->val, st_b, op);
        } DBALLE_CATCH_RETURN_PYO
    }

    static Py_hash_t _hash(Impl* self)
    {
        return std::hash<Station>{}(self->val);
    }
};

struct Definition : public BaseDefinition<Definition, Station>
{
    constexpr static const char* name = "Station";
    constexpr static const char* qual_name = "dballe.Station";
    constexpr static const char* doc = R"(
Station information.

Constructor: Station(report: str, lat: float, lon: float, ident: str=None)
)";
    GetSetters<report<Station>, lat<Station>, lon<Station>, ident<Station>> getsetters;
    static Station from_args(PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "report", "lat", "lon", "ident", nullptr };
        PyObject* py_report = nullptr;
        PyObject* py_lat = nullptr;
        PyObject* py_lon = nullptr;
        PyObject* py_ident = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOO|O", const_cast<char**>(kwlist), &py_report, &py_lat, &py_lon, &py_ident))
            throw PythonException();

        Station res;
        if (py_report != Py_None)
            res.report = from_python<std::string>(py_report);
        res.coords = coords_from_python(py_lat, py_lon);
        res.ident = from_python<Ident>(py_ident);
        return res;
    }
};

struct DBDefinition : public BaseDefinition<DBDefinition, DBStation>
{
    constexpr static const char* name = "DBStation";
    constexpr static const char* qual_name = "dballe.DBStation";
    constexpr static const char* doc = R"(
Station information with database ID.

Constructor: Station(report: str, id: int, lat: float, lon: float, ident: str=None)
)";
    GetSetters<report<Station>, id, lat<Station>, lon<Station>, ident<Station>> getsetters;
    static DBStation from_args(PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "report", "id", "lat", "lon", "ident", nullptr };
        PyObject* py_report = nullptr;
        PyObject* py_id = nullptr;
        PyObject* py_lat = nullptr;
        PyObject* py_lon = nullptr;
        PyObject* py_ident = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOOO|O", const_cast<char**>(kwlist), &py_report, &py_id, &py_lat, &py_lon, &py_ident))
            throw PythonException();

        DBStation res;
        if (py_report != Py_None)
            res.report = from_python<std::string>(py_report);
        res.id = dballe_int_from_python(py_id);
        res.coords = coords_from_python(py_lat, py_lon);
        res.ident = from_python<Ident>(py_ident);
        return res;
    }
};

Definition* definition = nullptr;
DBDefinition* dbdefinition = nullptr;

}

}


namespace dballe {
namespace python {

PyObject* datetime_to_python(const Datetime& dt)
{
    if (dt.is_missing())
        Py_RETURN_NONE;

    return throw_ifnull(PyDateTime_FromDateAndTime(
            dt.year, dt.month,  dt.day,
            dt.hour, dt.minute, dt.second, 0));
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
    pyo_unique_ptr dtmin(throw_ifnull(PySequence_GetItem(val, 0)));
    pyo_unique_ptr dtmax(throw_ifnull(PySequence_GetItem(val, 1)));

    return DatetimeRange(datetime_from_python(dtmin), datetime_from_python(dtmax));
}

PyObject* coords_to_python(const Coords& coords)
{
    if (coords.is_missing())
        Py_RETURN_NONE;

    pyo_unique_ptr res(throw_ifnull(PyTuple_New(2)));

    if (PyTuple_SetItem(res, 0, dballe_int_lat_to_python(coords.lat)) != 0)
        throw PythonException();

    if (PyTuple_SetItem(res, 1, dballe_int_lon_to_python(coords.lon)) != 0)
        throw PythonException();

    return res.release();
}

Coords coords_from_python(PyObject* lat, PyObject* lon)
{
    if ((!lat || lat == Py_None) && (!lon || lon == Py_None))
        return Coords();

    // TODO: check for int and Decimal
    if (lat && lat != Py_None && lon && lon != Py_None)
        return Coords(from_python<double>(lat), from_python<double>(lon));

    PyErr_SetString(PyExc_ValueError, "both latitude and longitude must be either None or set");
    throw PythonException();
}

PyObject* ident_to_python(const Ident& ident)
{
    if (ident.is_missing())
        Py_RETURN_NONE;
    return throw_ifnull(PyUnicode_FromString(ident.get()));
}

Ident ident_from_python(PyObject* o)
{
    if (!o || o == Py_None)
        return Ident();
    return Ident(throw_ifnull(PyUnicode_AsUTF8(o)));
}

PyObject* level_to_python(const Level& lev)
{
    if (lev.is_missing())
        Py_RETURN_NONE;

    py_unique_ptr<dpy_Level> res = throw_ifnull(PyObject_New(dpy_Level, dpy_Level_Type));
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

    py_unique_ptr<dpy_Trange> res = throw_ifnull(PyObject_New(dpy_Trange, dpy_Trange_Type));
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
    py_unique_ptr<dpy_Station> res = throw_ifnull(PyObject_New(dpy_Station, dpy_Station_Type));
    new (&(res->val)) Station(st);
    return (PyObject*)res.release();
}

Station station_from_python(PyObject* o)
{
    if (Py_TYPE(o) == dpy_Station_Type || PyType_IsSubtype(Py_TYPE(o), dpy_Station_Type))
        return ((dpy_Station*)o)->val;

    if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size != 4)
        {
            PyErr_SetString(PyExc_TypeError, "Station tuple must have exactly 4 elements");
            throw PythonException();
        }

        Station res;
        PyObject* py_report = PyTuple_GET_ITEM(o, 0);
        if (py_report != Py_None)
            res.report = string_from_python(py_report);
        res.coords = coords_from_python(PyTuple_GET_ITEM(o, 1), PyTuple_GET_ITEM(o, 2));
        res.ident = ident_from_python(PyTuple_GET_ITEM(o, 3));
        return res;
    }

    PyErr_SetString(PyExc_TypeError, "station must be a 4-tuple or a Station object");
    throw PythonException();
}

PyObject* dbstation_to_python(const DBStation& st)
{
    py_unique_ptr<dpy_DBStation> res = throw_ifnull(PyObject_New(dpy_DBStation, dpy_DBStation_Type));
    new (&(res->val)) DBStation(st);
    return (PyObject*)res.release();
}

DBStation dbstation_from_python(PyObject* o)
{
    if (Py_TYPE(o) == dpy_DBStation_Type || PyType_IsSubtype(Py_TYPE(o), dpy_DBStation_Type))
        return ((dpy_DBStation*)o)->val;

    if (PyTuple_Check(o))
    {
        unsigned size = PyTuple_Size(o);
        if (size != 5)
        {
            PyErr_SetString(PyExc_TypeError, "DBStation tuple must have exactly 4 elements");
            throw PythonException();
        }

        DBStation res;
        PyObject* py_report = PyTuple_GET_ITEM(o, 0);
        res.id = dballe_int_from_python(PyTuple_GET_ITEM(o, 1));
        if (py_report != Py_None)
            res.report = string_from_python(py_report);
        res.coords = coords_from_python(PyTuple_GET_ITEM(o, 2), PyTuple_GET_ITEM(o, 3));
        res.ident = ident_from_python(PyTuple_GET_ITEM(o, 4));
        return res;
    }

    PyErr_SetString(PyExc_TypeError, "station must be a 5-tuple or a DBStation object");
    throw PythonException();
}

PyObject* varcode_to_python(wreport::Varcode code)
{
    char buf[7];
    format_code(code, buf);
    return throw_ifnull(PyUnicode_FromString(buf));
}

wreport::Varcode varcode_from_python(PyObject* o)
{
    try {
        if (PyUnicode_Check(o))
            return resolve_varcode(throw_ifnull(PyUnicode_AsUTF8(o)));
    } DBALLE_CATCH_RETURN_INT

    PyErr_SetString(PyExc_TypeError, "Expected str");
    throw PythonException();
}

std::unique_ptr<Query> query_from_python(PyObject* o)
{
    core::Query* q;
    std::unique_ptr<dballe::Query> res(q = new core::Query);
    if (!o || o == Py_None)
        return res;

    if (PyDict_Check(o))
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(o, &pos, &key, &value))
        {
            std::string k = string_from_python(key);
            query_setpy(*q, k.data(), k.size(), value);
        }
        q->validate();
        return res;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}


DataPtr::DataPtr(DataPtr&& o)
    : data(o.data), owned(o.owned)
{
    o.owned = false;
}

DataPtr::~DataPtr()
{
    if (owned)
        delete data;
}

void DataPtr::create()
{
    if (data)
        throw std::runtime_error("DataPtr::create/reuse called twice");

    data = new core::Data;
    owned = true;
}

void DataPtr::reuse(core::Data* data)
{
    if (this->data)
        throw std::runtime_error("DataPtr::create/reuse called twice");

    this->data = data;
    owned = false;
}

DataPtr::DataPtr(PyObject* from_python)
{
    if (!from_python || from_python == Py_None)
        return;

    if (dpy_Data_Check(from_python))
    {
        reuse(((dpy_Data*)from_python)->data);
        return;
    }

    if (PyDict_Check(from_python))
    {
        create();
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
        {
            std::string k = string_from_python(key);
            data_setpy(*data, k.data(), k.size(), value);
        }
        return;
    }

    if (dpy_CursorStationDataDB_Check(from_python))
    {
        create();
        dpy_CursorStationDataDB* cur = (dpy_CursorStationDataDB*)from_python;
        data->station = cur->cur->get_station();
        data->station.id = MISSING_INT;
        data->values.set(*cur->cur->rows->value.get());
        return;
    }

    if (dpy_CursorDataDB_Check(from_python))
    {
        create();
        dpy_CursorDataDB* cur = (dpy_CursorDataDB*)from_python;
        data->station = cur->cur->get_station();
        data->station.id = MISSING_INT;
        data->datetime = cur->cur->get_datetime();
        data->level = cur->cur->get_level();
        data->trange = cur->cur->get_trange();
        data->values.set(*cur->cur->rows->value.get());
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected Data, dict, station data cursor, data cursor, or None");
    throw PythonException();
}

Values values_from_python(PyObject* from_python)
{
    Values values;
    if (!from_python || from_python == Py_None)
        return values;

    if (PyDict_Check(from_python))
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
            set_values_from_python(values, varcode_from_python(key), value);
        return values;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

std::string dballe_nullable_string_from_python(PyObject* o)
{
    if (!o || o == Py_None)
        return std::string();
    if (PyUnicode_Check(o))
        return throw_ifnull(PyUnicode_AsUTF8(o));
    if (PyBytes_Check(o))
        return throw_ifnull(PyBytes_AsString(o));
    PyErr_SetString(PyExc_TypeError, "report value must be an instance of str, bytes, or None");
    throw PythonException();
}

int dballe_int_lat_from_python(PyObject* o)
{
    if (!o || o == Py_None)
        return MISSING_INT;
    if (PyLong_Check(o))
    {
        int res = PyLong_AsLong(o);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        return res;
    }
    if (PyFloat_Check(o))
    {
        double res = PyFloat_AsDouble(o);
        if (res == -1.0 && PyErr_Occurred())
            throw PythonException();
        return Coords::lat_to_int(res);
    }
    if (PyUnicode_Check(o))
    {
        double res = strtod(throw_ifnull(PyUnicode_AsUTF8(o)), nullptr);
        return Coords::lat_to_int(res);
    }
    // We cannot directly test if it is a decimal, but we can try to duck type
    PyObject* scaleb = PyObject_GetAttrString(o, "scaleb");
    if (scaleb == nullptr)
    {
        PyErr_Clear();
    } else {
        pyo_unique_ptr six(throw_ifnull(PyLong_FromLong(5)));
        pyo_unique_ptr scaled(throw_ifnull(PyObject_CallFunctionObjArgs(scaleb, six.get(), nullptr)));
        int res = PyLong_AsLong(scaled);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        return res;
    }
    PyErr_SetString(PyExc_TypeError, "latitude value must be an instance of int, float, str, Decimal, or None");
    throw PythonException();
}

int dballe_int_lon_from_python(PyObject* o)
{
    if (!o || o == Py_None)
        return MISSING_INT;
    if (PyLong_Check(o))
    {
        int res = PyLong_AsLong(o);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        return res;
    }
    if (PyFloat_Check(o))
    {
        double res = PyFloat_AsDouble(o);
        if (res == -1.0 && PyErr_Occurred())
            throw PythonException();
        return Coords::lon_to_int(res);
    }
    if (PyUnicode_Check(o))
    {
        double res = strtod(throw_ifnull(PyUnicode_AsUTF8(o)), nullptr);
        return Coords::lon_to_int(res);
    }
    // We cannot directly test if it is a decimal, but we can try to duck type
    PyObject* scaleb = PyObject_GetAttrString(o, "scaleb");
    if (scaleb == nullptr)
    {
        PyErr_Clear();
    } else {
        pyo_unique_ptr six(throw_ifnull(PyLong_FromLong(5)));
        pyo_unique_ptr scaled(throw_ifnull(PyObject_CallFunctionObjArgs(scaleb, six.get(), nullptr)));
        int res = PyLong_AsLong(scaled);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        return res;
    }
    PyErr_SetString(PyExc_TypeError, "longitude value must be an instance of int, float, str, Decimal, or None");
    throw PythonException();
}

namespace {

struct Decimal
{
    PyObject* mod;
    PyObject* ctor;

    Decimal()
        : mod(throw_ifnull(PyImport_ImportModule("decimal"))),
          ctor(throw_ifnull(PyObject_GetAttrString(mod, "Decimal")))
    {
    }

    PyObject* make_lat(const char* val)
    {
        pyo_unique_ptr arg(throw_ifnull(PyUnicode_FromString(val)));
        return throw_ifnull(PyObject_CallFunctionObjArgs(ctor, arg.get(), nullptr));
    }
    PyObject* make_lat(const std::string& val)
    {
        pyo_unique_ptr arg(throw_ifnull(PyUnicode_FromStringAndSize(val.data(), val.size())));
        return throw_ifnull(PyObject_CallFunctionObjArgs(ctor, arg.get(), nullptr));
    }
    PyObject* make_lat(int val)
    {
        double dval = Coords::lat_from_int(val);
        char buf[16];
        int size = snprintf(buf, 16, "%.5f", dval);
        pyo_unique_ptr arg(throw_ifnull(PyUnicode_FromStringAndSize(buf, size)));
        return throw_ifnull(PyObject_CallFunctionObjArgs(ctor, arg.get(), nullptr));
    }

    PyObject* make_lon(const char* val)
    {
        pyo_unique_ptr arg(throw_ifnull(PyUnicode_FromString(val)));
        return throw_ifnull(PyObject_CallFunctionObjArgs(ctor, arg.get(), nullptr));
    }
    PyObject* make_lon(const std::string& val)
    {
        pyo_unique_ptr arg(throw_ifnull(PyUnicode_FromStringAndSize(val.data(), val.size())));
        return throw_ifnull(PyObject_CallFunctionObjArgs(ctor, arg.get(), nullptr));
    }
    PyObject* make_lon(int val)
    {
        double dval = Coords::lon_from_int(val);
        char buf[16];
        int size = snprintf(buf, 16, "%.5f", dval);
        pyo_unique_ptr arg(throw_ifnull(PyUnicode_FromStringAndSize(buf, size)));
        return throw_ifnull(PyObject_CallFunctionObjArgs(ctor, arg.get(), nullptr));
    }
};

Decimal* decimal = nullptr;

}


PyObject* dballe_int_lat_to_python(int lat)
{
    if (lat == MISSING_INT)
        Py_RETURN_NONE;
    if (!decimal)
        decimal = new Decimal;
    return decimal->make_lat(lat);
}

PyObject* dballe_int_lon_to_python(int lon)
{
    if (lon == MISSING_INT)
        Py_RETURN_NONE;
    if (!decimal)
        decimal = new Decimal;
    return decimal->make_lon(lon);
}

void set_lat_from_python(PyObject* o, Coords& coords)
{
    if (!o || o == Py_None)
    {
        coords = Coords();
        return;
    }

    if (PyLong_Check(o))
    {
        int res = PyLong_AsLong(o);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        coords.set_lat(res);
        return;
    }

    if (PyFloat_Check(o))
    {
        double res = PyFloat_AsDouble(o);
        if (res == -1.0 && PyErr_Occurred())
            throw PythonException();
        coords.set_lat(res);
        return;
    }

    // We cannot directly test if it is a decimal, but we can try to duck type
    PyObject* scaleb = PyObject_GetAttrString(o, "scaleb");
    if (scaleb == nullptr)
    {
        PyErr_Clear();
    } else {
        pyo_unique_ptr six(throw_ifnull(PyLong_FromLong(5)));
        pyo_unique_ptr scaled(throw_ifnull(PyObject_CallFunctionObjArgs(scaleb, six.get(), nullptr)));
        int res = PyLong_AsLong(scaled);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        coords.set_lat(res);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "latitude value must be an instance of int, float, or None");
    throw PythonException();
}

void set_lon_from_python(PyObject* o, Coords& coords)
{
    if (!o || o == Py_None)
    {
        coords = Coords();
        return;
    }

    if (PyLong_Check(o))
    {
        int res = PyLong_AsLong(o);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        coords.set_lon(res);
        return;
    }

    if (PyFloat_Check(o))
    {
        double res = PyFloat_AsDouble(o);
        if (res == -1.0 && PyErr_Occurred())
            throw PythonException();
        coords.set_lon(res);
        return;
    }

    // We cannot directly test if it is a decimal, but we can try to duck type
    PyObject* scaleb = PyObject_GetAttrString(o, "scaleb");
    if (scaleb == nullptr)
    {
        PyErr_Clear();
    } else {
        pyo_unique_ptr six(throw_ifnull(PyLong_FromLong(5)));
        pyo_unique_ptr scaled(throw_ifnull(PyObject_CallFunctionObjArgs(scaleb, six.get(), nullptr)));
        int res = PyLong_AsLong(scaled);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        coords.set_lon(res);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "longitude value must be an instance of int, float, or None");
    throw PythonException();
}

unsigned short datetime_int16_from_python(PyObject* o)
{
    if (!o || o == Py_None)
        return 0xffff;
    if (PyLong_Check(o))
    {
        int res = PyLong_AsLong(o);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        return res;
    }
    PyErr_SetString(PyExc_TypeError, "datetime value must be an instance of int, or None");
    throw PythonException();
}

unsigned char datetime_int8_from_python(PyObject* o)
{
    if (!o || o == Py_None)
        return 0xff;
    if (PyLong_Check(o))
    {
        int res = PyLong_AsLong(o);
        if (res == -1 && PyErr_Occurred())
            throw PythonException();
        return res;
    }
    PyErr_SetString(PyExc_TypeError, "datetime value must be an instance of int, or None");
    throw PythonException();
}

template<typename Values>
void set_values_from_python(Values& values, wreport::Varcode code, PyObject* val)
{
    if (!val || val == Py_None)
        values.unset(code);
    else if (PyFloat_Check(val))
    {
        double v = PyFloat_AsDouble(val);
        if (v == -1.0 && PyErr_Occurred())
            throw PythonException();
        values.set(code, v);
    } else if (PyLong_Check(val)) {
        long v = PyLong_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            throw PythonException();
        values.set(code, (int)v);
    } else if (PyUnicode_Check(val) || PyBytes_Check(val)) {
        values.set(code, string_from_python(val));
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
        throw PythonException();
    }
}

template void set_values_from_python(Values& values, wreport::Varcode code, PyObject* val);
template void set_values_from_python(DBValues& values, wreport::Varcode code, PyObject* val);

PyObject* attrs_to_python(const wreport::Var& var)
{
    pyo_unique_ptr list(PyList_New(0));
    for (const wreport::Var* a = var.next_attr(); a; a = a->next_attr())
        if (PyList_Append(list, (PyObject*)wrpy->var_create_copy(*a)) == -1)
            throw PythonException();
    return list.release();
}

void add_var_to_dict(PyObject* dict, const wreport::Var& var)
{
    char bcode[7];
    format_bcode(var.code(), bcode);
    pyo_unique_ptr pyvar((PyObject*)throw_ifnull(wrpy->var_create_copy(var)));
    if (PyDict_SetItemString(dict, bcode, pyvar))
        throw PythonException();
}

std::set<wreport::Varcode> varcodes_from_python(PyObject* o)
{
    std::set<wreport::Varcode> res;

    // Iterate, resolve, and insert into res
    pyo_unique_ptr seq(throw_ifnull(PySequence_Fast(o, "varcodes must be a sequence of strings")));
    auto size = PySequence_Fast_GET_SIZE(seq.get());
    PyObject** vals = PySequence_Fast_ITEMS(seq.get());
    for (unsigned i = 0; i < size; ++i)
        res.insert(varcode_from_python(vals[i]));
    return res;
}

bool bool_from_python(PyObject* o)
{
    int is_true = PyObject_IsTrue(o);
    if (is_true == -1)
        throw PythonException();
    return is_true == 1;
}

void set_dict(PyObject* dict, const char* key, const char* val)
{
    pyo_unique_ptr pyval(throw_ifnull(PyUnicode_FromString(val)));
    if (PyDict_SetItemString(dict, key, pyval))
        throw PythonException();
}

void set_dict(PyObject* dict, const char* key, const std::string& val)
{
    pyo_unique_ptr pyval(throw_ifnull(PyUnicode_FromStringAndSize(val.data(), val.size())));
    if (PyDict_SetItemString(dict, key, pyval))
        throw PythonException();
}

void set_dict(PyObject* dict, const char* key, bool val)
{
    PyObject* pyval = val ? Py_True : Py_False;
    if (PyDict_SetItemString(dict, key, pyval))
        throw PythonException();
}

void set_dict(PyObject* dict, const char* key, int val)
{
    pyo_unique_ptr pyval(throw_ifnull(PyLong_FromLong(val)));
    if (PyDict_SetItemString(dict, key, pyval))
        throw PythonException();
}

void set_dict(PyObject* dict, const char* key, PyObject* val)
{
    if (PyDict_SetItemString(dict, key, val))
        throw PythonException();
}

void set_dict(PyObject* dict, const char* key, pyo_unique_ptr&& val)
{
    if (PyDict_SetItemString(dict, key, val))
        throw PythonException();
}

void register_types(PyObject* m)
{
    common_init();

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

    level::definition = new level::Definition;
    dpy_Level_Type = level::definition->activate(m);

    trange::definition = new trange::Definition;
    dpy_Trange_Type = trange::definition->activate(m);

    station::definition = new station::Definition;
    dpy_Station_Type = station::definition->activate(m);

    station::dbdefinition = new station::DBDefinition;
    dpy_DBStation_Type = station::dbdefinition->activate(m);
}

}
}
