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

namespace {

template<typename Station>
static PyObject* _export_stations(const db::summary::StationEntries<Station>& stations)
{
    try {
        pyo_unique_ptr result(PyList_New(stations.size()));

        unsigned idx = 0;
        for (const auto& entry: stations)
        {
            pyo_unique_ptr station(to_python(entry.station));
            if (PyList_SetItem(result, idx, station.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_stations(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_stations(summary.stations());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _stations(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_stations(summary.stations());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_reports(const core::SortedSmallUniqueValueSet<std::string>& reports)
{
    try {
        pyo_unique_ptr result(PyList_New(reports.size()));

        unsigned idx = 0;
        for (const auto& v: reports)
        {
            pyo_unique_ptr value(string_to_python(v));
            if (PyList_SetItem(result, idx, value.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_reports(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_reports(summary.reports());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _reports(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_reports(summary.reports());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_levels(const core::SortedSmallUniqueValueSet<dballe::Level>& levels)
{
    try {
        pyo_unique_ptr result(PyList_New(levels.size()));

        unsigned idx = 0;
        for (const auto& v: levels)
        {
            pyo_unique_ptr level(level_to_python(v));
            if (PyList_SetItem(result, idx, level.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_levels(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_levels(summary.levels());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _levels(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_levels(summary.levels());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_tranges(const core::SortedSmallUniqueValueSet<dballe::Trange>& tranges)
{
    try {
        pyo_unique_ptr result(PyList_New(tranges.size()));

        unsigned idx = 0;
        for (const auto& v: tranges)
        {
            pyo_unique_ptr trange(trange_to_python(v));
            if (PyList_SetItem(result, idx, trange.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_tranges(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_tranges(summary.tranges());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _tranges(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_tranges(summary.tranges());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_varcodes(const core::SortedSmallUniqueValueSet<wreport::Varcode>& varcodes)
{
    try {
        pyo_unique_ptr result(PyList_New(varcodes.size()));

        unsigned idx = 0;
        for (const auto& v: varcodes)
        {
            pyo_unique_ptr varcode(varcode_to_python(v));
            if (PyList_SetItem(result, idx, varcode.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_varcodes(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_varcodes(summary.varcodes());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _varcodes(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_varcodes(summary.varcodes());
    } DBALLE_CATCH_RETURN_PYO
}


template<typename Station>
static PyObject* _export_stats(const dballe::db::BaseSummary<Station>& summary)
{
    try {
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

template<typename dpy_Explorer>
static PyObject* _all_stats(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_stats(summary);
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _stats(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_stats(summary);
    } DBALLE_CATCH_RETURN_PYO
}


template<typename T> struct ImplTraits {};

template<> struct ImplTraits<dpy_Explorer>
{
    typedef db::Explorer cpp_impl;
    typedef dpy_ExplorerUpdate dpy_Update;
};
template<> struct ImplTraits<dpy_DBExplorer>
{
    typedef db::DBExplorer cpp_impl;
    typedef dpy_DBExplorerUpdate dpy_Update;
};
template<> struct ImplTraits<dpy_ExplorerUpdate> { typedef db::Explorer::Update cpp_impl; };
template<> struct ImplTraits<dpy_DBExplorerUpdate> { typedef db::DBExplorer::Update cpp_impl; };



template<class Impl>
struct ExplorerDefinition
{
    static const char* name;
    static const char* qual_name;
    static PyGetSetDef getsetters[];
    static PyMethodDef methods[];
    static PyTypeObject type;

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
        string res = name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { nullptr };
        if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
            return -1;

        try {
            self->explorer = new typename ImplTraits<Impl>::cpp_impl;
        } DBALLE_CATCH_RETURN_INT

        return 0;
    }

    static PyObject* _set_filter(Impl* self, PyObject* args)
    {
        PyObject* pyquery = nullptr;
        if (!PyArg_ParseTuple(args, "O", &pyquery))
            return nullptr;

        try {
            core::Query query;
            read_query(pyquery, query);
            ReleaseGIL rg;
            self->explorer->set_filter(query);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }

    static PyObject* _to_json(Impl* self)
    {
        try {
            std::ostringstream json;
            {
                ReleaseGIL rg;
                core::JSONWriter writer(json);
                self->explorer->to_json(writer);
            }

            return string_to_python(json.str());
        } DBALLE_CATCH_RETURN_PYO
    }

    static typename ImplTraits<Impl>::dpy_Update* update_create();

    static PyObject* _rebuild(Impl* self)
    {
        try {
            auto res = update_create();
            res->update = self->explorer->rebuild();
            return (PyObject*)res;
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _update(Impl* self)
    {
        try {
            auto res = update_create();
            res->update = self->explorer->update();
            return (PyObject*)res;
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _query_summary_all(Impl* self, PyObject* args, PyObject* kw)
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
            std::unique_ptr<db::Cursor> res = self->explorer->global_summary().query_summary(query);
            gil.lock();
            return (PyObject*)cursor_create(move(res));
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _query_summary(Impl* self, PyObject* args, PyObject* kw)
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
            std::unique_ptr<db::Cursor> res = self->explorer->active_summary().query_summary(query);
            gil.lock();
            return (PyObject*)cursor_create(move(res));
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<> const char* ExplorerDefinition<dpy_Explorer>::name = "Explorer";
template<> const char* ExplorerDefinition<dpy_Explorer>::qual_name = "dballe.Explorer";

template<> const char* ExplorerDefinition<dpy_DBExplorer>::name = "DBExplorer";
template<> const char* ExplorerDefinition<dpy_DBExplorer>::qual_name = "dballe.DBExplorer";

template<>
dpy_ExplorerUpdate* ExplorerDefinition<dpy_Explorer>::update_create()
{
    return (dpy_ExplorerUpdate*)PyObject_CallObject((PyObject*)&dpy_ExplorerUpdate_Type, nullptr);
}

template<>
dpy_DBExplorerUpdate* ExplorerDefinition<dpy_DBExplorer>::update_create()
{
    return (dpy_DBExplorerUpdate*)PyObject_CallObject((PyObject*)&dpy_DBExplorerUpdate_Type, nullptr);
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
template<class Impl>
PyGetSetDef ExplorerDefinition<Impl>::getsetters[] = {
    {"all_stations", (getter)_all_stations<Impl>, nullptr, "get all stations", nullptr },
    {"stations", (getter)_stations<Impl>, nullptr, "get the stations currently selected", nullptr },
    {"all_reports", (getter)_all_reports<Impl>, nullptr, "get all rep_memo values", nullptr },
    {"reports", (getter)_reports<Impl>, nullptr, "get the rep_memo values currently selected", nullptr },
    {"all_levels", (getter)_all_levels<Impl>, nullptr, "get all level values", nullptr },
    {"levels", (getter)_levels<Impl>, nullptr, "get the level values currently selected", nullptr },
    {"all_tranges", (getter)_all_tranges<Impl>, nullptr, "get all time range values", nullptr },
    {"tranges", (getter)_tranges<Impl>, nullptr, "get the time range values currently selected", nullptr },
    {"all_varcodes", (getter)_all_varcodes<Impl>, nullptr, "get all varcode values", nullptr },
    {"varcodes", (getter)_varcodes<Impl>, nullptr, "get the varcode values currently selected", nullptr },
    {"all_stats", (getter)_all_stats<Impl>, nullptr, "get stats for all values", nullptr },
    {"stats", (getter)_stats<Impl>, nullptr, "get stats for currently selected values", nullptr },
    {nullptr}
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

template<typename Impl>
PyMethodDef ExplorerDefinition<Impl>::methods[] = {
    {"set_filter",        (PyCFunction)_set_filter, METH_VARARGS,
        "Set a new filter, updating all browsing data" },
    {"rebuild",           (PyCFunction)_rebuild, METH_NOARGS, R"(
        Empty the Explorer and start adding new data to it.

        Returns an ExplorerUpdate context manager object that can be used to
        add data to the explorer in a single transaction.
    )" },
    {"update",            (PyCFunction)_update, METH_NOARGS, R"(
        Start adding new data to the Explorer without clearing it first.

        Returns an ExplorerUpdate context manager object that can be used to
        add data to the explorer in a single transaction.
    )" },
    {"to_json",           (PyCFunction)_to_json, METH_NOARGS, R"(
        Serialize the contents of this explorer to JSON.

        Only the global summary is serialized: the current query is not
        preserved.
    )" },
    {"query_summary_all",     (PyCFunction)_query_summary_all, METH_VARARGS | METH_KEYWORDS,
        "Get all the Explorer summary information; returns a Cursor" },
    {"query_summary",     (PyCFunction)_query_summary, METH_VARARGS | METH_KEYWORDS,
        "Get the currently selected Explorer summary information; returns a Cursor" },
    {nullptr}
};

template<typename Impl>
PyTypeObject ExplorerDefinition<Impl>::type = {
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
    getsetters,                // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)_init,           // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};


template<class Impl>
struct ExplorerUpdateDefinition
{
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
        string res = name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { nullptr };
        if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
            return -1;

        try {
            new (&(self->update)) typename ImplTraits<Impl>::cpp_impl;
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

template<> const char* ExplorerUpdateDefinition<dpy_ExplorerUpdate>::name = "ExplorerUpdate";
template<> const char* ExplorerUpdateDefinition<dpy_ExplorerUpdate>::qual_name = "dballe.ExplorerUpdate";
template<> const char* ExplorerUpdateDefinition<dpy_DBExplorerUpdate>::name = "DBExplorerUpdate";
template<> const char* ExplorerUpdateDefinition<dpy_DBExplorerUpdate>::qual_name = "dballe.DBExplorerUpdate";

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

}

extern "C" {

PyTypeObject dpy_Explorer_Type = ExplorerDefinition<dpy_Explorer>::type;
PyTypeObject dpy_DBExplorer_Type = ExplorerDefinition<dpy_DBExplorer>::type;
PyTypeObject dpy_ExplorerUpdate_Type = ExplorerUpdateDefinition<dpy_ExplorerUpdate>::type;
PyTypeObject dpy_DBExplorerUpdate_Type = ExplorerUpdateDefinition<dpy_DBExplorerUpdate>::type;

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

void register_explorer(PyObject* m)
{
    common_init();

    PyStructSequence_InitType(&dpy_stats_Type, &dpy_stats_desc);
    Py_INCREF(&dpy_stats_Type);
    PyModule_AddObject(m, "ExplorerStats", (PyObject*)&dpy_stats_Type);

    dpy_Explorer_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Explorer_Type) < 0)
        return;
    Py_INCREF(&dpy_Explorer_Type);
    PyModule_AddObject(m, "Explorer", (PyObject*)&dpy_Explorer_Type);

    dpy_ExplorerUpdate_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_ExplorerUpdate_Type) < 0)
        return;
    // Only allow to create via an Explorer method
    // Py_INCREF(&dpy_ExplorerUpdate_Type);
    // PyModule_AddObject(m, "ExplorerUpdate", (PyObject*)&dpy_ExplorerUpdate_Type);

    dpy_DBExplorer_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DBExplorer_Type) < 0)
        return;
    Py_INCREF(&dpy_DBExplorer_Type);
    PyModule_AddObject(m, "DBExplorer", (PyObject*)&dpy_DBExplorer_Type);

    dpy_DBExplorerUpdate_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DBExplorerUpdate_Type) < 0)
        return;
    // Only allow to create via a DBExplorer method
    // Py_INCREF(&dpy_DBExplorerUpdate_Type);
    // PyModule_AddObject(m, "DBExplorerUpdate", (PyObject*)&dpy_DBExplorerUpdate_Type);
}

}
}
