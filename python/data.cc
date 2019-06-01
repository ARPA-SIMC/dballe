#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "data.h"
#include "common.h"
#include "types.h"
#include "impl-utils.h"
#include "dballe/core/data.h"
// #include <algorithm>
// #include <sstream>
// #include "config.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

PyTypeObject* dpy_Data_Type = nullptr;

}

namespace {

namespace data {

#if 0
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
            auto query = query_from_python(pyquery);
            ReleaseGIL rg;
            self->explorer->set_filter(*query);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

dpy_ExplorerUpdate* update_create(const dpy_Explorer*)
{
    return (dpy_ExplorerUpdate*)PyObject_CallObject((PyObject*)dpy_ExplorerUpdate_Type, nullptr);
}

dpy_DBExplorerUpdate* update_create(const dpy_DBExplorer*)
{
    return (dpy_DBExplorerUpdate*)PyObject_CallObject((PyObject*)dpy_DBExplorerUpdate_Type, nullptr);
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
        PyObject* pyquery = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &pyquery))
            return nullptr;

        try {
            auto query = query_from_python(pyquery);
            ReleaseGIL gil;
            const auto& summary = get_summary<Station, Scope>(*self);
            auto res = summary.query_summary(*query);
            gil.lock();
            return (PyObject*)cursor_create(std::unique_ptr<db::summary::Cursor<Station>>(dynamic_cast<db::summary::Cursor<Station>*>(res.release())));
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
#endif

struct Definition : public Binding<Definition, dpy_Data>
{
    constexpr static const char* name = "Data";
    constexpr static const char* qual_name = "dballe.Data";
    constexpr static const char* doc = R"(
key-value representation of a value with its associated metadata
)";
    GetSetters<
    /*
        GetAllStations<Station>, GetStations<Station>,
        GetAllReports<Station>, GetReports<Station>,
        GetAllLevels<Station>, GetLevels<Station>,
        GetAllTranges<Station>, GetTranges<Station>,
        GetAllVarcodes<Station>, GetVarcodes<Station>,
        GetAllStats<Station>, GetStats<Station>*/> getsetters;
    Methods</*set_filter<Station>, rebuild<Station>, update<Station>, to_json<Station>, query_summary_all<Station>, query_summary<Station>*/> methods;

    static void _dealloc(Impl* self)
    {
        delete self->data;
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
            self->data = new core::Data;
        } DBALLE_CATCH_RETURN_INT

        // TODO: support a dict-like constructor?

        return 0;
    }

#if 0
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
#endif

    static PyObject* mp_subscript(Impl* self, PyObject* pykey)
    {
        try {
            Py_ssize_t len;
            const char* key = throw_ifnull(PyUnicode_AsUTF8AndSize(pykey, &len));
#if 0
            Enqpy enq(key, len);
            self->cur->enq_generic(enq);
            if (enq.missing)
            {
                PyErr_Format(PyExc_KeyError, "key %s not found", key);
                throw PythonException();
            }
            return enq.res;
#endif

            PyErr_Format(PyExc_NotImplementedError, "mp_subscript %s", key);
            throw PythonException();
        } DBALLE_CATCH_RETURN_PYO
    }

    static int mp_ass_subscript(Impl* self, PyObject *pykey, PyObject *val)
    {
        try {
            Py_ssize_t len;
            const char* key = throw_ifnull(PyUnicode_AsUTF8AndSize(pykey, &len));
            if (val)
                data_setpy(*self->data, key, len, val);
            else
            {
                // TODO: if v is nullptr, delete/unset
                PyErr_Format(PyExc_NotImplementedError, "mp_ass_subscript unset %s", key);
                throw PythonException();
            }
            return 0;
        } DBALLE_CATCH_RETURN_INT
    }

};

Definition* definition = nullptr;
}

}

namespace dballe {
namespace python {

dpy_Data* data_create()
{
    unique_ptr<core::Data> data(new core::Data);
    return data_create(move(data));
}

dpy_Data* data_create(std::unique_ptr<core::Data> data)
{
    dpy_Data* result = PyObject_New(dpy_Data, dpy_Data_Type);
    if (!result) return nullptr;

    result->data = data.release();
    return result;
}


void register_data(PyObject* m)
{
    common_init();

    data::definition = new data::Definition;
    dpy_Data_Type = data::definition->activate(m);
}

}
}

