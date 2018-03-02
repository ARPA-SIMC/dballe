#include <Python.h>
#include "common.h"
#include "explorer.h"
#include "types.h"
#include "db.h"
#include "record.h"
#include <algorithm>
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
using namespace wreport;

extern "C" {

static PyObject* _export_stations(const std::map<int, Station>& stations)
{
    try {
        pyo_unique_ptr result(PyList_New(stations.size()));

        unsigned idx = 0;
        for (const auto& s: stations)
        {
            pyo_unique_ptr station(station_to_python(s.second));
            if (PyList_SetItem(result, idx, station.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Explorer_all_stations(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_stations(summary.all_stations);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Explorer_stations(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_stations(summary.all_stations);
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_reports(const std::set<std::string>& reports)
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

static PyObject* dpy_Explorer_all_reports(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_reports(summary.all_reports);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Explorer_reports(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_reports(summary.all_reports);
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_levels(const std::set<dballe::Level>& levels)
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

static PyObject* dpy_Explorer_all_levels(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_levels(summary.all_levels);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Explorer_levels(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_levels(summary.all_levels);
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_tranges(const std::set<dballe::Trange>& tranges)
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

static PyObject* dpy_Explorer_all_tranges(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_tranges(summary.all_tranges);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Explorer_tranges(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_tranges(summary.all_tranges);
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_varcodes(const std::set<wreport::Varcode>& varcodes)
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

static PyObject* dpy_Explorer_all_varcodes(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_varcodes(summary.all_varcodes);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Explorer_varcodes(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_varcodes(summary.all_varcodes);
    } DBALLE_CATCH_RETURN_PYO
}


static PyGetSetDef dpy_Explorer_getsetters[] = {
    {"all_stations", (getter)dpy_Explorer_all_stations, nullptr, "get all stations", nullptr },
    {"stations", (getter)dpy_Explorer_stations, nullptr, "get the stations currently selected", nullptr },
    {"all_reports", (getter)dpy_Explorer_all_reports, nullptr, "get all rep_memo values", nullptr },
    {"reports", (getter)dpy_Explorer_reports, nullptr, "get the rep_memo values currently selected", nullptr },
    {"all_levels", (getter)dpy_Explorer_all_levels, nullptr, "get all level values", nullptr },
    {"levels", (getter)dpy_Explorer_levels, nullptr, "get the level values currently selected", nullptr },
    {"all_tranges", (getter)dpy_Explorer_all_tranges, nullptr, "get all time range values", nullptr },
    {"tranges", (getter)dpy_Explorer_tranges, nullptr, "get the time range values currently selected", nullptr },
    {"all_varcodes", (getter)dpy_Explorer_all_varcodes, nullptr, "get all varcode values", nullptr },
    {"varcodes", (getter)dpy_Explorer_varcodes, nullptr, "get the varcode values currently selected", nullptr },
    {nullptr}
};

static PyObject* dpy_Explorer_set_filter(dpy_Explorer* self, PyObject* args)
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

static PyObject* dpy_Explorer_revalidate(dpy_Explorer* self)
{
    try {
        ReleaseGIL rg;
        self->explorer->revalidate();
    } DBALLE_CATCH_RETURN_PYO

    Py_RETURN_NONE;
}

static PyMethodDef dpy_Explorer_methods[] = {
    {"set_filter",        (PyCFunction)dpy_Explorer_set_filter, METH_VARARGS,
        "Set a new filter, updating all browsing data" },
    {"revalidate",        (PyCFunction)dpy_Explorer_revalidate, METH_NOARGS, R"(
        Throw away all cached data and reload everything from the database.

        Use this when you suspect that the database has been externally modified
    )" },
    {nullptr}
};

static int dpy_Explorer_init(dpy_Explorer* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { "db", nullptr };
    dpy_DB* db = nullptr;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!", const_cast<char**>(kwlist), &dpy_DB_Type, &db))
        return -1;

    try {
        self->explorer = new db::Explorer(*db->db);
    } DBALLE_CATCH_RETURN_INT

    return 0;
}

static void dpy_Explorer_dealloc(dpy_Explorer* self)
{
    delete self->explorer;
    Py_TYPE(self)->tp_free(self);
}

static PyObject* dpy_Explorer_str(dpy_Explorer* self)
{
    /*
    std::string f = self->var.format("None");
    return PyUnicode_FromString(f.c_str());
    */
    return PyUnicode_FromString("Explorer");
}

static PyObject* dpy_Explorer_repr(dpy_Explorer* self)
{
    /*
    string res = "Var('";
    res += varcode_format(self->var.code());
    if (self->var.info()->is_string())
    {
        res += "', '";
        res += self->var.format();
        res += "')";
    } else {
        res += "', ";
        res += self->var.format("None");
        res += ")";
    }
    return PyUnicode_FromString(res.c_str());
    */
    return PyUnicode_FromString("Explorer object");
}

PyTypeObject dpy_Explorer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.Explorer",               // tp_name
    sizeof(dpy_Explorer),            // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_Explorer_dealloc, // tp_dealloc
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

}

namespace dballe {
namespace python {

dpy_Explorer* explorer_create(dballe::DB& db)
{
    unique_ptr<db::Explorer> explorer(new db::Explorer(db));
    return explorer_create(move(explorer));
}

dpy_Explorer* explorer_create(std::unique_ptr<db::Explorer> explorer)
{
    dpy_Explorer* result = PyObject_New(dpy_Explorer, &dpy_Explorer_Type);
    if (!result) return nullptr;

    result->explorer = explorer.release();
    return result;
}

void register_explorer(PyObject* m)
{
    common_init();

    dpy_Explorer_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Explorer_Type) < 0)
        return;

    Py_INCREF(&dpy_Explorer_Type);
    PyModule_AddObject(m, "Explorer", (PyObject*)&dpy_Explorer_Type);
}

}
}
