#include "db.h"
#include "cursor.h"
#include "common.h"
#include "types.h"
#include "dballe/types.h"
#include "dballe/file.h"
#include "dballe/values.h"
#include "dballe/core/query.h"
#include "dballe/core/data.h"
#include "dballe/message.h"
#include "dballe/importer.h"
#include "dballe/exporter.h"
#include "dballe/msg/msg.h"
#include "dballe/db/defs.h"
#include "dballe/db/v7/cursor.h"
#include <algorithm>
#include <wreport/bulletin.h>
#include "utils/type.h"
#include "message.h"
#include "importer.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_DB_Type = nullptr;
PyTypeObject* dpy_Transaction_Type = nullptr;
}

namespace {

bool deprecate_on_db(dpy_Transaction*, const char*) { return false; }
bool deprecate_on_db(dpy_DB*, const char* name)
{
    if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1, "calling %s without a transaction is deprecated", name))
        return true;
    return false;
}


/**
 * call o.fileno() and return its result.
 *
 * In case of AttributeError and IOError (parent of UnsupportedOperation, not
 * available from C), it clear the error indicator.
 *
 * Returns -1 if fileno() was not available or some other exception happened.
 * Use PyErr_Occurred to tell between the two.
 */
int file_get_fileno(PyObject* o)
{
    // fileno_value = obj.fileno()
    pyo_unique_ptr fileno_meth(PyObject_GetAttrString(o, "fileno"));
    if (!fileno_meth) return -1;
    pyo_unique_ptr fileno_args(Py_BuildValue("()"));
    if (!fileno_args) return -1;
    PyObject* fileno_value = PyObject_Call(fileno_meth, fileno_args, NULL);
    if (!fileno_value)
    {
        if (PyErr_ExceptionMatches(PyExc_AttributeError) || PyErr_ExceptionMatches(PyExc_IOError))
            PyErr_Clear();
        return -1;
    }

    // fileno = int(fileno_value)
    if (!PyObject_TypeCheck(fileno_value, &PyLong_Type)) {
        PyErr_SetString(PyExc_ValueError, "fileno() function must return an integer");
        return -1;
    }

    return PyLong_AsLong(fileno_value);
}

/**
 * call o.data() and return its result, both as a PyObject and as a buffer.
 *
 * The data returned in buf and len will be valid as long as the returned
 * object stays valid.
 */
PyObject* file_get_data(PyObject* o, char*&buf, Py_ssize_t& len)
{
    // Use read() instead
    pyo_unique_ptr read_meth(PyObject_GetAttrString(o, "read"));
    pyo_unique_ptr read_args(Py_BuildValue("()"));
    pyo_unique_ptr data(PyObject_Call(read_meth, read_args, NULL));
    if (!data) return nullptr;

    if (!PyObject_TypeCheck(data, &PyBytes_Type)) {
        PyErr_SetString(PyExc_ValueError, "read() function must return a bytes object");
        return nullptr;
    }
    if (PyBytes_AsStringAndSize(data, &buf, &len))
        return nullptr;

    return data.release();
}

static PyObject* get_insert_ids(const Data& data)
{
    const core::Data& vals = core::Data::downcast(data);
    pyo_unique_ptr res(throw_ifnull(PyDict_New()));
    pyo_unique_ptr ana_id(throw_ifnull(PyLong_FromLong(vals.station.id)));
    if (PyDict_SetItemString(res, "ana_id", ana_id))
        throw PythonException();

    for (const auto& v: vals.values)
    {
        pyo_unique_ptr id(throw_ifnull(PyLong_FromLong(v.data_id)));
        pyo_unique_ptr varcode(to_python(v.code()));

        if (PyDict_SetItem(res, varcode, id))
            throw PythonException();
    }

    return res.release();
}

template<typename Impl>
struct insert_station_data : MethKwargs<insert_station_data<Impl>, Impl>
{
    constexpr static const char* name = "insert_station_data";
    constexpr static const char* signature = "record: Union[Dict[str, Any], dballe.Cursor], can_replace: bool=False, can_add_stations: bool=False";
    constexpr static const char* returns = "Dict[str, int]";
    constexpr static const char* summary = "Insert station values in the database";
    constexpr static const char* doc = R"(
The return value is a dict that always contains `ana_id` mapped to the station
ID just inserted, and an entry for each varcode inserted mapping to the
database ID of its value.
)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "data", "can_replace", "can_add_stations", NULL };
        PyObject* pydata;
        int can_replace = 0;
        int can_add_stations = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii", const_cast<char**>(kwlist), &pydata, &can_replace, &can_add_stations))
            return nullptr;

        try {
            DataPtr data(pydata);
            ReleaseGIL gil;
            impl::DBInsertOptions opts;
            opts.can_replace = can_replace;
            opts.can_add_stations = can_add_stations;
            self->db->insert_station_data(*data, opts);
            gil.lock();
            return get_insert_ids(*data);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct insert_data : MethKwargs<insert_data<Impl>, Impl>
{
    constexpr static const char* name = "insert_data";
    constexpr static const char* signature = "record: Union[Dict[str, Any], dballe.Cursor], can_replace: bool=False, can_add_stations: bool=False";
    constexpr static const char* returns = "Dict[str, int]";
    constexpr static const char* summary = "Insert data values in the database";
    constexpr static const char* doc = R"(
The return value is a dict that always contains `ana_id` mapped to the station
ID just inserted, and an entry for each varcode inserted mapping to the
database ID of its value.
)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "data", "can_replace", "can_add_stations", NULL };
        PyObject* pydata;
        int can_replace = 0;
        int can_add_stations = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii", const_cast<char**>(kwlist), &pydata, &can_replace, &can_add_stations))
            return nullptr;

        try {
            DataPtr data(pydata);
            ReleaseGIL gil;
            impl::DBInsertOptions opts;
            opts.can_replace = can_replace;
            opts.can_add_stations = can_add_stations;
            self->db->insert_data(*data, opts);
            gil.lock();
            return get_insert_ids(*data);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Base, typename Impl>
struct MethQuery : public MethKwargs<Base, Impl>
{
    constexpr static const char* signature = "query: Dict[str, Any]";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, Base::name)) return nullptr;

        static const char* kwlist[] = { "query", NULL };
        PyObject* pyquery = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &pyquery))
            return nullptr;

        try {
            auto query = query_from_python(pyquery);
            return Base::run_query(self, *query);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct remove_station_data : MethQuery<remove_station_data<Impl>, Impl>
{
    constexpr static const char* name = "remove_station_data";
    constexpr static const char* summary = "Remove station variables from the database";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        try {
            ReleaseGIL gil;
            self->db->remove_station_data(query);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct remove_data : MethQuery<remove_data<Impl>, Impl>
{
    constexpr static const char* name = "remove_data";
    constexpr static const char* summary = "Remove data variables from the database";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        try {
            ReleaseGIL gil;
            self->db->remove_data(query);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct remove : MethQuery<remove<Impl>, Impl>
{
    constexpr static const char* name = "remove";
    constexpr static const char* summary = "Remove data variables from the database (deprecated)";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use remove_data instead of DB.remove", 1))
            return nullptr;
        try {
            ReleaseGIL gil;
            self->db->remove_data(query);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct remove_all : MethNoargs<remove_all<Impl>, Impl>
{
    constexpr static const char* name = "remove_all";
    constexpr static const char* summary = "Remove all data from the database";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->remove_all();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct query_stations : MethQuery<query_stations<Impl>, Impl>
{
    constexpr static const char* name = "query_stations";
    constexpr static const char* returns = "dballe.CursorStation";
    constexpr static const char* summary = "Query the stations in the database";
    constexpr static const char* doc = R"(
:return: a cursor to iterate the query results (see :py:class:`dballe.CursorStationDB`)
)";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->db->query_stations(query);
        gil.lock();
        return (PyObject*)cursor_create(db::v7::cursor::Stations::downcast(std::move(res)));
    }
};

template<typename Impl>
struct query_station_data : MethQuery<query_station_data<Impl>, Impl>
{
    constexpr static const char* name = "query_station_data";
    constexpr static const char* returns = "dballe.CursorStationData";
    constexpr static const char* summary = "Query the constant station data in the database";
    constexpr static const char* doc = R"(
:return: a cursor to iterate the query results (see :py:class:`dballe.CursorStationDataDB`)
)";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->db->query_station_data(query);
        gil.lock();
        return (PyObject*)cursor_create(db::v7::cursor::StationData::downcast(std::move(res)));
    }
};

template<typename Impl>
struct query_data : MethQuery<query_data<Impl>, Impl>
{
    constexpr static const char* name = "query_data";
    constexpr static const char* returns = "dballe.CursorData";
    constexpr static const char* summary = "Query the data in the database";
    constexpr static const char* doc = R"(
:return: a cursor to iterate the query results (see :py:class:`dballe.CursorDataDB`)
)";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->db->query_data(query);
        gil.lock();
        return (PyObject*)cursor_create(db::v7::cursor::Data::downcast(std::move(res)));
    }
};

template<typename Impl>
struct query_summary : MethQuery<query_summary<Impl>, Impl>
{
    constexpr static const char* name = "query_summary";
    constexpr static const char* returns = "dballe.CursorSummary";
    constexpr static const char* summary = "Query the summary of the results of a query";
    constexpr static const char* doc = R"(
:return: a cursor to iterate the query results (see :py:class:`dballe.CursorSummaryDB`)
)";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->db->query_summary(query);
        gil.lock();
        return (PyObject*)cursor_create(db::v7::cursor::Summary::downcast(std::move(res)));
    }
};

template<typename Impl>
struct query_messages : MethQuery<query_messages<Impl>, Impl>
{
    constexpr static const char* name = "query_messages";
    constexpr static const char* returns = "dballe.CursorMessage";
    constexpr static const char* summary = "Query the database returning the matching data as Message objects";
    constexpr static const char* doc = R"(
This can also be used to export messages to a file. For example::

    exporter = dballe.Exporter("BUFR")
    with open("file.bufr", "wb") as outfile:
        for row in tr.query_messages(...):
            outfile.write(exporter.to_binary(row.message))

See: :class:`dballe.Exporter` and :py:class:`dballe.CursorMessage`.
)";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->db->query_messages(query);
        gil.lock();
        return (PyObject*)cursor_create(std::move(res));
    }
};

template<typename Impl>
struct query_attrs : MethKwargs<query_attrs<Impl>, Impl>
{
    constexpr static const char* name = "query_attrs";
    constexpr static const char* signature = "varcode: str, reference_id: int, attrs: Iterable[str]";
    constexpr static const char* summary = "Query attributes (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Transaction.attr_query_station or Transaction.attr_query_data instead of DB.query_attrs", 1))
            return nullptr;

        static const char* kwlist[] = { "varcode", "reference_id", "attrs", NULL };
        int reference_id;
        const char* varname;
        PyObject* attrs = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", const_cast<char**>(kwlist), &varname, &reference_id, &attrs))
            return nullptr;

        try {
            // Read the attribute list, if provided
            db::AttrList codes = db_read_attrlist(attrs);
            pyo_unique_ptr res(throw_ifnull(PyDict_New()));
            self->db->attr_query_data(reference_id, [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                add_var_to_dict(res, *var);
            });
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};


template<typename Impl>
struct attr_query_station : MethKwargs<attr_query_station<Impl>, Impl>
{
    constexpr static const char* name = "attr_query_station";
    constexpr static const char* signature = "varid: int";
    constexpr static const char* returns = "Dict[str, Any]";
    constexpr static const char* summary = "query constant station data attributes";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "varid", NULL };
        int varid;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i", const_cast<char**>(kwlist), &varid))
            return nullptr;

        try {
            pyo_unique_ptr res(throw_ifnull(PyDict_New()));
            self->db->attr_query_station(varid, [&](unique_ptr<Var> var) {
                add_var_to_dict(res, *var);
            });
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct attr_query_data : MethKwargs<attr_query_data<Impl>, Impl>
{
    constexpr static const char* name = "attr_query_data";
    constexpr static const char* signature = "varid: int";
    constexpr static const char* returns = "Dict[str, Any]";
    constexpr static const char* doc = "query data attributes";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "varid", NULL };
        int varid;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i", const_cast<char**>(kwlist), &varid))
            return nullptr;

        try {
            pyo_unique_ptr res(throw_ifnull(PyDict_New()));
            self->db->attr_query_data(varid, [&](unique_ptr<Var>&& var) {
                add_var_to_dict(res, *var);
            });
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct attr_insert : MethKwargs<attr_insert<Impl>, Impl>
{
    constexpr static const char* name = "attr_insert";
    constexpr static const char* signature = "varcode: str, attrs: Dict[str, Any], varid: int=None";
    constexpr static const char* doc = "Insert new attributes into the database (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Transaction.attr_insert_station or Transaction.attr_insert_data instead of DB.attr_insert", 1))
            return nullptr;

        static const char* kwlist[] = { "varcode", "attrs", "varid", NULL };
        int varid = -1;
        const char* varname;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "sO|i", const_cast<char**>(kwlist),
                    &varname,
                    &attrs,
                    &varid))
            return nullptr;

        if (varid == -1)
        {
            PyErr_SetString(PyExc_ValueError, "please provide a reference_id argument: implicitly reusing the one from the last insert is not supported anymore");
            return nullptr;
        }

        try {
            Values values = values_from_python(attrs);
            ReleaseGIL gil;
            self->db->attr_insert_data(varid, values);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_insert_station : MethKwargs<attr_insert_station<Impl>, Impl>
{
    constexpr static const char* name = "attr_insert_station";
    constexpr static const char* signature = "varid: int, attrs: Dict[str, Any]";
    constexpr static const char* summary = "Insert new constant station data attributes into the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "iO", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            Values values = values_from_python(attrs);
            ReleaseGIL gil;
            self->db->attr_insert_station(varid, values);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_insert_data : MethKwargs<attr_insert_data<Impl>, Impl>
{
    constexpr static const char* name = "attr_insert_data";
    constexpr static const char* signature = "varid: int, attrs: Dict[str, Any]";
    constexpr static const char* summary = "Insert new data attributes into the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "iO", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            Values values = values_from_python(attrs);
            ReleaseGIL gil;
            self->db->attr_insert_data(varid, values);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_remove : MethKwargs<attr_remove<Impl>, Impl>
{
    constexpr static const char* name = "attr_remove";
    constexpr static const char* signature = "varcode: str, varid: int=None, attrs: Iterable[str]";
    constexpr static const char* doc = "Remove attributes (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Transaction.attr_remove_station or Transaction.attr_remove_data instead of DB.attr_remove", 1))
            return nullptr;

        static const char* kwlist[] = { "varcode", "varid", "attrs", NULL };
        int varid;
        const char* varname;
        PyObject* attrs = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", const_cast<char**>(kwlist), &varname, &varid, &attrs))
            return nullptr;

        try {
            // Read the attribute list, if provided
            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->db->attr_remove_data(varid, codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_remove_station : MethKwargs<attr_remove_station<Impl>, Impl>
{
    constexpr static const char* name = "attr_remove_station";
    constexpr static const char* signature = "varid: int, attrs: Iterable[str]";
    constexpr static const char* summary = "Remove attributes from constant station data";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i|O", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->db->attr_remove_station(varid, codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_remove_data : MethKwargs<attr_remove_data<Impl>, Impl>
{
    constexpr static const char* name = "attr_remove_data";
    constexpr static const char* signature = "varid: int, attrs: Iterable[str]";
    constexpr static const char* summary = "Remove attributes from data";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (deprecate_on_db(self, name)) return nullptr;

        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i|O", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->db->attr_remove_data(varid, codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename DB>
static unsigned db_load_file_enc(DB& db, Encoding encoding, FILE* file, bool close_on_exit, const std::string& name, DBImportOptions& opts)
{
    std::unique_ptr<File> f = File::create(encoding, file, close_on_exit, name);
    std::unique_ptr<Importer> imp = Importer::create(f->encoding());
    unsigned count = 0;
    f->foreach([&](const BinaryMessage& raw) {
        impl::Messages messages = imp->from_binary(raw);
        db.import_messages(messages, opts);
        ++count;
        return true;
    });
    return count;
}

template<typename DB>
static unsigned db_load_file(DB& db, FILE* file, bool close_on_exit, const std::string& name, DBImportOptions& opts)
{
    std::unique_ptr<File> f = File::create(file, close_on_exit, name);
    std::unique_ptr<Importer> imp = Importer::create(f->encoding());
    unsigned count = 0;
    f->foreach([&](const BinaryMessage& raw) {
        impl::Messages messages = imp->from_binary(raw);
        db.import_messages(messages, opts);
        ++count;
        return true;
    });
    return count;
}

template<typename Impl>
struct load : MethKwargs<load<Impl>, Impl>
{
    constexpr static const char* name = "load";
    constexpr static const char* signature = "fp: file, encoding: str=None, attrs: bool=False, full_pseudoana: bool=False, overwrite: bool=False";
    constexpr static const char* summary = "Load a file object in the database. (deprecated)";
    constexpr static const char* doc = R"(
An encoding can optionally be provided as a
string ("BUFR", "CREX"). If encoding is None then load will try to autodetect
based on the first byte of the file.
    )";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = {"fp", "encoding", "attrs", "full_pseudoana", "overwrite", NULL};
        PyObject* obj;
        const char* encoding = nullptr;
        int attrs = 0;
        int full_pseudoana = 0;
        int overwrite = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|siii", const_cast<char**>(kwlist), &obj, &encoding, &attrs, &full_pseudoana, &overwrite))
            return nullptr;

        try {
            auto opts = DBImportOptions::create();
            string repr = object_repr(obj);

            opts->import_attributes = attrs;
            opts->update_station = full_pseudoana;
            opts->overwrite = overwrite;

            int fileno = file_get_fileno(obj);
            if (fileno == -1)
            {
                if (PyErr_Occurred()) return nullptr;

                char* buf;
                Py_ssize_t len;
                pyo_unique_ptr data = file_get_data(obj, buf, len);
                if (!data) return nullptr;

                FILE* f = fmemopen(buf, len, "r");
                if (!f) return nullptr;
                unsigned count;
                if (encoding)
                {
                    count = db_load_file_enc(*self->db, File::parse_encoding(encoding), f, true, repr, *opts);
                } else
                    count = db_load_file(*self->db, f, true, repr, *opts);
                return PyLong_FromLong(count);
            } else {
                // Duplicate the file descriptor because both python and libc will want to
                // close it
                fileno = dup(fileno);
                if (fileno == -1)
                {
                    PyErr_Format(PyExc_OSError, "cannot dup() the file handle from %s", repr.c_str());
                    return nullptr;
                }

                FILE* f = fdopen(fileno, "rb");
                if (f == nullptr)
                {
                    close(fileno);
                    PyErr_Format(PyExc_OSError, "cannot fdopen() the dup()ed file handle from %s", repr.c_str());
                    return nullptr;
                }

                unsigned count;
                if (encoding)
                {
                    count = db_load_file_enc(*self->db, File::parse_encoding(encoding), f, true, repr, *opts);
                } else
                    count = db_load_file(*self->db, f, true, repr, *opts);
                return PyLong_FromLong(count);
            }
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct export_to_file : MethKwargs<export_to_file<Impl>, Impl>
{
    constexpr static const char* name = "export_to_file";
    constexpr static const char* signature = "query: Dict[str, Any], format: str, filename: Union[str, file], generic: bool=False";
    constexpr static const char* summary = "Export data matching a query as bulletins to a named file (deprecated)";

    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", "format", "filename", "generic", NULL };
        PyObject* pyquery;
        const char* format;
        PyObject* file;
        int as_generic = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OsO|i", const_cast<char**>(kwlist), &pyquery, &format, &file, &as_generic))
            return NULL;

        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use query_messages instead of export_to_file", 1))
            return nullptr;

        try {
            Encoding encoding = Encoding::BUFR;
            if (strcmp(format, "BUFR") == 0)
                encoding = Encoding::BUFR;
            else if (strcmp(format, "CREX") == 0)
                encoding = Encoding::CREX;
            else
            {
                PyErr_SetString(PyExc_ValueError, "encoding must be one of BUFR or CREX");
                return NULL;
            }

            auto query = query_from_python(pyquery);

            if (PyUnicode_Check(file))
            {
                std::string filename = string_from_python(file);
                std::unique_ptr<File> out = File::create(encoding, filename, "wb");
                impl::ExporterOptions opts;
                if (as_generic)
                    opts.template_name = "generic";
                auto exporter = Exporter::create(out->encoding(), opts);
                ReleaseGIL gil;
                auto cursor = self->db->query_messages(*query);
                while (cursor->next())
                {
                    impl::Messages msgs;
                    msgs.emplace_back(cursor->detach_message());
                    out->write(exporter->to_binary(msgs));
                }
                gil.lock();
                Py_RETURN_NONE;
            } else {
                impl::ExporterOptions opts;
                if (as_generic)
                    opts.template_name = "generic";
                auto exporter = Exporter::create(encoding, opts);
                pyo_unique_ptr res(nullptr);
                bool has_error = false;
                auto cursor = self->db->query_messages(*query);
                while (cursor->next())
                {
                    impl::Messages msgs;
                    msgs.emplace_back(cursor->detach_message());
                    std::string encoded = exporter->to_binary(msgs);
                    res = pyo_unique_ptr(PyObject_CallMethod(file, (char*)"write", (char*)"y#", encoded.data(), (int)encoded.size()));
                    if (!res)
                    {
                        has_error = true;
                        break;
                    }
                }
                if (has_error)
                    return nullptr;
                Py_RETURN_NONE;
            }
        } DBALLE_CATCH_RETURN_PYO
    }
};


template<typename Impl>
struct import_messages : MethKwargs<import_messages<Impl>, Impl>
{
    constexpr static const char* name = "import_messages";
    constexpr static const char* signature = "messages: Union[dballe.Message, Sequence[dballe.Message], Iterable[dballe.Message], dballe.ImporterFile], report: str=None, import_attributes: bool=False, update_station: bool=False, overwrite: bool=False";
    constexpr static const char* summary = "Import one or more Messages into the database.";
    constexpr static const char* doc = R"(
:arg messages:
 * a :class:`dballe.Message` object
 * a sequence or iterable of :class:`dballe.Message` objects
 * a :class:`dballe.ImporterFile` that generates a sequence of :class:`dballe.Message` objects
:arg report: the network name to use for importing the data. If left to None,
             the network is selected automatically from the message type
:arg import_attributes: if set to True, requests the variable attributes to
                        also be imported.
:arg update_station: if set to True, station information is merged with existing
                     one in the database. If false (default), station
                     information is imported only when the station did not
                     exist in the database.
:arg overwrite: if set to True, causes existing information already in the
                database to be overwritten. If false (default), trying to
                import a message which contains data already present in the
                database causes the import to fail.
:arg varlist: if set to a string in the same format as the `varlist` query
              parameter, only imports data whose varcode is in the list.
)";

    [[noreturn]] static void throw_typeerror()
    {
        PyErr_SetString(PyExc_TypeError, "import_messages requires a dballe.Message, a sequence of dballe.Message objects, an iterable of dballe.Message objects, or the result of Importer.from_file");
        throw PythonException();
    }

    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = {"messages", "report", "import_attributes", "update_station", "overwrite", "varlist", nullptr};
        PyObject* obj = nullptr;
        const char* report = nullptr;
        int import_attributes = 0;
        int update_station = 0;
        int overwrite = 0;
        const char* varlist = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|sppps", const_cast<char**>(kwlist), &obj, &report, &import_attributes, &update_station, &overwrite, &varlist))
            return nullptr;

        try {
            auto opts = DBImportOptions::create();
            if (report) opts->report = report;
            opts->import_attributes = import_attributes;
            opts->update_station = update_station;
            opts->overwrite = overwrite;
            if (varlist)
                resolve_varlist(varlist, [&](wreport::Varcode code) { opts->varlist.push_back(code); });

            if (dpy_Message_Check(obj))
            {
                self->db->import_message(*(((dpy_Message*)obj)->message), *opts);
                Py_RETURN_NONE;
            }

            if (dpy_ImporterFile_Check(obj))
            {
                dpy_ImporterFile* impf = (dpy_ImporterFile*)obj;
                while (auto binmsg = impf->file->file->file().read())
                {
                    auto messages = impf->importer->importer->from_binary(binmsg);
                    self->db->import_messages(messages, *opts);
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
                std::vector<std::shared_ptr<Message>> messages;
                messages.reserve(len);
                for (Py_ssize_t i = 0; i < len; ++i)
                {
                    pyo_unique_ptr o(throw_ifnull(PySequence_ITEM(obj, i)));
                    if (!dpy_Message_Check(o)) throw_typeerror();
                    messages.push_back(((dpy_Message*)o.get())->message);
                }
                self->db->import_messages(messages, *opts);
                Py_RETURN_NONE;
            }

            if (PyIter_Check(obj))
            {
                // Iterate iterator
                pyo_unique_ptr iterator = throw_ifnull(PyObject_GetIter(obj));
                while (pyo_unique_ptr item = PyIter_Next(iterator))
                {
                    if (!dpy_Message_Check(item)) throw_typeerror();
                    self->db->import_message(*(((dpy_Message*)item.get())->message), *opts);
                }
                if (PyErr_Occurred()) return nullptr;
                Py_RETURN_NONE;
            }

            throw_typeerror();
        } DBALLE_CATCH_RETURN_PYO
    }
};

namespace pydb {

struct get_default_format : ClassMethNoargs<get_default_format>
{
    constexpr static const char* name = "get_default_format";
    constexpr static const char* returns = "str";
    constexpr static const char* summary = "get the default DB format";
    static PyObject* run(PyTypeObject* cls)
    {
        try {
            string format = db::format_format(db::DB::get_default_format());
            return PyUnicode_FromString(format.c_str());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct set_default_format : ClassMethKwargs<set_default_format>
{
    constexpr static const char* name = "set_default_format";
    constexpr static const char* signature = "format: str";
    constexpr static const char* summary = "set the default DB format";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "format", nullptr };
        const char* format;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &format))
            return nullptr;

        try {
            db::DB::set_default_format(db::format_parse(format));
            Py_RETURN_NONE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect_from_file : ClassMethKwargs<connect_from_file>
{
    constexpr static const char* name = "connect_from_file";
    constexpr static const char* signature = "name: str";
    constexpr static const char* returns = "dballe.DB";
    constexpr static const char* summary = "create a DB to access a SQLite file";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", nullptr };
        const char* name;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &name))
            return nullptr;

        try {
            ReleaseGIL gil;
            std::shared_ptr<db::DB> db = db::DB::connect_from_file(name);
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect_from_url : ClassMethKwargs<connect_from_url>
{
    constexpr static const char* name = "connect_from_url";
    constexpr static const char* signature = "url: str";
    constexpr static const char* returns = "dballe.DB";
    constexpr static const char* summary = "create a DB to access a database identified by a DB-All.e URL (deprecated, use connect instead)";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use connect instead of connect_from_url", 1))
            return nullptr;

        static const char* kwlist[] = { "url", nullptr };
        const char* url;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &url))
            return nullptr;

        try {
            ReleaseGIL gil;
            auto opts = DBConnectOptions::create(url);
            shared_ptr<db::DB> db = dynamic_pointer_cast<db::DB>(DB::connect(*opts));
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect : ClassMethKwargs<connect>
{
    constexpr static const char* name = "connect";
    constexpr static const char* signature = "url: str";
    constexpr static const char* returns = "dballe.DB";
    constexpr static const char* summary = "create a DB to access a database identified by a DB-All.e URL";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "url", nullptr };
        const char* url;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &url))
            return nullptr;

        try {
            ReleaseGIL gil;
            auto opts = DBConnectOptions::create(url);
            shared_ptr<db::DB> db = dynamic_pointer_cast<db::DB>(DB::connect(*opts));
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect_test : ClassMethNoargs<connect_test>
{
    constexpr static const char* name = "connect_test";
    constexpr static const char* returns = "dballe.DB";
    constexpr static const char* summary = "Create a DB for running the test suite, as configured in the test environment";
    static PyObject* run(PyTypeObject* cls)
    {
        try {
            ReleaseGIL gil;
            auto options = DBConnectOptions::test_create();
            shared_ptr<db::DB> db = dynamic_pointer_cast<db::DB>(DB::connect(*options));
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct is_url : ClassMethKwargs<is_url>
{
    constexpr static const char* name = "is_url";
    constexpr static const char* signature = "url: str";
    constexpr static const char* returns = "bool";
    constexpr static const char* summary = "Checks if a string looks like a DB-All.e DB url";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "url", nullptr };
        const char* url;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &url))
            return nullptr;

        try {
            if (db::DB::is_url(url))
                Py_RETURN_TRUE;
            else
                Py_RETURN_FALSE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct transaction : MethKwargs<transaction, dpy_DB>
{
    constexpr static const char* name = "transaction";
    constexpr static const char* signature = "readonly: bool=False";
    constexpr static const char* returns = "dballe.Transaction";
    constexpr static const char* summary = "Create a new database transaction";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "readonly", nullptr };
        int readonly = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|p", const_cast<char**>(kwlist), &readonly))
            return nullptr;

        try {
            auto res = dynamic_pointer_cast<db::Transaction>(self->db->transaction(readonly));
            return (PyObject*)transaction_create(move(res));
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct disappear : MethNoargs<disappear, dpy_DB>
{
    constexpr static const char* name = "disappear";
    constexpr static const char* doc = "Remove all DB-All.e tables and data from the database, if possible";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->disappear();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct reset : MethKwargs<reset, dpy_DB>
{
    constexpr static const char* name = "reset";
    constexpr static const char* signature = "repinfo_file: str=None";
    constexpr static const char* doc = "Reset the database, removing all existing Db-All.e tables and re-creating them empty.";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "repinfo_file", nullptr };
        const char* repinfo_file = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|s", const_cast<char**>(kwlist), &repinfo_file))
            return nullptr;

        try {
            ReleaseGIL gil;
            self->db->reset(repinfo_file);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct vacuum : MethNoargs<vacuum, dpy_DB>
{
    constexpr static const char* name = "vacuum";
    constexpr static const char* doc = "Perform database cleanup operations";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->vacuum();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


struct Definition : public Type<Definition, dpy_DB>
{
    constexpr static const char* name = "DB";
    constexpr static const char* qual_name = "dballe.DB";
    constexpr static const char* doc = R"(
DB-All.e database access.

Many methods are the same in :class:`dballe.DB` and
:class:`dballe.Transaction`. The versions in :class:`dballe.DB` are implemented
by automatically creating a temporary transaction and running the equivalent
:class:`dballe.Transaction` method inside it.

:class:`dballe.DB` objects are not constructed explicitly, but via one of the
:func:`DB.connect` or :func:`DB.connect_test` class methods.

Examples:

::

    # Connect to a database and run a query
    db = dballe.DB.connect_from_file("db.sqlite")
    query = {"latmin": 44.0, "latmax": 45.0, "lonmin": 11.0, "lonmax": 12.0}

    # The result is a dballe.Cursor (dballe.CursorData in this case), which can
    # be iterated to get results as dict objects.
    with db.transaction() as tr:
        for row in tr.query_data(query):
            print(row["lat"], row["lon"], row["var"], row["variable"].format("undefined"))

    # Insert 2 new variables in the database
    db.insert_data({
        "lat": 44.5, "lon": 11.4,
        "level": dballe.Level(1),
        "trange": dballe.Trange(254),
        "date": datetime.datetime(2013, 4, 25, 12, 0, 0),
        "B11101": 22.4,
        "B12103": 17.2,
    })
)";

    GetSetters<> getsetters;
    Methods<
        get_default_format, set_default_format,
        connect_from_file, connect, connect_from_url, connect_test, is_url,
        disappear, reset, vacuum,
        transaction,
        insert_station_data<Impl>, insert_data<Impl>,
        remove_station_data<Impl>, remove_data<Impl>, remove_all<Impl>, remove<Impl>,
        query_stations<Impl>, query_station_data<Impl>, query_data<Impl>, query_summary<Impl>, query_messages<Impl>, query_attrs<Impl>,
        attr_query_station<Impl>, attr_query_data<Impl>,
        attr_insert<Impl>, attr_insert_station<Impl>, attr_insert_data<Impl>,
        attr_remove<Impl>, attr_remove_station<Impl>, attr_remove_data<Impl>,
        import_messages<Impl>, load<Impl>, export_to_file<Impl>
        > methods;

    static void _dealloc(Impl* self)
    {
        self->db.~shared_ptr<db::DB>();
        Py_TYPE(self)->tp_free(self);
    }
};

Definition* definition = nullptr;

}


namespace pytr {

typedef MethGenericEnter<dpy_Transaction> __enter__;

struct __exit__ : MethVarargs<__exit__, dpy_Transaction>
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
            if (exc_type == Py_None)
                self->db->commit();
            else
                self->db->rollback();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct commit : MethNoargs<commit, dpy_Transaction>
{
    constexpr static const char* name = "commit";
    constexpr static const char* summary = "commit the transaction";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->commit();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct rollback : MethNoargs<rollback, dpy_Transaction>
{
    constexpr static const char* name = "rollback";
    constexpr static const char* summary = "roll back the transaction";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->rollback();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


struct Definition : public Type<Definition, dpy_Transaction>
{
    constexpr static const char* name = "Transaction";
    constexpr static const char* qual_name = "dballe.Transaction";
    constexpr static const char* doc = R"(
DB-All.e transaction

A Transaction is used to execute DB operations in an all-or-nothing fashion. In
fact, most DB methods are implemented using a short-lived temporary
transaction.

You cannot have more than one active dballe.Transaction for each dballe.DB. An
attempt to start a second one will result in an exception being raised. Note
that dballe.DB functions like :func:`dballe.Transaction.insert_data` create a
temporary transaction to run, and so they will also fail if a transaction is
currently open. The general idea is that all database work should be done
inside a transaction.

Transactions run using the `REPEATABLE READ` isolation level of the underlying
database. This usually means that modifications performed inside a transaction
are not visible to other database connections until the transaction is
committed. If a transaction is rolled back, all changes done with it are
undone.

Transactions can also be used as context managers, like this:

::

    with db.transaction() as t:
        for i in range(10):
            t.insert({
                "lat": 44.5 + i, "lon": 11.4 + i, "level": (1,),
                "trange": (254,), "date": datetime.datetime(2013, 4, 25, 12, 0, 0),
                "B11101": 22.4 + i, "B12103": 17.2
            })

The dballe.Transaction methods are the same as those in dballe.DB. The version
in dballe.DB is implemented by automatically creating a temporary transaction
and running the dballe.Transaction method inside it.
)";

    GetSetters<> getsetters;
    Methods<
        insert_station_data<Impl>, insert_data<Impl>,
        remove_station_data<Impl>, remove_data<Impl>, remove_all<Impl>, remove<Impl>,
        query_stations<Impl>, query_station_data<Impl>, query_data<Impl>, query_summary<Impl>, query_messages<Impl>,
        attr_query_station<Impl>, attr_query_data<Impl>,
        attr_insert_station<Impl>, attr_insert_data<Impl>,
        attr_remove_station<Impl>, attr_remove_data<Impl>,
        import_messages<Impl>, load<Impl>, export_to_file<Impl>,
        __enter__, __exit__, commit, rollback
        > methods;

    static void _dealloc(Impl* self)
    {
        self->db.~shared_ptr<dballe::db::Transaction>();
        Py_TYPE(self)->tp_free(self);
    }
};

Definition* definition = nullptr;

}

}

namespace dballe {
namespace python {

db::AttrList db_read_attrlist(PyObject* attrs)
{
    db::AttrList res;
    if (!attrs) return res;
    pyo_unique_ptr iter(throw_ifnull(PyObject_GetIter(attrs)));

    while (PyObject* iter_item = PyIter_Next(iter)) {
        pyo_unique_ptr item(iter_item);
        string name = string_from_python(item);
        res.push_back(resolve_varcode(name));
    }
    return res;
}

dpy_DB* db_create(std::shared_ptr<db::DB> db)
{
    py_unique_ptr<dpy_DB> res = throw_ifnull(PyObject_New(dpy_DB, dpy_DB_Type));
    new (&(res->db)) std::shared_ptr<db::DB>(db);
    return res.release();
}

dpy_Transaction* transaction_create(std::shared_ptr<db::Transaction> transaction)
{
    py_unique_ptr<dpy_Transaction> res = throw_ifnull(PyObject_New(dpy_Transaction, dpy_Transaction_Type));
    new (&(res->db)) std::shared_ptr<db::Transaction>(transaction);
    return res.release();
}

void register_db(PyObject* m)
{
    common_init();

    pydb::definition = new pydb::Definition;
    pydb::definition->define(dpy_DB_Type, m);

    pytr::definition = new pytr::Definition;
    pytr::definition->define(dpy_Transaction_Type, m);
}

}
}
