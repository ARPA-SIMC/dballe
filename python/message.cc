#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include <dballe/message.h>
#include "common.h"
#include "message.h"
#include "types.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;


namespace {

#if 0

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
#endif

struct MessageDefinition
{
    static const char* name;
    static const char* qual_name;
    static PyGetSetDef getsetters[];
    static PyMethodDef methods[];
    static PyTypeObject type;

    static void _dealloc(dpy_Message* self)
    {
        self->message.~shared_ptr<Message>();
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _str(dpy_Message* self)
    {
        return PyUnicode_FromString(name);
    }

    static PyObject* _repr(dpy_Message* self)
    {
        string res = qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(dpy_Message* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "type", nullptr };
        PyObject* py_message_type = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &py_message_type))
            return -1;

        try {
            MessageType type;
            if (read_message_type(py_message_type, type) == -1)
                return -1;

            new (&(self->message)) std::shared_ptr<Message>(Message::create(type));
        } DBALLE_CATCH_RETURN_INT

        return 0;
    }
};

const char* MessageDefinition::name = "Message";
const char* MessageDefinition::qual_name = "dballe.Message";

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
PyGetSetDef MessageDefinition::getsetters[] = {
#if 0
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
#endif
    {nullptr}
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

PyMethodDef MessageDefinition::methods[] = {
#if 0
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
#endif
    {nullptr}
};

PyTypeObject MessageDefinition::type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    qual_name,                 // tp_name
    sizeof(dpy_Message),       // tp_basicsize
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
    "DB-All.e Message",        // tp_doc
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
}

extern "C" {

PyTypeObject dpy_Message_Type = MessageDefinition::type;

}

namespace dballe {
namespace python {

int read_message_type(PyObject* from_python, dballe::MessageType& type)
{
    try {
        if (PyUnicode_Check(from_python))
        {
            const char* v = PyUnicode_AsUTF8(from_python);
            if (v == nullptr) return -1;

            if (strcasecmp(v, "generic") == 0)
                type = MessageType::GENERIC;
            else if (strcasecmp(v, "synop") == 0)
                type = MessageType::SYNOP;
            else if (strcasecmp(v, "pilot") == 0)
                type = MessageType::PILOT;
            else if (strcasecmp(v, "temp") == 0)
                type = MessageType::TEMP;
            else if (strcasecmp(v, "temp_ship") == 0)
                type = MessageType::TEMP_SHIP;
            else if (strcasecmp(v, "airep") == 0)
                type = MessageType::AIREP;
            else if (strcasecmp(v, "amdar") == 0)
                type = MessageType::AMDAR;
            else if (strcasecmp(v, "acars") == 0)
                type = MessageType::ACARS;
            else if (strcasecmp(v, "ship") == 0)
                type = MessageType::SHIP;
            else if (strcasecmp(v, "buoy") == 0)
                type = MessageType::BUOY;
            else if (strcasecmp(v, "metar") == 0)
                type = MessageType::METAR;
            else if (strcasecmp(v, "sat") == 0)
                type = MessageType::SAT;
            else if (strcasecmp(v, "pollution") == 0)
                type = MessageType::POLLUTION;
            else
            {
                PyErr_Format(PyExc_ValueError, "%R is not a valid MessageType value", from_python);
                return -1;
            }
            return 0;
        }
    } DBALLE_CATCH_RETURN_INT

    PyErr_SetString(PyExc_TypeError, "Expected str");
    return -1;
}

dpy_Message* message_create(MessageType type)
{
    dpy_Message* res = PyObject_New(dpy_Message, &dpy_Message_Type);
    new (&(res->message)) std::shared_ptr<Message>(Message::create(type));
    return res;
}

dpy_Message* message_create(std::shared_ptr<Message> message)
{
    dpy_Message* res = PyObject_New(dpy_Message, &dpy_Message_Type);
    new (&(res->message)) std::shared_ptr<Message>(message);
    return res;
}

void register_message(PyObject* m)
{
    common_init();

    dpy_Message_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Message_Type) < 0)
        return;
    Py_INCREF(&dpy_Message_Type);
    PyModule_AddObject(m, "Message", (PyObject*)&dpy_Message_Type);
}

}
}
