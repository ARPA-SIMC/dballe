#include "cursor.h"
#include "enq.h"
#include "types.h"
#include "data.h"
#include "db.h"
#include "message.h"
#include "common.h"
#include "dballe/core/enq.h"
#include "dballe/core/data.h"
#include "dballe/db/v7/cursor.h"
#include <algorithm>
#include "utils/type.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_CursorStation_Type = nullptr;
PyTypeObject* dpy_CursorStationDB_Type = nullptr;
PyTypeObject* dpy_CursorStationData_Type = nullptr;
PyTypeObject* dpy_CursorStationDataDB_Type = nullptr;
PyTypeObject* dpy_CursorData_Type = nullptr;
PyTypeObject* dpy_CursorDataDB_Type = nullptr;
PyTypeObject* dpy_CursorSummaryDB_Type = nullptr;
PyTypeObject* dpy_CursorSummarySummary_Type = nullptr;
PyTypeObject* dpy_CursorSummaryDBSummary_Type = nullptr;
PyTypeObject* dpy_CursorMessage_Type = nullptr;
}

namespace {

template<typename Impl>
void ensure_valid_cursor(Impl* self)
{
    if (self->cur == nullptr)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot access a cursor after the with block where it was used");
        throw PythonException();
    }
}

template<typename Impl>
void ensure_valid_iterating_cursor(Impl* self)
{
    ensure_valid_cursor(self);
    if (!self->cur->has_value())
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot access values on a cursor before or after iteration");
        throw PythonException();
    }
}

template<typename Impl>
struct remaining : Getter<remaining<Impl>, Impl>
{
    constexpr static const char* name = "remaining";
    constexpr static const char* doc = "number of results still to be returned";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_cursor(self);
            return PyLong_FromLong(self->cur->remaining());
        } DBALLE_CATCH_RETURN_PYO
    }
};

void _set_query(PyObject* dict, const DBStation& station)
{
    set_dict(dict, "report", station.report);
    set_dict(dict, "lat", dballe_int_lat_to_python(station.coords.lat));
    set_dict(dict, "lon", dballe_int_lon_to_python(station.coords.lon));
    if (station.ident.is_missing())
    {
        set_dict(dict, "mobile", false);
    } else {
        set_dict(dict, "mobile", true);
        set_dict(dict, "ident", station.ident.get());
    }
}

void _set_query(PyObject* dict, dballe::impl::CursorStation& cur)
{
    _set_query(dict, cur.get_station());
}

void _set_query(PyObject* dict, dballe::impl::CursorStationData& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", level_to_python(Level()));
    set_dict(dict, "trange", trange_to_python(Trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
}

void _set_query(PyObject* dict, dballe::impl::CursorData& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", to_python(cur.get_level()));
    set_dict(dict, "trange", to_python(cur.get_trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
    set_dict(dict, "datetime", to_python(cur.get_datetime()));
}

void _set_query(PyObject* dict, dballe::impl::CursorSummary& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", to_python(cur.get_level()));
    set_dict(dict, "trange", to_python(cur.get_trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
}

void _set_query(PyObject* dict, dballe::db::summary::Cursor<dballe::Station>& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", to_python(cur.get_level()));
    set_dict(dict, "trange", to_python(cur.get_trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
}

void _set_query(PyObject* dict, dballe::db::summary::Cursor<dballe::DBStation>& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", to_python(cur.get_level()));
    set_dict(dict, "trange", to_python(cur.get_trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
}

void _set_query(PyObject* dict, dballe::CursorMessage& cur)
{
    PyErr_SetString(PyExc_NotImplementedError, "accessing .query on CursorMessage is not yet implemented");
    throw PythonException();
}

void _set_data(core::Data& data, const DBStation& station)
{
    data.station = station;
}

void _set_data(PyObject* dict, const DBStation& station)
{
    set_dict(dict, "report", station.report);
    set_dict(dict, "lat", dballe_int_lat_to_python(station.coords.lat));
    set_dict(dict, "lon", dballe_int_lon_to_python(station.coords.lon));
    if (!station.ident.is_missing())
        set_dict(dict, "ident", station.ident.get());
}

void _set_data(core::Data& data, const Var& var)
{
    data.values.set(var);
}

void _set_data(PyObject* dict, const Var& var)
{
    if (!var.isset())
    {
        PyErr_SetString(PyExc_ValueError, ".data called on an cursor referencing an unset variable");
        throw PythonException();
    }

    pyo_unique_ptr pyvalue(wreport_api.var_value_to_python(var));

    char bcode[7];
    format_bcode(var.code(), bcode);

    if (PyDict_SetItemString(dict, bcode, pyvalue))
        throw PythonException();
}

void _set_data(core::Data& data, dballe::impl::CursorStationData& cur)
{
    _set_data(data, cur.get_station());
    _set_data(data, cur.get_var());
}

void _set_data(PyObject* dict, dballe::impl::CursorStationData& cur)
{
    _set_data(dict, cur.get_station());
    _set_data(dict, cur.get_var());
}

void _set_data(core::Data& data, dballe::impl::CursorData& cur)
{
    _set_data(data, cur.get_station());
    data.datetime = cur.get_datetime();
    data.level = cur.get_level();
    data.trange = cur.get_trange();
    _set_data(data, cur.get_var());
}

void _set_data(PyObject* dict, dballe::impl::CursorData& cur)
{
    _set_data(dict, cur.get_station());
    set_dict(dict, "level", to_python(cur.get_level()));
    set_dict(dict, "trange", to_python(cur.get_trange()));
    set_dict(dict, "datetime", to_python(cur.get_datetime()));
    _set_data(dict, cur.get_var());
}


template<typename Impl>
struct query : Getter<query<Impl>, Impl>
{
    constexpr static const char* name = "query";
    constexpr static const char* doc = "return a dict with a query to select exactly the current value at this cursor";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_iterating_cursor(self);
            pyo_unique_ptr result(throw_ifnull(PyDict_New()));
            _set_query(result, *self->cur);
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct data : Getter<data<Impl>, Impl>
{
    constexpr static const char* name = "data";
    constexpr static const char* doc = "return a dballe.Data which can be used to insert into a database the current cursor value";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_iterating_cursor(self);
            dpy_Data* d;
            pyo_unique_ptr result((PyObject*)(d = throw_ifnull(python::data_create())));
            _set_data(*d->data, *self->cur);
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct data_dict : Getter<data_dict<Impl>, Impl>
{
    constexpr static const char* name = "data_dict";
    constexpr static const char* doc = "return a dict which can be used to insert into a database the current cursor value";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_iterating_cursor(self);
            pyo_unique_ptr result(throw_ifnull(PyDict_New()));
            _set_data(result, *self->cur);
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

namespace {
inline void run_attr_query(const db::CursorStationData& cur, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    cur.get_transaction()->attr_query_station(cur.attr_reference_id(), dest);
}
inline void run_attr_query(const db::CursorData& cur, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    cur.get_transaction()->attr_query_data(cur.attr_reference_id(), dest);
}
}

template<typename Impl>
struct remove : MethNoargs<remove<Impl>, Impl>
{
    constexpr static const char* name = "remove";
    constexpr static const char* summary = "Remove the data currently addressed by the cursor";
    static PyObject* run(Impl* self)
    {
        try {
            ensure_valid_iterating_cursor(self);
            ReleaseGIL gil;
            self->cur->remove();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct query_attrs : MethNoargs<query_attrs<Impl>, Impl>
{
    constexpr static const char* name = "query_attrs";
    constexpr static const char* returns = "Dict[str, Any]";
    constexpr static const char* summary = "Query attributes for the current variable";
    static PyObject* run(Impl* self)
    {
        try {
            ensure_valid_iterating_cursor(self);
            pyo_unique_ptr res(throw_ifnull(PyDict_New()));
            run_attr_query(*self->cur, [&](unique_ptr<Var>&& var) {
                add_var_to_dict(res, *var);
            });
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct insert_attrs : MethKwargs<insert_attrs<Impl>, Impl>
{
    constexpr static const char* name = "insert_attrs";
    constexpr static const char* signature = "attrs: Dict[str, Any]";
    constexpr static const char* summary = "Insert or update attributes for the current variable";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_iterating_cursor(self);

            static const char* kwlist[] = { "attrs", NULL };
            PyObject* attrs;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &attrs))
                return nullptr;

            Values values = values_from_python(attrs);
            ReleaseGIL gil;
            self->cur->insert_attrs(values);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct remove_attrs : MethKwargs<remove_attrs<Impl>, Impl>
{
    constexpr static const char* name = "remove_attrs";
    constexpr static const char* signature = "attrs: Iterable[str]";
    constexpr static const char* summary = "Remove attributes from the current variable";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_iterating_cursor(self);

            static const char* kwlist[] = { "attrs", NULL };
            PyObject* attrs = nullptr;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &attrs))
                return nullptr;

            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->cur->remove_attrs(codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct enqi : MethKwargs<enqi<Impl>, Impl>
{
    constexpr static const char* name = "enqi";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[int, None]";
    constexpr static const char* summary = "Return the integer value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_iterating_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            impl::Enqi enq(key, len);
            self->cur->enq(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyLong_FromLong(enq.res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct enqd : MethKwargs<enqd<Impl>, Impl>
{
    constexpr static const char* name = "enqd";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[float, None]";
    constexpr static const char* summary = "Return the float value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_iterating_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            impl::Enqd enq(key, len);
            self->cur->enq(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyFloat_FromDouble(enq.res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct enqs : MethKwargs<enqs<Impl>, Impl>
{
    constexpr static const char* name = "enqs";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[str, None]";
    constexpr static const char* summary = "Return the string value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_iterating_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            Enqs enq(key, len);
            self->cur->enq(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyUnicode_FromStringAndSize(enq.res.data(), enq.res.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct enqf : MethKwargs<enqf<Impl>, Impl>
{
    constexpr static const char* name = "enqf";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[str, None]";
    constexpr static const char* summary = "Return the formatted string value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_iterating_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            Enqf enq(key, len);
            self->cur->enq(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyUnicode_FromStringAndSize(enq.res.data(), enq.res.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct __exit__ : MethVarargs<__exit__<Impl>, Impl>
{
    constexpr static const char* name = "__exit__";
    constexpr static const char* doc = "Context manager __exit__";
    static PyObject* run(Impl* self, PyObject* args)
    {
        PyObject* exc_type;
        PyObject* exc_val;
        PyObject* exc_tb;
        if (!PyArg_ParseTuple(args, "OOO", &exc_type, &exc_val, &exc_tb))
            return nullptr;

        try {
            ReleaseGIL gil;
            self->cur.reset();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


template<typename Definition, typename Impl>
struct DefinitionBase : public Type<Definition, Impl>
{
    constexpr static const char* doc = "TODO";

    static void _dealloc(Impl* self)
    {
        self->cur.~shared_ptr();
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _iter(Impl* self)
    {
        try {
            ensure_valid_cursor(self);
            Py_INCREF(self);
            return (PyObject*)self;
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _iternext(Impl* self)
    {
        try {
            ensure_valid_cursor(self);
            if (self->cur->next())
            {
                Py_INCREF(self);
                return (PyObject*)self;
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* mp_subscript(Impl* self, PyObject* pykey)
    {
        try {
            ensure_valid_iterating_cursor(self);
            Py_ssize_t len;
            const char* key = throw_ifnull(PyUnicode_AsUTF8AndSize(pykey, &len));
            // return enqpy(*self->cur, key, len);
            Enqpy enq(key, len);
            self->cur->enq(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return enq.res;
        } DBALLE_CATCH_RETURN_PYO
    }
};


struct DefinitionStation : public DefinitionBase<DefinitionStation, dpy_CursorStation>
{
    constexpr static const char* name = "CursorStation";
    constexpr static const char* qual_name = "dballe.CursorStation";
    constexpr static const char* summary = "Cursor iterating generic query_station results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_stations`` operation performed
outside a database, like :func:`dballe.Message.query_stations`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_station`.

For example::

    with msg.query_stations(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";


    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionStationDB : public DefinitionBase<DefinitionStationDB, dpy_CursorStationDB>
{
    constexpr static const char* name = "CursorStationDB";
    constexpr static const char* qual_name = "dballe.CursorStationDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_station results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_stations`` operation performed
in a database, like :func:`dballe.Transaction.query_stations`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_station`.

For example::

    with tr.query_stations(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionStationData : public DefinitionBase<DefinitionStationData, dpy_CursorStationData>
{
    constexpr static const char* name = "CursorStationData";
    constexpr static const char* qual_name = "dballe.CursorStationData";
    constexpr static const char* summary = "cursor iterating generic query_station_data results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_station_data`` operation performed
outside a database, like :func:`dballe.Message.query_station_data`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_stationdata`.

For example::

    with msg.query_station_data(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>, data<Impl>, data_dict<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionStationDataDB : public DefinitionBase<DefinitionStationDataDB, dpy_CursorStationDataDB>
{
    constexpr static const char* name = "CursorStationDataDB";
    constexpr static const char* qual_name = "dballe.CursorStationDataDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_station_data results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_station_data`` operation performed
in a database, like :func:`dballe.Transaction.query_station_data`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_stationdata`.

For example::

    with tr.query_station_data(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>, data<Impl>, data_dict<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, query_attrs<Impl>, insert_attrs<Impl>, remove_attrs<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionData : public DefinitionBase<DefinitionData, dpy_CursorData>
{
    constexpr static const char* name = "CursorData";
    constexpr static const char* qual_name = "dballe.CursorData";
    constexpr static const char* summary = "cursor iterating generic query_data results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_data`` operation performed
outside a database, like :func:`dballe.Message.query_data`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_data`.

For example::

    with msg.query_data(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>, data<Impl>, data_dict<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionDataDB : public DefinitionBase<DefinitionDataDB, dpy_CursorDataDB>
{
    constexpr static const char* name = "CursorDataDB";
    constexpr static const char* qual_name = "dballe.CursorDataDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_data results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_data`` operation performed
in a database, like :func:`dballe.Transaction.query_data`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_data`.

For example::

    with tr.query_data(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>, data<Impl>, data_dict<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, query_attrs<Impl>, insert_attrs<Impl>, remove_attrs<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummaryDB : public DefinitionBase<DefinitionSummaryDB, dpy_CursorSummaryDB>
{
    constexpr static const char* name = "CursorSummaryDB";
    constexpr static const char* qual_name = "dballe.CursorSummaryDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_summary results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_summary`` operation performed
in a database, like :func:`dballe.Transaction.query_summary`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_summary`.

For example::

    with tr.query_summary(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummarySummary : public DefinitionBase<DefinitionSummarySummary, dpy_CursorSummarySummary>
{
    constexpr static const char* name = "CursorSummarySummary";
    constexpr static const char* qual_name = "dballe.CursorSummarySummary";
    constexpr static const char* summary = "cursor iterating dballe.Explorer query_summary* results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a :class:`dballe.Explorer` ``query_*``.
like :func:`dballe.Explorer.query_summary` or :func:`dballe.Explorer.query_summary_all`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_summary`.

For example::

    with explorer.query_summary_all(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummaryDBSummary : public DefinitionBase<DefinitionSummaryDBSummary, dpy_CursorSummaryDBSummary>
{
    constexpr static const char* name = "CursorSummaryDBSummary";
    constexpr static const char* qual_name = "dballe.CursorSummaryDBSummary";
    constexpr static const char* summary = "cursor iterating dballe.DBExplorer query_summary* results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a :class:`dballe.DBExplorer` ``query_*``.
like :func:`dballe.DBExplorer.query_summary` or :func:`dballe.DBExplorer.query_summary_all`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the using dict-like access or the various `enq*`
functions. For the keys available, see :ref:`parms_read_summary`.

For example::

    with explorer.query_summary_all(...) as cur:
        for row in cur:
            print("Station:", cur["station"])
)";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


template<typename Impl>
struct message : Getter<message<Impl>, Impl>
{
    constexpr static const char* name = "message";
    constexpr static const char* doc = "dballe.Message object with the current message";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_iterating_cursor(self);
            return (PyObject*)message_create(self->cur->get_message());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct DefinitionMessage : public DefinitionBase<DefinitionMessage, dpy_CursorMessage>
{
    constexpr static const char* name = "CursorMessage";
    constexpr static const char* qual_name = "dballe.CursorMessage";
    constexpr static const char* summary = "cursor iterating query_message results";
    constexpr static const char* doc = R"(
This cursor is the iterable result of a ``query_messages`` operation, like
:func:`dballe.Transaction.query_messages`.

Each iteration returns the cursor itself, that can be used to access the
current values.

Data is read from the cursor using the ``message`` property. The dict-like
access and the various `enq*` functions are present for uniformity with other
cursors, but there are currently no valid keys that can be used.

For example::

    exporter = dballe.Exporter("BUFR")
    with open("file.bufr", "wb") as outfile:
        for cur in tr.query_messages(...):
            outfile.write(exporter.to_binary(cur.message))
)";

    GetSetters<remaining<Impl>, message<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


DefinitionStation*           definition_station = nullptr;
DefinitionStationDB*         definition_stationdb = nullptr;
DefinitionStationData*       definition_stationdata = nullptr;
DefinitionStationDataDB*     definition_stationdatadb = nullptr;
DefinitionData*              definition_data = nullptr;
DefinitionDataDB*            definition_datadb = nullptr;
DefinitionSummaryDB*         definition_summarydb = nullptr;
DefinitionSummarySummary*    definition_summarysummary = nullptr;
DefinitionSummaryDBSummary*  definition_summarydbsummary = nullptr;
DefinitionMessage*           definition_message = nullptr;

}

namespace dballe {
namespace python {

dpy_CursorStation* cursor_create(std::shared_ptr<impl::CursorStation> cur)
{
    py_unique_ptr<dpy_CursorStation> result(throw_ifnull(PyObject_New(dpy_CursorStation, dpy_CursorStation_Type)));
    new (&(result->cur)) std::shared_ptr<CursorStation>(cur);
    return result.release();
}

dpy_CursorStationDB* cursor_create(std::shared_ptr<db::v7::cursor::Stations> cur)
{
    py_unique_ptr<dpy_CursorStationDB> result(throw_ifnull(PyObject_New(dpy_CursorStationDB, dpy_CursorStationDB_Type)));
    new (&(result->cur)) std::shared_ptr<CursorStation>(cur);
    return result.release();
}

dpy_CursorStationData* cursor_create(std::shared_ptr<impl::CursorStationData> cur)
{
    py_unique_ptr<dpy_CursorStationData> result(throw_ifnull(PyObject_New(dpy_CursorStationData, dpy_CursorStationData_Type)));
    new (&(result->cur)) std::shared_ptr<CursorStationData>(cur);
    return result.release();
}

dpy_CursorStationDataDB* cursor_create(std::shared_ptr<db::v7::cursor::StationData> cur)
{
    py_unique_ptr<dpy_CursorStationDataDB> result(throw_ifnull(PyObject_New(dpy_CursorStationDataDB, dpy_CursorStationDataDB_Type)));
    new (&(result->cur)) std::shared_ptr<CursorStationData>(cur);
    return result.release();
}

dpy_CursorData* cursor_create(std::shared_ptr<impl::CursorData> cur)
{
    py_unique_ptr<dpy_CursorData> result(throw_ifnull(PyObject_New(dpy_CursorData, dpy_CursorData_Type)));
    new (&(result->cur)) std::shared_ptr<CursorData>(cur);
    return result.release();
}

dpy_CursorDataDB* cursor_create(std::shared_ptr<db::v7::cursor::Data> cur)
{
    py_unique_ptr<dpy_CursorDataDB> result(throw_ifnull(PyObject_New(dpy_CursorDataDB, dpy_CursorDataDB_Type)));
    new (&(result->cur)) std::shared_ptr<CursorData>(cur);
    return result.release();
}

dpy_CursorSummaryDB* cursor_create(std::shared_ptr<db::v7::cursor::Summary> cur)
{
    py_unique_ptr<dpy_CursorSummaryDB> result(throw_ifnull(PyObject_New(dpy_CursorSummaryDB, dpy_CursorSummaryDB_Type)));
    new (&(result->cur)) std::shared_ptr<CursorSummary>(cur);
    return result.release();
}

dpy_CursorSummarySummary* cursor_create(std::shared_ptr<db::summary::Cursor<Station>> cur)
{
    py_unique_ptr<dpy_CursorSummarySummary> result(throw_ifnull(PyObject_New(dpy_CursorSummarySummary, dpy_CursorSummarySummary_Type)));
    new (&(result->cur)) std::shared_ptr<CursorSummary>(cur);
    return result.release();
}

dpy_CursorSummaryDBSummary* cursor_create(std::shared_ptr<db::summary::Cursor<DBStation>> cur)
{
    py_unique_ptr<dpy_CursorSummaryDBSummary> result(throw_ifnull(PyObject_New(dpy_CursorSummaryDBSummary, dpy_CursorSummaryDBSummary_Type)));
    new (&(result->cur)) std::shared_ptr<CursorSummary>(cur);
    return result.release();
}

dpy_CursorMessage* cursor_create(std::shared_ptr<dballe::CursorMessage> cur)
{
    py_unique_ptr<dpy_CursorMessage> result(throw_ifnull(PyObject_New(dpy_CursorMessage, dpy_CursorMessage_Type)));
    new (&(result->cur)) std::shared_ptr<CursorMessage>(cur);
    return result.release();
}


void register_cursor(PyObject* m)
{
    common_init();

    definition_station = new DefinitionStation;
    definition_station->define(dpy_CursorStation_Type, m);

    definition_stationdb = new DefinitionStationDB;
    definition_stationdb->define(dpy_CursorStationDB_Type, m);

    definition_stationdata = new DefinitionStationData;
    definition_stationdata->define(dpy_CursorStationData_Type, m);

    definition_stationdatadb = new DefinitionStationDataDB;
    definition_stationdatadb->define(dpy_CursorStationDataDB_Type, m);

    definition_data = new DefinitionData;
    definition_data->define(dpy_CursorData_Type, m);

    definition_datadb = new DefinitionDataDB;
    definition_datadb->define(dpy_CursorDataDB_Type, m);

    definition_summarydb = new DefinitionSummaryDB;
    definition_summarydb->define(dpy_CursorSummaryDB_Type, m);

    definition_summarysummary = new DefinitionSummarySummary;
    definition_summarysummary->define(dpy_CursorSummarySummary_Type, m);

    definition_summarydbsummary = new DefinitionSummaryDBSummary;
    definition_summarydbsummary->define(dpy_CursorSummaryDBSummary_Type, m);

    definition_message = new DefinitionMessage;
    definition_message->define(dpy_CursorMessage_Type, m);
}

}
}
