#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "dballe/db/explorer.h"
#include "dballe/core/json.h"
#include "common.h"
#include "cursor.h"
#include "explorer.h"
#include "types.h"
#include "db.h"
#include "record.h"
#include "impl-utils.h"
#include <algorithm>
#include <sstream>
#include "config.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

PyStructSequence_Field dpy_stats_fields[] = {
    { "datetime_min", "Minimum datetime" },
    { "datetime_max", "Maximum datetime" },
    { "count", "Number of values" },
    nullptr,
};

PyStructSequence_Desc dpy_stats_desc = {
    "ExplorerStats",
    "DB-All.e Explorer statistics",
    dpy_stats_fields,
    3,
};

PyTypeObject dpy_stats_Type;

}

namespace dballe {
namespace python {

template<typename Station>
inline PyObject* to_python(const db::summary::StationEntry<Station>& s) { return to_python(s.station); }

}
}

namespace {

struct All {};
struct Selected {};

template<typename T> struct ImplTraits {};

template<> struct ImplTraits<Station>
{
    typedef dpy_Explorer Impl;
    typedef dpy_ExplorerUpdate UpdateImpl;
    typedef db::Explorer cpp_impl;
    typedef db::Explorer::Update update_cpp_impl;
};

template<> struct ImplTraits<DBStation>
{
    typedef dpy_DBExplorer Impl;
    typedef dpy_DBExplorerUpdate UpdateImpl;
    typedef db::DBExplorer cpp_impl;
    typedef db::DBExplorer::Update update_cpp_impl;
    template<typename Scope>
    static const db::BaseSummary<Station>& summary();
};

template<typename Station, typename Scope>
const db::BaseSummary<Station>& get_summary(typename ImplTraits<Station>::Impl& impl) { throw std::runtime_error("not implemented"); }
template<> const db::BaseSummary<Station>& get_summary<Station, All>(dpy_Explorer& impl) { return impl.explorer->global_summary(); }
template<> const db::BaseSummary<Station>& get_summary<Station, Selected>(dpy_Explorer& impl) { return impl.explorer->active_summary(); }
template<> const db::BaseSummary<DBStation>& get_summary<DBStation, All>(dpy_DBExplorer& impl) { return impl.explorer->global_summary(); }
template<> const db::BaseSummary<DBStation>& get_summary<DBStation, Selected>(dpy_DBExplorer& impl) { return impl.explorer->active_summary(); }

template<typename Station>
struct get_stations {
    static const db::summary::StationEntries<Station>& get(const db::BaseSummary<Station>& summary) { return summary.stations(); }
};

template<typename Station>
struct get_reports {
    static const core::SortedSmallUniqueValueSet<std::string>& get(const db::BaseSummary<Station>& summary) { return summary.reports(); }
};

template<typename Station>
struct get_levels {
    static const core::SortedSmallUniqueValueSet<dballe::Level>& get(const db::BaseSummary<Station>& summary) { return summary.levels(); }
};

template<typename Station>
struct get_tranges {
    static const core::SortedSmallUniqueValueSet<dballe::Trange>& get(const db::BaseSummary<Station>& summary) { return summary.tranges(); }
};

template<typename Station>
struct get_varcodes {
    static const core::SortedSmallUniqueValueSet<wreport::Varcode>& get(const db::BaseSummary<Station>& summary) { return summary.varcodes(); }
};


template<typename Station, typename Scope, typename GET>
struct BaseGetter : public Getter<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            const auto& summary = get_summary<Station, Scope>(*self);
            const auto& stations = GET::get(summary);
            pyo_unique_ptr result(PyList_New(stations.size()));

            unsigned idx = 0;
            for (const auto& entry: stations)
            {
                pyo_unique_ptr station(to_python(entry));
                if (PyList_SetItem(result, idx, station.release()))
                    return nullptr;
                ++idx;
            }

            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct GetAllStations : public BaseGetter<Station, All, get_stations<Station>>
{
    constexpr static const char* name = "all_stations";
    constexpr static const char* doc = "get all stations";
};

template<typename Station>
struct GetStations : public BaseGetter<Station, Selected, get_stations<Station>>
{
    constexpr static const char* name = "stations";
    constexpr static const char* doc = "get all the stations currently selected";
};

template<typename Station>
struct GetAllReports : public BaseGetter<Station, All, get_reports<Station>>
{
    constexpr static const char* name = "all_reports";
    constexpr static const char* doc = "get all report values";
};

template<typename Station>
struct GetReports : public BaseGetter<Station, Selected, get_reports<Station>>
{
    constexpr static const char* name = "reports";
    constexpr static const char* doc = "get all the report values currently selected";
};

template<typename Station>
struct GetAllLevels : public BaseGetter<Station, All, get_levels<Station>>
{
    constexpr static const char* name = "all_levels";
    constexpr static const char* doc = "get all level values";
};

template<typename Station>
struct GetLevels : public BaseGetter<Station, Selected, get_levels<Station>>
{
    constexpr static const char* name = "levels";
    constexpr static const char* doc = "get all the level values currently selected";
};

template<typename Station>
struct GetAllTranges : public BaseGetter<Station, All, get_tranges<Station>>
{
    constexpr static const char* name = "all_tranges";
    constexpr static const char* doc = "get all time range values";
};

template<typename Station>
struct GetTranges : public BaseGetter<Station, Selected, get_tranges<Station>>
{
    constexpr static const char* name = "tranges";
    constexpr static const char* doc = "get all the time range values currently selected";
};

template<typename Station>
struct GetAllVarcodes : public BaseGetter<Station, All, get_varcodes<Station>>
{
    constexpr static const char* name = "all_varcodes";
    constexpr static const char* doc = "get all varcode values";
};

template<typename Station>
struct GetVarcodes : public BaseGetter<Station, Selected, get_varcodes<Station>>
{
    constexpr static const char* name = "varcodes";
    constexpr static const char* doc = "get all the varcode values currently selected";
};


template<typename Station, typename Scope>
struct BaseGetStats : public Getter<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            const auto& summary = get_summary<Station, Scope>(*self);
            pyo_unique_ptr res(PyStructSequence_New(&dpy_stats_Type));
            if (!res) return nullptr;

            if (PyObject* v = datetime_to_python(summary.datetime_min()))
                PyStructSequence_SET_ITEM((PyObject*)res, 0, v);
            else
                return nullptr;

            if (PyObject* v = datetime_to_python(summary.datetime_max()))
                PyStructSequence_SET_ITEM((PyObject*)res, 1, v);
            else
                return nullptr;

            if (PyObject* v = PyLong_FromLong(summary.data_count()))
                PyStructSequence_SET_ITEM((PyObject*)res, 2, v);
            else
                return nullptr;

            return res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct GetAllStats : public BaseGetStats<Station, All>
{
    constexpr static const char* name = "all_stats";
    constexpr static const char* doc = "get the stats for all values";
};

template<typename Station>
struct GetStats : public BaseGetStats<Station, Selected>
{
    constexpr static const char* name = "stats";
    constexpr static const char* doc = "get stats for the currently selected values";
};


template<typename Station>
struct set_filter : public MethKwargs<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "set_filter";
    constexpr static const char* doc = "Set a new filter, updating all browsing data";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", nullptr };
        PyObject* pyquery = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &pyquery))
            return nullptr;

        try {
            core::Query query;
            read_query(pyquery, query);
            ReleaseGIL rg;
            self->explorer->set_filter(query);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

dpy_ExplorerUpdate* update_create(const dpy_Explorer*)
{
    return (dpy_ExplorerUpdate*)PyObject_CallObject((PyObject*)&dpy_ExplorerUpdate_Type, nullptr);
}

dpy_DBExplorerUpdate* update_create(const dpy_DBExplorer*)
{
    return (dpy_DBExplorerUpdate*)PyObject_CallObject((PyObject*)&dpy_DBExplorerUpdate_Type, nullptr);
}


template<typename Station>
struct rebuild : public MethNoargs<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "rebuild";
    constexpr static const char* doc = R"(
Empty the Explorer and start adding new data to it.

Returns an ExplorerUpdate context manager object that can be used to
add data to the explorer in a single transaction.)";

    static PyObject* run(Impl* self)
    {
        try {
            auto res = update_create(self);
            res->update = self->explorer->rebuild();
            return (PyObject*)res;
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct update : public MethNoargs<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "update";
    constexpr static const char* doc = R"(
Start adding new data to the Explorer without clearing it first.

Returns an ExplorerUpdate context manager object that can be used to
add data to the explorer in a single transaction.)";

    static PyObject* run(Impl* self)
    {
        try {
            auto res = update_create(self);
            res->update = self->explorer->update();
            return (PyObject*)res;
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct to_json : public MethNoargs<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    constexpr static const char* name = "to_json";
    constexpr static const char* doc = R"(
Serialize the contents of this explorer to JSON.

Only the global summary is serialized: the current query is not
preserved.)";

    static PyObject* run(Impl* self)
    {
        std::ostringstream json;
        {
            ReleaseGIL rg;
            core::JSONWriter writer(json);
            self->explorer->to_json(writer);
        }

        return string_to_python(json.str());
    }
};

template<typename Station, typename Scope>
struct BaseQuerySummary : public MethKwargs<typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", nullptr };
        PyObject* record = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &record))
            return nullptr;

        try {
            core::Query query;
            if (read_query(record, query) == -1)
                return nullptr;
            ReleaseGIL gil;
            const auto& summary = get_summary<Station, Scope>(*self);
            std::unique_ptr<db::Cursor> res = summary.query_summary(query);
            gil.lock();
            return (PyObject*)cursor_create(move(res));
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Station>
struct query_summary_all : public BaseQuerySummary<Station, All>
{
    constexpr static const char* name = "query_summary_all";
    constexpr static const char* doc = "Get all the Explorer summary information; returns a Cursor";
};

template<typename Station>
struct query_summary : public BaseQuerySummary<Station, Selected>
{
    constexpr static const char* name = "query_summary";
    constexpr static const char* doc = "Get the currently selected Explorer summary information; returns a Cursor";
};


template<class Station>
struct ExplorerDefinition : public Binding<ExplorerDefinition<Station>, typename ImplTraits<Station>::Impl>
{
    typedef typename ImplTraits<Station>::Impl Impl;
    static const char* name;
    static const char* qual_name;
    constexpr static const char* doc = "Browser for a summary of DB-All-e database of message contents";
    GetSetters<
        GetAllStations<Station>, GetStations<Station>,
        GetAllReports<Station>, GetReports<Station>,
        GetAllLevels<Station>, GetLevels<Station>,
        GetAllTranges<Station>, GetTranges<Station>,
        GetAllVarcodes<Station>, GetVarcodes<Station>,
        GetAllStats<Station>, GetStats<Station>> getsetters;
    Methods<set_filter<Station>, rebuild<Station>, update<Station>, to_json<Station>, query_summary_all<Station>, query_summary<Station>> methods;

    static void _dealloc(Impl* self)
    {
        delete self->explorer;
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _str(Impl* self)
    {
        return PyUnicode_FromString(name);
    }

    static PyObject* _repr(Impl* self)
    {
        string res = qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { nullptr };
        if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
            return -1;

        try {
            self->explorer = new typename ImplTraits<Station>::cpp_impl;
        } DBALLE_CATCH_RETURN_INT

        return 0;
    }

};

template<> const char* ExplorerDefinition<Station>::name = "Explorer";
template<> const char* ExplorerDefinition<Station>::qual_name = "dballe.Explorer";

template<> const char* ExplorerDefinition<DBStation>::name = "DBExplorer";
template<> const char* ExplorerDefinition<DBStation>::qual_name = "dballe.DBExplorer";


template<class Station>
struct ExplorerUpdateDefinition
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
    static const char* name;
    static const char* qual_name;
    static PyMethodDef methods[];
    static PyTypeObject type;

    static void _dealloc(Impl* self)
    {
        self->update.~Update();
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _str(Impl* self)
    {
        return PyUnicode_FromString(name);
    }

    static PyObject* _repr(Impl* self)
    {
        string res = qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { nullptr };
        if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
            return -1;

        try {
            new (&(self->update)) typename ImplTraits<Station>::update_cpp_impl;
        } DBALLE_CATCH_RETURN_INT

        return 0;
    }

    static PyObject* _enter(Impl* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
    }

    static PyObject* _exit(Impl* self, PyObject* args)
    {
        PyObject* exc_type;
        PyObject* exc_val;
        PyObject* exc_tb;
        if (!PyArg_ParseTuple(args, "OOO", &exc_type, &exc_val, &exc_tb))
            return nullptr;

        try {
            ReleaseGIL gil;
            self->update.commit();
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }

    static PyObject* _add_json(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "string", NULL };
        const char* json_str;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &json_str))
            return nullptr;
        try {
            {
                ReleaseGIL rg;
                std::istringstream json(json_str);
                core::json::Stream in(json);
                self->update.add_json(in);
            }
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }

    static PyObject* _add_db(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "tr", NULL };
        dpy_Transaction* tr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O!", const_cast<char**>(kwlist), &dpy_Transaction_Type, &tr))
            return nullptr;

        try {
            ReleaseGIL rg;
            self->update.add_db(*tr->db);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }

    static PyObject* _add_explorer(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "explorer", NULL };
        PyObject* explorer;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &explorer))
            return nullptr;

        if (dpy_Explorer_Check(explorer))
        {
            try {
                ReleaseGIL rg;
                self->update.add_explorer(*((dpy_Explorer*)explorer)->explorer);
            } DBALLE_CATCH_RETURN_PYO
        } else if (dpy_DBExplorer_Check(explorer)) {
            try {
                ReleaseGIL rg;
                self->update.add_explorer(*((dpy_DBExplorer*)explorer)->explorer);
            } DBALLE_CATCH_RETURN_PYO
        } else {
            PyErr_SetString(PyExc_TypeError, "Expected a dballe.Explorer or dballe.DBExplorer object");
            return nullptr;
        }

        Py_RETURN_NONE;
    }
};

template<> const char* ExplorerUpdateDefinition<Station>::name = "ExplorerUpdate";
template<> const char* ExplorerUpdateDefinition<Station>::qual_name = "dballe.ExplorerUpdate";
template<> const char* ExplorerUpdateDefinition<DBStation>::name = "DBExplorerUpdate";
template<> const char* ExplorerUpdateDefinition<DBStation>::qual_name = "dballe.DBExplorerUpdate";

template<typename Impl>
PyMethodDef ExplorerUpdateDefinition<Impl>::methods[] = {
    {"add_db",            (PyCFunction)_add_db, METH_VARARGS | METH_KEYWORDS, R"(
        Add the summary of the contents of the given database to the Explorer.
    )" },
    {"add_json",          (PyCFunction)_add_json, METH_VARARGS | METH_KEYWORDS, R"(
        Add the contents of the given JSON string to the Explorer.
    )" },
    {"add_explorer",      (PyCFunction)_add_explorer, METH_VARARGS | METH_KEYWORDS, R"(
        Add the contents of the given Explorer or DBExplorer to the Explorer.
    )" },
    {"__enter__",         (PyCFunction)_enter, METH_NOARGS, "Context manager __enter__" },
    {"__exit__",          (PyCFunction)_exit, METH_VARARGS, "Context manager __exit__" },
    {nullptr}
};

template<typename Impl>
PyTypeObject ExplorerUpdateDefinition<Impl>::type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    qual_name,                 // tp_name
    sizeof(Impl),              // tp_basicsize
    0,                         // tp_itemsize
    (destructor)_dealloc,      // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)_repr,           // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)_str,            // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e Explorer",       // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    methods,                   // tp_methods
    0,                         // tp_members
    0,                         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)_init,           // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

ExplorerDefinition<Station>* definition_explorer = nullptr;
ExplorerDefinition<DBStation>* definition_dbexplorer = nullptr;

}

extern "C" {

PyTypeObject dpy_Explorer_Type;
PyTypeObject dpy_DBExplorer_Type;
PyTypeObject dpy_ExplorerUpdate_Type = ExplorerUpdateDefinition<Station>::type;
PyTypeObject dpy_DBExplorerUpdate_Type = ExplorerUpdateDefinition<DBStation>::type;

}

namespace dballe {
namespace python {

dpy_Explorer* explorer_create()
{
    unique_ptr<db::Explorer> explorer(new db::Explorer);
    return explorer_create(move(explorer));
}

dpy_Explorer* explorer_create(std::unique_ptr<db::Explorer> explorer)
{
    dpy_Explorer* result = PyObject_New(dpy_Explorer, &dpy_Explorer_Type);
    if (!result) return nullptr;

    result->explorer = explorer.release();
    return result;
}

dpy_DBExplorer* dbexplorer_create()
{
    unique_ptr<db::DBExplorer> explorer(new db::DBExplorer);
    return dbexplorer_create(move(explorer));
}

dpy_DBExplorer* dbexplorer_create(std::unique_ptr<db::DBExplorer> explorer)
{
    dpy_DBExplorer* result = PyObject_New(dpy_DBExplorer, &dpy_DBExplorer_Type);
    if (!result) return nullptr;

    result->explorer = explorer.release();
    return result;
}

int register_explorer(PyObject* m)
{
    if (common_init() != 0) return -1;

    PyStructSequence_InitType(&dpy_stats_Type, &dpy_stats_desc);
    Py_INCREF(&dpy_stats_Type);
    PyModule_AddObject(m, "ExplorerStats", (PyObject*)&dpy_stats_Type);

    definition_explorer = new ExplorerDefinition<Station>;
    if (definition_explorer->activate(dpy_Explorer_Type, m) != 0)
        return -1;
    definition_dbexplorer = new ExplorerDefinition<DBStation>;
    if (definition_dbexplorer->activate(dpy_DBExplorer_Type, m) != 0)
        return -1;

    dpy_ExplorerUpdate_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_ExplorerUpdate_Type) < 0)
        return -1;
    // Only allow to create via an Explorer method
    // Py_INCREF(&dpy_ExplorerUpdate_Type);
    // PyModule_AddObject(m, "ExplorerUpdate", (PyObject*)&dpy_ExplorerUpdate_Type);

    dpy_DBExplorerUpdate_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DBExplorerUpdate_Type) < 0)
        return -1;
    // Only allow to create via a DBExplorer method
    // Py_INCREF(&dpy_DBExplorerUpdate_Type);
    // PyModule_AddObject(m, "DBExplorerUpdate", (PyObject*)&dpy_DBExplorerUpdate_Type);

    return 0;
}

}
}
