#define _DBALLE_LIBRARY_CODE
#include "dballe/db/explorer.h"
#include "dballe/core/json.h"
#include "common.h"
#include "cursor.h"
#include "explorer.h"
#include "message.h"
#include "importer.h"
#include "types.h"
#include "db.h"
#include "utils/type.h"
#include <algorithm>
#include <sstream>
#include "config.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

PyTypeObject dpy_stats_Type;

PyTypeObject* dpy_Explorer_Type = nullptr;
PyTypeObject* dpy_DBExplorer_Type = nullptr;
PyTypeObject* dpy_ExplorerUpdate_Type = nullptr;
PyTypeObject* dpy_DBExplorerUpdate_Type = nullptr;

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

namespace explorer {

template<typename Base, typename Station, typename Scope, typename GET>
struct BaseGetter : public Getter<Base, typename ImplTraits<Station>::Impl>
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
struct GetAllStations : public BaseGetter<GetAllStations<Station>, Station, All, get_stations<Station>>
{
    constexpr static const char* name = "all_stations";
    constexpr static const char* doc = "get all stations";
};

template<typename Station>
struct GetStations : public BaseGetter<GetStations<Station>, Station, Selected, get_stations<Station>>
{
    constexpr static const char* name = "stations";
    constexpr static const char* doc = "get all the stations currently selected";
};

template<typename Station>
struct GetAllReports : public BaseGetter<GetAllReports<Station>, Station, All, get_reports<Station>>
{
    constexpr static const char* name = "all_reports";
    constexpr static const char* doc = "get all report values";
};

template<typename Station>
struct GetReports : public BaseGetter<GetReports<Station>, Station, Selected, get_reports<Station>>
{
    constexpr static const char* name = "reports";
    constexpr static const char* doc = "get all the report values currently selected";
};

template<typename Station>
struct GetAllLevels : public BaseGetter<GetAllLevels<Station>, Station, All, get_levels<Station>>
{
    constexpr static const char* name = "all_levels";
    constexpr static const char* doc = "get all level values";
};

template<typename Station>
struct GetLevels : public BaseGetter<GetLevels<Station>, Station, Selected, get_levels<Station>>
{
    constexpr static const char* name = "levels";
    constexpr static const char* doc = "get all the level values currently selected";
};

template<typename Station>
struct GetAllTranges : public BaseGetter<GetAllTranges<Station>, Station, All, get_tranges<Station>>
{
    constexpr static const char* name = "all_tranges";
    constexpr static const char* doc = "get all time range values";
};

template<typename Station>
struct GetTranges : public BaseGetter<GetTranges<Station>, Station, Selected, get_tranges<Station>>
{
    constexpr static const char* name = "tranges";
    constexpr static const char* doc = "get all the time range values currently selected";
};

template<typename Station>
struct GetAllVarcodes : public BaseGetter<GetAllVarcodes<Station>, Station, All, get_varcodes<Station>>
{
    constexpr static const char* name = "all_varcodes";
    constexpr static const char* doc = "get all varcode values";
};

template<typename Station>
struct GetVarcodes : public BaseGetter<GetVarcodes<Station>, Station, Selected, get_varcodes<Station>>
{
    constexpr static const char* name = "varcodes";
    constexpr static const char* doc = "get all the varcode values currently selected";
};


template<typename Base, typename Station, typename Scope>
struct BaseGetStats : public Getter<Base, typename ImplTraits<Station>::Impl>
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
struct GetAllStats : public BaseGetStats<GetAllStats<Station>, Station, All>
{
    constexpr static const char* name = "all_stats";
    constexpr static const char* doc = "get the stats for all values";
};

template<typename Station>
struct GetStats : public BaseGetStats<GetStats<Station>, Station, Selected>
{
    constexpr static const char* name = "stats";
    constexpr static const char* doc = "get stats for the currently selected values";
};


template<typename Station>
struct set_filter : public MethKwargs<set_filter<Station>, typename ImplTraits<Station>::Impl>
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
struct rebuild : public MethNoargs<rebuild<Station>, typename ImplTraits<Station>::Impl>
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
struct update : public MethNoargs<update<Station>, typename ImplTraits<Station>::Impl>
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
struct to_json : public MethNoargs<to_json<Station>, typename ImplTraits<Station>::Impl>
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

template<typename Base, typename Station, typename Scope>
struct BaseQuerySummary : public MethKwargs<Base, typename ImplTraits<Station>::Impl>
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

template<typename T>
struct query_summary_doc_traits
{
};

template<>
struct query_summary_doc_traits<Station>
{
    constexpr static const char* returns = "dballe.CursorSummarySummary";
    constexpr static const char* doc = R"(
:return: a cursor to iterate the query results (see :py:class:`dballe.CursorSummarySummary`)
)";
};

template<>
struct query_summary_doc_traits<DBStation>
{
    constexpr static const char* returns = "dballe.CursorSummaryDBSummary";
    constexpr static const char* doc = R"(
:return: a cursor to iterate the query results (see :py:class:`dballe.CursorSummaryDBSummary`)
)";
};


template<typename Station>
struct query_summary_all : public BaseQuerySummary<query_summary_all<Station>, Station, All>
{
    constexpr static const char* name = "query_summary_all";
    constexpr static const char* returns = query_summary_doc_traits<Station>::returns;
    constexpr static const char* summary = "Get all the Explorer summary information.";
    constexpr static const char* doc = query_summary_doc_traits<Station>::doc;
};

template<typename Station>
struct query_summary : public BaseQuerySummary<query_summary<Station>, Station, Selected>
{
    constexpr static const char* name = "query_summary";
    constexpr static const char* returns = query_summary_doc_traits<Station>::returns;
    constexpr static const char* summary = "Get the currently selected Explorer summary information";
    constexpr static const char* doc = query_summary_doc_traits<Station>::doc;
};


template<class Station>
struct Definition : Type<Definition<Station>, typename ImplTraits<Station>::Impl>
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

template<> const char* Definition<Station>::name = "Explorer";
template<> const char* Definition<Station>::qual_name = "dballe.Explorer";

template<> const char* Definition<DBStation>::name = "DBExplorer";
template<> const char* Definition<DBStation>::qual_name = "dballe.DBExplorer";

Definition<Station>* definition = nullptr;
Definition<DBStation>* definition_db = nullptr;
}

namespace explorerupdate {

template<typename Station>
struct __exit__ : public MethVarargs<__exit__<Station>, typename ImplTraits<Station>::UpdateImpl>
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
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
            self->update.commit();
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

template<typename Station>
struct add_db : public MethKwargs<add_db<Station>, typename ImplTraits<Station>::UpdateImpl>
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
    constexpr static const char* name = "add_db";
    constexpr static const char* doc = R"(
Add the summary of the contents of the given database to the Explorer.
)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "tr", NULL };
        dpy_Transaction* tr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O!", const_cast<char**>(kwlist), dpy_Transaction_Type, &tr))
            return nullptr;

        try {
            ReleaseGIL rg;
            self->update.add_db(*tr->db);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

template<typename Station>
struct add_json : public MethKwargs<add_json<Station>, typename ImplTraits<Station>::UpdateImpl>
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
    constexpr static const char* name = "add_json";
    constexpr static const char* doc = R"(
Add the contents of the given JSON string to the Explorer.
)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
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
};

template<typename Station>
struct add_explorer : public MethKwargs<add_explorer<Station>, typename ImplTraits<Station>::UpdateImpl>
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
    constexpr static const char* name = "add_explorer";
    constexpr static const char* doc = R"(
Add the contents of the given Explorer or DBExplorer to the Explorer.
)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
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

template<typename Station>
struct add_messages : public MethKwargs<add_messages<Station>, typename ImplTraits<Station>::UpdateImpl>
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
    constexpr static const char* name = "add_messages";
    constexpr static const char* doc = R"(
Add dballe.Message objects to the explorer.

It takes the same messages argument of dballe.DB.import_messages
)";
    [[noreturn]] static void throw_typeerror()
    {
        PyErr_SetString(PyExc_TypeError, "add_messages requires a dballe.Message, a sequence of dballe.Message objects, an iterable of dballe.Message objects, or the result of Importer.from_file");
        throw PythonException();
    }

    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "messages", nullptr };
        PyObject* obj;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &obj))
            return nullptr;

        try {
            if (dpy_Message_Check(obj))
            {
                self->update.add_message(*(((dpy_Message*)obj)->message));
                Py_RETURN_NONE;
            }

            if (dpy_ImporterFile_Check(obj))
            {
                dpy_ImporterFile* impf = (dpy_ImporterFile*)obj;
                while (auto binmsg = impf->file->file->file().read())
                {
                    auto messages = impf->importer->importer->from_binary(binmsg);
                    self->update.add_messages(messages);
                }
                Py_RETURN_NONE;
            }

            if (PySequence_Check(obj))
            {
                // Iterate sequence
                Py_ssize_t len = PySequence_Length(obj);
                if (len == -1) return nullptr;
                if (len == 0)
                    Py_RETURN_NONE;
                for (Py_ssize_t i = 0; i < len; ++i)
                {
                    pyo_unique_ptr o(throw_ifnull(PySequence_ITEM(obj, i)));
                    if (!dpy_Message_Check(o)) throw_typeerror();
                    self->update.add_message(*(((dpy_Message*)o.get())->message));
                }
                Py_RETURN_NONE;
            }

            if (PyIter_Check(obj))
            {
                // Iterate iterator
                pyo_unique_ptr iterator = throw_ifnull(PyObject_GetIter(obj));
                while (pyo_unique_ptr item = PyIter_Next(iterator))
                {
                    if (!dpy_Message_Check(item)) throw_typeerror();
                    self->update.add_message(*(((dpy_Message*)item.get())->message));
                }
                if (PyErr_Occurred()) return nullptr;
                Py_RETURN_NONE;
            }

            throw_typeerror();
        } DBALLE_CATCH_RETURN_PYO
    }
};


template<class Station>
struct Definition : public Type<Definition<Station>, typename ImplTraits<Station>::UpdateImpl>
{
    typedef typename ImplTraits<Station>::UpdateImpl Impl;
    static const char* name;
    static const char* qual_name;
    constexpr static const char* doc = "Manage updates to an Explorer";
    Methods<MethGenericEnter<typename ImplTraits<Station>::UpdateImpl>, __exit__<Station>, add_db<Station>, add_json<Station>, add_explorer<Station>, add_messages<Station>> methods;
    GetSetters<> getsetters;

    static void _dealloc(Impl* self)
    {
        self->update.~Update();
        Py_TYPE(self)->tp_free(self);
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

};

template<> const char* Definition<Station>::name = "ExplorerUpdate";
template<> const char* Definition<Station>::qual_name = "dballe.ExplorerUpdate";
template<> const char* Definition<DBStation>::name = "DBExplorerUpdate";
template<> const char* Definition<DBStation>::qual_name = "dballe.DBExplorerUpdate";

Definition<Station>* definition = nullptr;
Definition<DBStation>* definition_db = nullptr;
}

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
    dpy_Explorer* result = PyObject_New(dpy_Explorer, dpy_Explorer_Type);
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
    dpy_DBExplorer* result = PyObject_New(dpy_DBExplorer, dpy_DBExplorer_Type);
    if (!result) return nullptr;

    result->explorer = explorer.release();
    return result;
}

void register_explorer(PyObject* m)
{
    common_init();

    PyStructSequence_InitType(&dpy_stats_Type, &dpy_stats_desc);
    Py_INCREF(&dpy_stats_Type);
    if (PyModule_AddObject(m, "ExplorerStats", (PyObject*)&dpy_stats_Type) != 0)
        throw PythonException();

    explorer::definition = new explorer::Definition<Station>;
    explorer::definition->define(dpy_Explorer_Type, m);

    explorer::definition_db = new explorer::Definition<DBStation>;
    explorer::definition_db->define(dpy_DBExplorer_Type, m);

    explorerupdate::definition = new explorerupdate::Definition<Station>;
    explorerupdate::definition->define(dpy_ExplorerUpdate_Type, m);

    explorerupdate::definition_db = new explorerupdate::Definition<DBStation>;
    explorerupdate::definition_db->define(dpy_DBExplorerUpdate_Type, m);
}

}
}
