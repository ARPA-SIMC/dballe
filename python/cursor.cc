#include <Python.h>
#include "cursor.h"
#include "types.h"
#include "db.h"
#include "message.h"
#include "common.h"
#include "dballe/core/enq.h"
#include "dballe/db/v7/cursor.h"
#include <algorithm>
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_CursorStationDB_Type = nullptr;
PyTypeObject* dpy_CursorStationDataDB_Type = nullptr;
PyTypeObject* dpy_CursorDataDB_Type = nullptr;
PyTypeObject* dpy_CursorSummaryDB_Type = nullptr;
PyTypeObject* dpy_CursorSummarySummary_Type = nullptr;
PyTypeObject* dpy_CursorSummaryDBSummary_Type = nullptr;
PyTypeObject* dpy_CursorMessage_Type = nullptr;
}

namespace {

struct Enqpy
{
    const char* key;
    unsigned len;
    PyObject* res = nullptr;
    bool missing = true;

    Enqpy(const char* key, unsigned len)
        : key(key), len(len)
    {
    }

    void set_bool(bool val)
    {
        res = val ? Py_True : Py_False;
        Py_INCREF(res);
        missing = false;
    }

    void set_int(int val)
    {
        res = throw_ifnull(PyLong_FromLong(val));
        missing = false;
    }

    void set_dballe_int(int val)
    {
        if (val == MISSING_INT)
        {
            res = Py_None;
            Py_INCREF(res);
        } else
            res = throw_ifnull(PyLong_FromLong(val));
        missing = false;
    }

    void set_string(const std::string& val)
    {
        res = string_to_python(val);
        missing = false;
    }

    void set_ident(const Ident& ident)
    {
        if (ident.is_missing())
        {
            res = Py_None;
            Py_INCREF(res);
        } else
            res = throw_ifnull(PyUnicode_FromString(ident.get()));
        missing = false;
    }

    void set_varcode(wreport::Varcode val)
    {
        char buf[7];
        dballe::format_bcode(val, buf);
        res = throw_ifnull(PyUnicode_FromStringAndSize(buf, 6));
        missing = false;
    }

    void set_var(const wreport::Var* val)
    {
        if (!val) return;
        res = (PyObject*)throw_ifnull(wrpy->var_create_copy(*val));
        missing = false;
    }

    void set_attrs(const wreport::Var* val)
    {
        if (!val) return;
        res = attrs_to_python(*val);
        missing = false;
    }

    void set_lat(int lat)
    {
        if (lat == MISSING_INT)
            return;
        res = dballe_int_lat_to_python(lat);
        missing = false;
    }

    void set_lon(int lon)
    {
        if (lon == MISSING_INT)
            return;
        res = dballe_int_lon_to_python(lon);
        missing = false;
    }

    template<typename Station>
    void set_coords(const Station& s)
    {
        res = coords_to_python(s.coords);
        missing = false;
    }

    template<typename Station>
    void set_station(const Station& s)
    {
        res = station_to_python(s);
        missing = false;
    }

    void set_datetime(const Datetime& dt)
    {
        res = datetime_to_python(dt);
        missing = false;
    }

    void set_level(const Level& lev)
    {
        res = level_to_python(lev);
        missing = false;
    }

    void set_trange(const Trange& tr)
    {
        res = trange_to_python(tr);
        missing = false;
    }

    template<typename Values>
    bool search_b_values(const Values& values)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        if (const wreport::Var* var = values.maybe_var(code))
        {
            res = (PyObject*)throw_ifnull(wrpy->var_create_copy(*var));
            missing = false;
        }
        return true;
    }

    bool search_b_value(const dballe::Value& value)
    {
        if (key[0] != 'B' || len != 6)
            return false;

        wreport::Varcode code = WR_STRING_TO_VAR(key + 1);
        if (code != value.code())
            wreport::error_notfound::throwf("key %s not found on this query result", key);

        if (const wreport::Var* var = value.get())
        {
            res = (PyObject*)throw_ifnull(wrpy->var_create_copy(*var));
            missing = false;
        }
        return true;
    }

    template<typename Values>
    void search_alias_values(const Values& values)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (const wreport::Var* var = values.maybe_var(code))
        {
            res = (PyObject*)throw_ifnull(wrpy->var_create_copy(*var));
            missing = false;
        }
    }

    void search_alias_value(const dballe::Value& value)
    {
        wreport::Varcode code = dballe::resolve_varcode(key);
        if (code != value.code())
            wreport::error_notfound::throwf("key %s not found on this query result", key);
        if (const wreport::Var* var = value.get())
        {
            res = (PyObject*)throw_ifnull(wrpy->var_create_copy(*var));
            missing = false;
        }
    }
};

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
struct remaining : Getter<Impl>
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

void _set_query(PyObject* dict, dballe::db::CursorStation& cur)
{
    _set_query(dict, cur.get_station());
}

void _set_query(PyObject* dict, dballe::db::CursorStationData& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", level_to_python(Level()));
    set_dict(dict, "trange", trange_to_python(Trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
}

void _set_query(PyObject* dict, dballe::db::CursorData& cur)
{
    _set_query(dict, cur.get_station());
    set_dict(dict, "level", to_python(cur.get_level()));
    set_dict(dict, "trange", to_python(cur.get_trange()));
    set_dict(dict, "var", varcode_to_python(cur.get_varcode()));
    set_dict(dict, "datetime", to_python(cur.get_datetime()));
}

void _set_query(PyObject* dict, dballe::db::CursorSummary& cur)
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



template<typename Impl>
struct query : Getter<Impl>
{
    constexpr static const char* name = "query";
    constexpr static const char* doc = "return a dict with a query to select exactly the current value at this cursor";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_cursor(self);
            pyo_unique_ptr result(throw_ifnull(PyDict_New()));
            _set_query(result, *self->cur);
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
struct remove : MethNoargs<Impl>
{
    constexpr static const char* name = "remove";
    constexpr static const char* summary = "Remove the data currently addressed by the cursor";
    static PyObject* run(Impl* self)
    {
        try {
            ensure_valid_cursor(self);
            ReleaseGIL gil;
            self->cur->remove();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct query_attrs : MethNoargs<Impl>
{
    constexpr static const char* name = "query_attrs";
    constexpr static const char* returns = "Dict[str, Any]";
    constexpr static const char* summary = "Query attributes for the current variable";
    static PyObject* run(Impl* self)
    {
        try {
            ensure_valid_cursor(self);
            pyo_unique_ptr res(throw_ifnull(PyDict_New()));
            run_attr_query(*self->cur, [&](unique_ptr<Var>&& var) {
                add_var_to_dict(res, *var);
            });
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct insert_attrs : MethKwargs<Impl>
{
    constexpr static const char* name = "insert_attrs";
    constexpr static const char* signature = "attrs: Dict[str, Any]";
    constexpr static const char* summary = "Insert or update attributes for the current variable";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

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
struct remove_attrs : MethKwargs<Impl>
{
    constexpr static const char* name = "remove_attrs";
    constexpr static const char* signature = "attrs: Iterable[str]";
    constexpr static const char* summary = "Remove attributes from the current variable";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

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
struct enqi : MethKwargs<Impl>
{
    constexpr static const char* name = "enqi";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[int, None]";
    constexpr static const char* summary = "Return the integer value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            impl::Enqi enq(key, len);
            self->cur->enq_generic(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyLong_FromLong(enq.res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct enqd : MethKwargs<Impl>
{
    constexpr static const char* name = "enqd";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[float, None]";
    constexpr static const char* summary = "Return the float value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            impl::Enqd enq(key, len);
            self->cur->enq_generic(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyFloat_FromDouble(enq.res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct enqs : MethKwargs<Impl>
{
    constexpr static const char* name = "enqs";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[str, None]";
    constexpr static const char* summary = "Return the string value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            impl::Enqs enq(key, len);
            self->cur->enq_generic(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyUnicode_FromStringAndSize(enq.res.data(), enq.res.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct enqf : MethKwargs<Impl>
{
    constexpr static const char* name = "enqf";
    constexpr static const char* signature = "key: str";
    constexpr static const char* returns = "Union[str, None]";
    constexpr static const char* summary = "Return the formatted string value for a keyword";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

            static const char* kwlist[] = { "key", nullptr };
            const char* key;
            Py_ssize_t len;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "s#", const_cast<char**>(kwlist), &key, &len))
                return nullptr;

            impl::Enqf enq(key, len);
            self->cur->enq_generic(enq);
            if (enq.missing)
                Py_RETURN_NONE;
            return PyUnicode_FromStringAndSize(enq.res.data(), enq.res.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct __exit__ : MethVarargs<Impl>
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
            delete self->cur;
            self->cur = nullptr;
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


template<typename Definition, typename Impl>
struct DefinitionBase : public Binding<Definition, Impl>
{
    constexpr static const char* doc = R"(
A Cursor is the result of database queries. It is generally iterated through
the contents of the result. Each iteration returns the cursor itself, that can
be used to access the result values.
)";

    static void _dealloc(Impl* self)
    {
        delete self->cur;
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
            ensure_valid_cursor(self);
            Py_ssize_t len;
            const char* key = throw_ifnull(PyUnicode_AsUTF8AndSize(pykey, &len));
            // return enqpy(*self->cur, key, len);
            Enqpy enq(key, len);
            self->cur->enq_generic(enq);
            if (enq.missing)
            {
                PyErr_Format(PyExc_KeyError, "key %s not found", key);
                throw PythonException();
            }
            return enq.res;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct DefinitionStationDB : public DefinitionBase<DefinitionStationDB, dpy_CursorStationDB>
{
    constexpr static const char* name = "CursorStationDB";
    constexpr static const char* qual_name = "dballe.CursorStationDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_station results";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionStationDataDB : public DefinitionBase<DefinitionStationDataDB, dpy_CursorStationDataDB>
{
    constexpr static const char* name = "CursorStationDataDB";
    constexpr static const char* qual_name = "dballe.CursorStationDataDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_station_data results";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, query_attrs<Impl>, insert_attrs<Impl>, remove_attrs<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionDataDB : public DefinitionBase<DefinitionDataDB, dpy_CursorDataDB>
{
    constexpr static const char* name = "CursorDataDB";
    constexpr static const char* qual_name = "dballe.CursorDataDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_data results";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, query_attrs<Impl>, insert_attrs<Impl>, remove_attrs<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummaryDB : public DefinitionBase<DefinitionSummaryDB, dpy_CursorSummaryDB>
{
    constexpr static const char* name = "CursorSummaryDB";
    constexpr static const char* qual_name = "dballe.CursorSummaryDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_summary results";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, remove<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummarySummary : public DefinitionBase<DefinitionSummarySummary, dpy_CursorSummarySummary>
{
    constexpr static const char* name = "CursorSummarySummary";
    constexpr static const char* qual_name = "dballe.CursorSummarySummary";
    constexpr static const char* summary = "cursor iterating dballe.Explorer query_summary* results";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummaryDBSummary : public DefinitionBase<DefinitionSummaryDBSummary, dpy_CursorSummaryDBSummary>
{
    constexpr static const char* name = "CursorSummaryDBSummary";
    constexpr static const char* qual_name = "dballe.CursorSummaryDBSummary";
    constexpr static const char* summary = "cursor iterating dballe.DBExplorer query_summary* results";

    GetSetters<remaining<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


template<typename Impl>
struct message : Getter<Impl>
{
    constexpr static const char* name = "message";
    constexpr static const char* doc = "dballe.Message object with the current message";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            ensure_valid_cursor(self);
            if (self->curmsg)
            {
                Py_INCREF(self->curmsg);
                return self->curmsg;
            }
            Py_RETURN_NONE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct DefinitionMessage : public DefinitionBase<DefinitionMessage, dpy_CursorMessage>
{
    constexpr static const char* name = "CursorMessage";
    constexpr static const char* qual_name = "dballe.CursorMessage";
    constexpr static const char* summary = "cursor iterating Message results";

    GetSetters<remaining<Impl>, message<Impl>, query<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;

    static void _dealloc(Impl* self)
    {
        delete self->cur;
        Py_XDECREF(self->curmsg);
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _iternext(Impl* self)
    {
        try {
            ensure_valid_cursor(self);
            if (self->cur->next())
            {
                std::unique_ptr<Message> msg = self->cur->detach_message();
                Py_XDECREF(self->curmsg);
                self->curmsg = nullptr;
                self->curmsg = (PyObject*)message_create(std::move(msg));
                Py_INCREF(self);
                return (PyObject*)self;
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        } DBALLE_CATCH_RETURN_PYO
    }
};


DefinitionStationDB*         definition_stationdb = nullptr;
DefinitionStationDataDB*     definition_stationdatadb = nullptr;
DefinitionDataDB*            definition_datadb = nullptr;
DefinitionSummaryDB*         definition_summarydb = nullptr;
DefinitionSummarySummary*    definition_summarysummary = nullptr;
DefinitionSummaryDBSummary*  definition_summarydbsummary = nullptr;
DefinitionMessage*           definition_message = nullptr;

}

namespace dballe {
namespace python {

dpy_CursorStationDB* cursor_create(std::unique_ptr<db::v7::cursor::Stations> cur)
{
    py_unique_ptr<dpy_CursorStationDB> result(throw_ifnull(PyObject_New(dpy_CursorStationDB, dpy_CursorStationDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorStationDataDB* cursor_create(std::unique_ptr<db::v7::cursor::StationData> cur)
{
    py_unique_ptr<dpy_CursorStationDataDB> result(throw_ifnull(PyObject_New(dpy_CursorStationDataDB, dpy_CursorStationDataDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorDataDB* cursor_create(std::unique_ptr<db::v7::cursor::Data> cur)
{
    py_unique_ptr<dpy_CursorDataDB> result(throw_ifnull(PyObject_New(dpy_CursorDataDB, dpy_CursorDataDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorSummaryDB* cursor_create(std::unique_ptr<db::v7::cursor::Summary> cur)
{
    py_unique_ptr<dpy_CursorSummaryDB> result(throw_ifnull(PyObject_New(dpy_CursorSummaryDB, dpy_CursorSummaryDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorSummarySummary* cursor_create(std::unique_ptr<db::summary::Cursor<Station>> cur)
{
    py_unique_ptr<dpy_CursorSummarySummary> result(throw_ifnull(PyObject_New(dpy_CursorSummarySummary, dpy_CursorSummarySummary_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorSummaryDBSummary* cursor_create(std::unique_ptr<db::summary::Cursor<DBStation>> cur)
{
    py_unique_ptr<dpy_CursorSummaryDBSummary> result(throw_ifnull(PyObject_New(dpy_CursorSummaryDBSummary, dpy_CursorSummaryDBSummary_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorMessage* cursor_create(std::unique_ptr<dballe::CursorMessage> cur)
{
    py_unique_ptr<dpy_CursorMessage> result(throw_ifnull(PyObject_New(dpy_CursorMessage, dpy_CursorMessage_Type)));
    result->cur = impl::CursorMessage::downcast(std::move(cur)).release();
    result->curmsg = nullptr;
    return result.release();
}


void register_cursor(PyObject* m)
{
    common_init();

    definition_stationdb = new DefinitionStationDB;
    dpy_CursorStationDB_Type = definition_stationdb->activate(m);

    definition_stationdatadb = new DefinitionStationDataDB;
    dpy_CursorStationDataDB_Type = definition_stationdatadb->activate(m);

    definition_datadb = new DefinitionDataDB;
    dpy_CursorDataDB_Type = definition_datadb->activate(m);

    definition_summarydb = new DefinitionSummaryDB;
    dpy_CursorSummaryDB_Type = definition_summarydb->activate(m);

    definition_summarysummary = new DefinitionSummarySummary;
    dpy_CursorSummarySummary_Type = definition_summarysummary->activate(m);

    definition_summarydbsummary = new DefinitionSummaryDBSummary;
    dpy_CursorSummaryDBSummary_Type = definition_summarydbsummary->activate(m);

    definition_message = new DefinitionMessage;
    dpy_CursorMessage_Type = definition_message->activate(m);
}

}
}

#include "dballe/db/v7/cursor-access.tcc"
#include "dballe/db/summary-access.tcc"
