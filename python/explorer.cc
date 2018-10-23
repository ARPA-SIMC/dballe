#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "dballe/db/explorer.h"
#include "dballe/core/json.h"
#include "common.h"
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

}

extern "C" {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
static PyGetSetDef dpy_Explorer_getsetters[] = {
    {"all_stations", (getter)_all_stations<dpy_Explorer>, nullptr, "get all stations", nullptr },
    {"stations", (getter)_stations<dpy_Explorer>, nullptr, "get the stations currently selected", nullptr },
    {"all_reports", (getter)_all_reports<dpy_Explorer>, nullptr, "get all rep_memo values", nullptr },
    {"reports", (getter)_reports<dpy_Explorer>, nullptr, "get the rep_memo values currently selected", nullptr },
    {"all_levels", (getter)_all_levels<dpy_Explorer>, nullptr, "get all level values", nullptr },
    {"levels", (getter)_levels<dpy_Explorer>, nullptr, "get the level values currently selected", nullptr },
    {"all_tranges", (getter)_all_tranges<dpy_Explorer>, nullptr, "get all time range values", nullptr },
    {"tranges", (getter)_tranges<dpy_Explorer>, nullptr, "get the time range values currently selected", nullptr },
    {"all_varcodes", (getter)_all_varcodes<dpy_Explorer>, nullptr, "get all varcode values", nullptr },
    {"varcodes", (getter)_varcodes<dpy_Explorer>, nullptr, "get the varcode values currently selected", nullptr },
    {"all_stats", (getter)_all_stats<dpy_Explorer>, nullptr, "get stats for all values", nullptr },
    {"stats", (getter)_stats<dpy_Explorer>, nullptr, "get stats for currently selected values", nullptr },
    {nullptr}
};
static PyGetSetDef dpy_DBExplorer_getsetters[] = {
    {"all_stations", (getter)_all_stations<dpy_DBExplorer>, nullptr, "get all stations", nullptr },
    {"stations", (getter)_stations<dpy_DBExplorer>, nullptr, "get the stations currently selected", nullptr },
    {"all_reports", (getter)_all_reports<dpy_DBExplorer>, nullptr, "get all rep_memo values", nullptr },
    {"reports", (getter)_reports<dpy_DBExplorer>, nullptr, "get the rep_memo values currently selected", nullptr },
    {"all_levels", (getter)_all_levels<dpy_DBExplorer>, nullptr, "get all level values", nullptr },
    {"levels", (getter)_levels<dpy_DBExplorer>, nullptr, "get the level values currently selected", nullptr },
    {"all_tranges", (getter)_all_tranges<dpy_DBExplorer>, nullptr, "get all time range values", nullptr },
    {"tranges", (getter)_tranges<dpy_DBExplorer>, nullptr, "get the time range values currently selected", nullptr },
    {"all_varcodes", (getter)_all_varcodes<dpy_DBExplorer>, nullptr, "get all varcode values", nullptr },
    {"varcodes", (getter)_varcodes<dpy_DBExplorer>, nullptr, "get the varcode values currently selected", nullptr },
    {"all_stats", (getter)_all_stats<dpy_DBExplorer>, nullptr, "get stats for all values", nullptr },
    {"stats", (getter)_stats<dpy_DBExplorer>, nullptr, "get stats for currently selected values", nullptr },
    {nullptr}
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

}

namespace {

template<typename dpy_Explorer>
static PyObject* _set_filter(dpy_Explorer* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return nullptr;

    // TODO: if it is a dict, turn it directly into a Query?

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        ReleaseGIL rg;
        self->explorer->set_filter(query);
    } DBALLE_CATCH_RETURN_PYO

    Py_RETURN_NONE;
}

template<typename dpy_Explorer>
static PyObject* _revalidate(dpy_Explorer* self, PyObject* args)
{
    dpy_Transaction* tr;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Transaction_Type, &tr))
        return nullptr;

    try {
        ReleaseGIL rg;
        self->explorer->revalidate(*tr->db);
    } DBALLE_CATCH_RETURN_PYO

    Py_RETURN_NONE;
}

template<typename dpy_Explorer>
static PyObject* _to_json(dpy_Explorer* self)
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

template<typename dpy_Explorer>
static PyObject* _from_json(dpy_Explorer* self, PyObject* args, PyObject* kw)
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
            self->explorer->from_json(in);
        }
    } DBALLE_CATCH_RETURN_PYO

    Py_RETURN_NONE;
}

}

extern "C" {

static PyMethodDef dpy_Explorer_methods[] = {
    {"set_filter",        (PyCFunction)_set_filter<dpy_Explorer>, METH_VARARGS,
        "Set a new filter, updating all browsing data" },
    {"revalidate",        (PyCFunction)_revalidate<dpy_Explorer>, METH_VARARGS, R"(
        Throw away all cached data and reload everything from the database.

        Use this when you suspect that the database has been externally modified
    )" },
    {"to_json",           (PyCFunction)_to_json<dpy_Explorer>, METH_NOARGS, R"(
        Serialize the contents of this explorer to JSON.

        Only the global summary is serialized: the current query is not
        preserved.
    )" },
    {"from_json",           (PyCFunction)_from_json<dpy_Explorer>, METH_VARARGS | METH_KEYWORDS, R"(
        Replace the contents of this Explorer with the contents of the given
        JSON string.

        The query is preserved and the filtered status is recomputed.
    )" },
    {nullptr}
};

static PyMethodDef dpy_DBExplorer_methods[] = {
    {"set_filter",        (PyCFunction)_set_filter<dpy_DBExplorer>, METH_VARARGS,
        "Set a new filter, updating all browsing data" },
    {"revalidate",        (PyCFunction)_revalidate<dpy_DBExplorer>, METH_VARARGS, R"(
        Throw away all cached data and reload everything from the database.

        Use this when you suspect that the database has been externally modified
    )" },
    {"to_json",           (PyCFunction)_to_json<dpy_DBExplorer>, METH_NOARGS, R"(
        Serialize the contents of this explorer to JSON.

        Only the global summary is serialized: the current query is not
        preserved.
    )" },
    {"from_json",         (PyCFunction)_from_json<dpy_DBExplorer>, METH_VARARGS | METH_KEYWORDS, R"(
        Replace the contents of this Explorer with the contents of the given
        JSON string.

        The query is preserved and the filtered status is recomputed.
    )" },
    {nullptr}
};

}

namespace {

static int dpy_Explorer_init(dpy_Explorer* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
        return -1;

    try {
        self->explorer = new db::Explorer;
    } DBALLE_CATCH_RETURN_INT

    return 0;
}

static int dpy_DBExplorer_init(dpy_DBExplorer* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
        return -1;

    try {
        self->explorer = new db::DBExplorer;
    } DBALLE_CATCH_RETURN_INT

    return 0;
}

template<typename dpy_Explorer>
static void _dealloc(dpy_Explorer* self)
{
    delete self->explorer;
    Py_TYPE(self)->tp_free(self);
}

static PyObject* dpy_Explorer_str(dpy_Explorer* self)
{
    return PyUnicode_FromString("Explorer");
}

static PyObject* dpy_DBExplorer_str(dpy_DBExplorer* self)
{
    return PyUnicode_FromString("DBExplorer");
}

static PyObject* dpy_Explorer_repr(dpy_Explorer* self)
{
    return PyUnicode_FromString("Explorer object");
}

static PyObject* dpy_DBExplorer_repr(dpy_DBExplorer* self)
{
    return PyUnicode_FromString("DBExplorer object");
}

}

extern "C" {

PyTypeObject dpy_Explorer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.Explorer",               // tp_name
    sizeof(dpy_Explorer),            // tp_basicsize
    0,                         // tp_itemsize
    (destructor)_dealloc<dpy_Explorer>, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Explorer_repr,     // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Explorer_str,      // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e Explorer",             // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_Explorer_methods,            // tp_methods
    0,                         // tp_members
    dpy_Explorer_getsetters,         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_Explorer_init,     // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

PyTypeObject dpy_DBExplorer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.DBExplorer",       // tp_name
    sizeof(dpy_DBExplorer),    // tp_basicsize
    0,                         // tp_itemsize
    (destructor)_dealloc<dpy_DBExplorer>, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_DBExplorer_repr, // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_DBExplorer_str,  // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e DBExplorer",     // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_DBExplorer_methods,      // tp_methods
    0,                         // tp_members
    dpy_DBExplorer_getsetters,   // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_DBExplorer_init,     // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

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

    dpy_DBExplorer_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DBExplorer_Type) < 0)
        return;
    Py_INCREF(&dpy_DBExplorer_Type);
    PyModule_AddObject(m, "DBExplorer", (PyObject*)&dpy_DBExplorer_Type);
}

}
}
