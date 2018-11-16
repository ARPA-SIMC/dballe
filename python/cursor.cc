#include <Python.h>
#include "cursor.h"
#include "types.h"
#include "db.h"
#include "common.h"
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

template<typename Impl>
struct query_attrs : MethKwargs<Impl>
{
    constexpr static const char* name = "query_attrs";
    constexpr static const char* signature = "attrs: Iterable[str]";
    constexpr static const char* returns = "Dict[str, Any]";
    constexpr static const char* doc = "Query attributes for the current variable (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            ensure_valid_cursor(self);

            if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Cursor.attr_query, DB.attr_query_station or DB.attr_query_data instead of Cursor.query_attrs", 1))
                return nullptr;

            static const char* kwlist[] = { "attrs", NULL };
            PyObject* attrs = 0;
            if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &attrs))
                return nullptr;

            // Read the attribute list, if provided
            db::AttrList codes = db_read_attrlist(attrs);

            pyo_unique_ptr res(throw_ifnull(PyDict_New()));

            self->cur->get_transaction()->attr_query_station(self->cur->attr_reference_id(), [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                add_var_to_dict(res, *var);
            });
            return (PyObject*)res.release();
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
struct attr_query : MethNoargs<Impl>
{
    constexpr static const char* name = "attr_query";
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

            int res;
            if (!self->cur->enqi(key, len, res))
                Py_RETURN_NONE;
            return PyLong_FromLong(res);
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

            double res;
            if (!self->cur->enqd(key, len, res))
                Py_RETURN_NONE;
            return PyFloat_FromDouble(res);
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

            std::string res;
            if (!self->cur->enqs(key, len, res))
                Py_RETURN_NONE;
            return PyUnicode_FromStringAndSize(res.data(), res.size());
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

            std::string res;
            if (!self->cur->enqf(key, len, res))
                Py_RETURN_NONE;
            return PyUnicode_FromStringAndSize(res.data(), res.size());
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
            return enqpy(*self->cur, key, len);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct DefinitionStationDB : public DefinitionBase<DefinitionStationDB, dpy_CursorStationDB>
{
    constexpr static const char* name = "CursorStationDB";
    constexpr static const char* qual_name = "dballe.CursorStationDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_station results";

    GetSetters<remaining<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionStationDataDB : public DefinitionBase<DefinitionStationDataDB, dpy_CursorStationDataDB>
{
    constexpr static const char* name = "CursorStationDataDB";
    constexpr static const char* qual_name = "dballe.CursorStationDataDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_station_data results";

    GetSetters<remaining<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, query_attrs<Impl>, attr_query<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionDataDB : public DefinitionBase<DefinitionDataDB, dpy_CursorDataDB>
{
    constexpr static const char* name = "CursorDataDB";
    constexpr static const char* qual_name = "dballe.CursorDataDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_data results";

    GetSetters<remaining<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, query_attrs<Impl>, attr_query<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummaryDB : public DefinitionBase<DefinitionSummaryDB, dpy_CursorSummaryDB>
{
    constexpr static const char* name = "CursorSummaryDB";
    constexpr static const char* qual_name = "dballe.CursorSummaryDB";
    constexpr static const char* summary = "cursor iterating dballe.DB query_summary results";

    GetSetters<remaining<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummarySummary : public DefinitionBase<DefinitionSummarySummary, dpy_CursorSummarySummary>
{
    constexpr static const char* name = "CursorSummarySummary";
    constexpr static const char* qual_name = "dballe.CursorSummarySummary";
    constexpr static const char* summary = "cursor iterating dballe.Explorer query_summary* results";

    GetSetters<remaining<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


struct DefinitionSummaryDBSummary : public DefinitionBase<DefinitionSummaryDBSummary, dpy_CursorSummaryDBSummary>
{
    constexpr static const char* name = "CursorSummaryDBSummary";
    constexpr static const char* qual_name = "dballe.CursorSummaryDBSummary";
    constexpr static const char* summary = "cursor iterating dballe.DBExplorer query_summary* results";

    GetSetters<remaining<Impl>> getsetters;
    Methods<MethGenericEnter<Impl>, __exit__<Impl>, enqi<Impl>, enqd<Impl>, enqs<Impl>, enqf<Impl>> methods;
};


DefinitionStationDB*         definition_stationdb = nullptr;
DefinitionStationDataDB*     definition_stationdatadb = nullptr;
DefinitionDataDB*            definition_datadb = nullptr;
DefinitionSummaryDB*         definition_summarydb = nullptr;
DefinitionSummarySummary*    definition_summarysummary = nullptr;
DefinitionSummaryDBSummary*  definition_summarydbsummary = nullptr;

}

namespace dballe {
namespace python {

dpy_CursorStationDB* cursor_create(std::unique_ptr<db::CursorStation> cur)
{
    py_unique_ptr<dpy_CursorStationDB> result(throw_ifnull(PyObject_New(dpy_CursorStationDB, dpy_CursorStationDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorStationDataDB* cursor_create(std::unique_ptr<db::CursorStationData> cur)
{
    py_unique_ptr<dpy_CursorStationDataDB> result(throw_ifnull(PyObject_New(dpy_CursorStationDataDB, dpy_CursorStationDataDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorDataDB* cursor_create(std::unique_ptr<db::CursorData> cur)
{
    py_unique_ptr<dpy_CursorDataDB> result(throw_ifnull(PyObject_New(dpy_CursorDataDB, dpy_CursorDataDB_Type)));
    result->cur = cur.release();
    return result.release();
}

dpy_CursorSummaryDB* cursor_create(std::unique_ptr<db::CursorSummary> cur)
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
}

}
}
