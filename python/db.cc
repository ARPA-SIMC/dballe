#include <Python.h>
#include "db.h"
#include "record.h"
#include "cursor.h"
#include "common.h"
#include "dballe/types.h"
#include "dballe/file.h"
#include "dballe/core/query.h"
#include "dballe/core/values.h"
#include "dballe/message.h"
#include "dballe/msg/codec.h"
#include <algorithm>
#include <wreport/bulletin.h>

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

template<typename Vals>
static PyObject* get_insert_ids(const Vals& vals)
{
    pyo_unique_ptr res(PyDict_New());

    pyo_unique_ptr ana_id(PyInt_FromLong(vals.info.ana_id));
    if (!ana_id) return nullptr;
    if (PyDict_SetItemString(res, "ana_id", ana_id))
        return nullptr;

    for (const auto& v: vals.values)
    {
        pyo_unique_ptr id(PyInt_FromLong(v.second.data_id));
        pyo_unique_ptr varcode(format_varcode(v.first));

        if (PyDict_SetItem(res, varcode, id))
            return nullptr;
    }

    return res.release();
}

extern "C" {

static PyGetSetDef dpy_DB_getsetters[] = {
    //{"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    //{"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {NULL}
};

static PyObject* dpy_DB_connect(PyTypeObject *type, PyObject *args, PyObject* kw)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.connect_from_url instead of DB.connect", 1))
        return NULL;
    static const char* kwlist[] = { "dsn", "user", "password", NULL };
    const char* dsn;
    const char* user = "";
    const char* pass = "";
    if (!PyArg_ParseTupleAndKeywords(args, kw, "s|ss", const_cast<char**>(kwlist), &dsn, &user, &pass))
        return NULL;

    unique_ptr<DB> db;
    try {
        db = DB::connect(dsn, user, pass);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(move(db));
}

static PyObject* dpy_DB_connect_from_file(PyTypeObject *type, PyObject *args)
{
    const char* fname;
    if (!PyArg_ParseTuple(args, "s", &fname))
        return NULL;

    unique_ptr<DB> db;
    try {
        db = DB::connect_from_file(fname);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(move(db));
}

static PyObject* dpy_DB_connect_from_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    unique_ptr<DB> db;
    try {
        db = DB::connect_from_url(url);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(move(db));
}

static PyObject* dpy_DB_connect_test(PyTypeObject *type)
{
    unique_ptr<DB> db;
    try {
        db = DB::connect_test();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(move(db));
}

static PyObject* dpy_DB_is_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    if (DB::is_url(url))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject* dpy_DB_reset(dpy_DB* self, PyObject *args)
{
    const char* repinfo_file = 0;
    if (!PyArg_ParseTuple(args, "|s", &repinfo_file))
        return NULL;

    try {
        self->db->reset(repinfo_file);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

/*
virtual void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated) = 0;
*/

static PyObject* dpy_DB_insert(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { "record", "can_replace", "can_add_stations", NULL };
    dpy_Record* record;
    int can_replace = 0;
    int station_can_add = 0;
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.insert_station_data or DB.insert_data instead of DB.insert", 1))
        return NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!|ii", const_cast<char**>(kwlist), &dpy_Record_Type, &record, &can_replace, &station_can_add))
        return NULL;

    try {
        if (record->station_context)
        {
            StationValues vals(*record->rec);
            self->db->insert_station_data(vals, can_replace, station_can_add);
        }
        else
        {
            DataValues vals(*record->rec);
            self->db->insert_data(vals, can_replace, station_can_add);
        }
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_insert_station_data(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { "record", "can_replace", "can_add_stations", NULL };
    dpy_Record* record;
    int can_replace = 0;
    int station_can_add = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!|ii", const_cast<char**>(kwlist), &dpy_Record_Type, &record, &can_replace, &station_can_add))
        return NULL;

    try {
        StationValues vals(*record->rec);
        self->db->insert_station_data(vals, can_replace, station_can_add);
        return get_insert_ids(vals);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_insert_data(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { "record", "can_replace", "can_add_stations", NULL };
    dpy_Record* record;
    int can_replace = 0;
    int station_can_add = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!|ii", const_cast<char**>(kwlist), &dpy_Record_Type, &record, &can_replace, &station_can_add))
        return NULL;

    try {
        DataValues vals(*record->rec);
        self->db->insert_data(vals, can_replace, station_can_add);
        return get_insert_ids(vals);
    } DBALLE_CATCH_RETURN_PYO
}

static unsigned db_load_file_enc(DB* db, File::Encoding encoding, FILE* file, bool close_on_exit, const std::string& name)
{
    std::unique_ptr<File> f = File::create(encoding, file, close_on_exit, name);
    std::unique_ptr<msg::Importer> imp = msg::Importer::create(f->encoding());
    unsigned count = 0;
    f->foreach([&](const BinaryMessage& raw) {
        Messages msgs = imp->from_binary(raw);
        db->import_msgs(msgs, NULL, 0);
        ++count;
        return true;
    });
    return count;
}

static unsigned db_load_file(DB* db, FILE* file, bool close_on_exit, const std::string& name)
{
    std::unique_ptr<File> f = File::create(file, close_on_exit, name);
    std::unique_ptr<msg::Importer> imp = msg::Importer::create(f->encoding());
    unsigned count = 0;
    f->foreach([&](const BinaryMessage& raw) {
        Messages msgs = imp->from_binary(raw);
        db->import_msgs(msgs, NULL, 0);
        ++count;
        return true;
    });
    return count;
}

static PyObject* dpy_DB_load(dpy_DB* self, PyObject* args)
{
    PyObject* obj;
    const char* encoding = nullptr;
    if (!PyArg_ParseTuple(args, "O|s", &obj, &encoding))
        return nullptr;

    string repr;
    if (object_repr(obj, repr))
        return nullptr;

    try {
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
                count = db_load_file_enc(self->db, File::parse_encoding(encoding), f, true, repr);
            } else
                count = db_load_file(self->db, f, true, repr);
            return PyInt_FromLong(count);
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
                count = db_load_file_enc(self->db, File::parse_encoding(encoding), f, true, repr);
            } else
                count = db_load_file(self->db, f, true, repr);
            return PyInt_FromLong(count);
        }
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}


static PyObject* dpy_DB_remove_station_data(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    // TODO: if it is a dict, turn it directly into a Query?

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        self->db->remove_station_data(query);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_remove(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    // TODO: if it is a dict, turn it directly into a Query?

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        if (record->station_context)
        {
            if (PyErr_WarnEx(PyExc_DeprecationWarning, "DB.remove after Record.set_station_context is deprecated in favour of using DB.remove_station_data", 1))
                return NULL;
            self->db->remove_station_data(query);
        } else
            self->db->remove(query);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_disappear(dpy_DB* self)
{
    try {
        self->db->disappear();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_vacuum(dpy_DB* self)
{
    try {
        self->db->vacuum();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_query_stations(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    // TODO: if it is a dict, turn it directly into a Query?

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        std::unique_ptr<db::Cursor> res = self->db->query_stations(query);
        return (PyObject*)cursor_create(self, move(res));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_data(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    // TODO: if it is a dict, turn it directly into a Query?

    //self->db->dump(stderr);

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        std::unique_ptr<db::Cursor> res;
        if (record->station_context)
        {
            if (PyErr_WarnEx(PyExc_DeprecationWarning, "DB.query_data after Record.set_station_context is deprecated in favour of using DB.query_station_data", 1))
                return NULL;
            res = self->db->query_station_data(query);
        } else
            res = self->db->query_data(query);
        return (PyObject*)cursor_create(self, move(res));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_station_data(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    // TODO: if it is a dict, turn it directly into a Query?

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        std::unique_ptr<db::Cursor> res = self->db->query_station_data(query);
        return (PyObject*)cursor_create(self, move(res));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_summary(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    // TODO: if it is a dict, turn it directly into a Query?

    try {
        core::Query query;
        query.set_from_record(*record->rec);
        std::unique_ptr<db::Cursor> res = self->db->query_summary(query);
        return (PyObject*)cursor_create(self, move(res));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_attrs(dpy_DB* self, PyObject* args, PyObject* kw)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.attr_query_station or DB.attr_query_data instead of DB.query_attrs", 1))
        return NULL;

    static const char* kwlist[] = { "varcode", "reference_id", "attrs", NULL };
    int reference_id;
    const char* varname;
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", const_cast<char**>(kwlist), &varname, &reference_id, &attrs))
        return NULL;

    // Read the attribute list, if provided
    db::AttrList codes;
    if (db_read_attrlist(attrs, codes))
        return NULL;

    self->attr_rec->rec->clear();

    try {
        wreport::Varcode varcode = resolve_varcode(varname);
        if (self->db->is_station_variable(reference_id, varcode))
            self->db->attr_query_station(reference_id, [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                self->attr_rec->rec->set(move(var));
            });
        else
            self->db->attr_query_data(reference_id, [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                self->attr_rec->rec->set(move(var));
            });
        Py_INCREF(self->attr_rec);
        return (PyObject*)self->attr_rec;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_query_station(dpy_DB* self, PyObject* args)
{
    int reference_id;
    if (!PyArg_ParseTuple(args, "i", &reference_id))
        return NULL;

    self->attr_rec->rec->clear();
    try {
        self->db->attr_query_station(reference_id, [&](unique_ptr<Var> var) {
            self->attr_rec->rec->set(move(var));
        });
        Py_INCREF(self->attr_rec);
        return (PyObject*)self->attr_rec;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_query_data(dpy_DB* self, PyObject* args)
{
    int reference_id;
    if (!PyArg_ParseTuple(args, "i", &reference_id))
        return NULL;

    self->attr_rec->rec->clear();
    try {
        self->db->attr_query_data(reference_id, [&](unique_ptr<Var>&& var) {
            self->attr_rec->rec->set(move(var));
        });
        Py_INCREF(self->attr_rec);
        return (PyObject*)self->attr_rec;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_insert(dpy_DB* self, PyObject* args, PyObject* kw)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.attr_insert_station or DB.attr_insert_data instead of DB.attr_insert", 1))
        return NULL;

    static const char* kwlist[] = { "varcode", "attrs", "reference_id", NULL };
    int reference_id = -1;
    const char* varname;
    dpy_Record* record;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "sO!|i", const_cast<char**>(kwlist),
                &varname,
                &dpy_Record_Type, &record,
                &reference_id))
        return NULL;

    if (reference_id == -1)
    {
        PyErr_SetString(PyExc_ValueError, "please provide a reference_id argument: implicitly reusing the one from the last insert is not supported anymore");
        return NULL;
    }

    try {
        if (self->db->is_station_variable(reference_id, resolve_varcode(varname)))
            self->db->attr_insert_data(reference_id, *record->rec);
        else
            self->db->attr_insert_data(reference_id, *record->rec);
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_insert_station(dpy_DB* self, PyObject* args)
{
    int data_id;
    dpy_Record* attrs;
    if (!PyArg_ParseTuple(args, "iO!", &data_id, &dpy_Record_Type, &attrs))
        return NULL;

    try {
        self->db->attr_insert_station(data_id, *attrs->rec);
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_insert_data(dpy_DB* self, PyObject* args)
{
    int data_id;
    dpy_Record* attrs;
    if (!PyArg_ParseTuple(args, "iO!", &data_id, &dpy_Record_Type, &attrs))
        return NULL;

    try {
        self->db->attr_insert_data(data_id, *attrs->rec);
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_remove(dpy_DB* self, PyObject* args, PyObject* kw)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.attr_remove_station or DB.attr_remove_data instead of DB.attr_remove", 1))
        return NULL;

    static const char* kwlist[] = { "varcode", "reference_id", "attrs", NULL };
    int reference_id;
    const char* varname;
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", const_cast<char**>(kwlist), &varname, &reference_id, &attrs))
        return NULL;

    // Read the attribute list, if provided
    db::AttrList codes;
    if (db_read_attrlist(attrs, codes))
        return NULL;

    try {
        if (self->db->is_station_variable(reference_id, resolve_varcode(varname)))
            self->db->attr_remove_station(reference_id, codes);
        else
            self->db->attr_remove_data(reference_id, codes);
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_remove_station(dpy_DB* self, PyObject* args)
{
    int reference_id;
    PyObject* attrs = 0;
    if (!PyArg_ParseTuple(args, "i|O", &reference_id, &attrs))
        return NULL;

    // Read the attribute list, if provided
    db::AttrList codes;
    if (db_read_attrlist(attrs, codes))
        return NULL;

    try {
        self->db->attr_remove_station(reference_id, codes);
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_attr_remove_data(dpy_DB* self, PyObject* args)
{
    int reference_id;
    PyObject* attrs = 0;
    if (!PyArg_ParseTuple(args, "i|O", &reference_id, &attrs))
        return NULL;

    // Read the attribute list, if provided
    db::AttrList codes;
    if (db_read_attrlist(attrs, codes))
        return NULL;

    try {
        self->db->attr_remove_data(reference_id, codes);
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_DB_export_to_file(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static const char* kwlist[] = { "query", "format", "filename", "generic", NULL };
    dpy_Record* query;
    const char* format;
    const char* filename;
    int as_generic = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!ss|i", const_cast<char**>(kwlist), &dpy_Record_Type, &query, &format, &filename, &as_generic))
        return NULL;

    File::Encoding encoding = File::BUFR;
    if (strcmp(format, "BUFR") == 0)
        encoding = File::BUFR;
    else if (strcmp(format, "CREX") == 0)
        encoding = File::CREX;
    else
    {
        PyErr_SetString(PyExc_ValueError, "encoding must be one of BUFR or CREX");
        return NULL;
    }

    try {
        std::unique_ptr<File> out = File::create(encoding, filename, "wb");
        msg::Exporter::Options opts;
        if (as_generic)
            opts.template_name = "generic";
        auto exporter = msg::Exporter::create(out->encoding(), opts);
        auto q = Query::create();
        q->set_from_record(*query->rec);
        self->db->export_msgs(*q, [&](unique_ptr<Message>&& msg) {
            Messages msgs;
            msgs.append(move(msg));
            out->write(exporter->to_binary(msgs));
            return true;
        });
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyMethodDef dpy_DB_methods[] = {
    {"connect",           (PyCFunction)dpy_DB_connect, METH_VARARGS | METH_KEYWORDS | METH_CLASS,
        "(deprecated) Create a DB connecting to an ODBC source" },
    {"connect_from_file", (PyCFunction)dpy_DB_connect_from_file, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to a SQLite file" },
    {"connect_from_url",  (PyCFunction)dpy_DB_connect_from_url, METH_VARARGS | METH_CLASS,
        "Create a DB as defined in an URL-like string" },
    {"connect_test",      (PyCFunction)dpy_DB_connect_test, METH_NOARGS | METH_CLASS,
        "Create a DB for running the test suite, as configured in the test environment" },
    {"is_url",            (PyCFunction)dpy_DB_is_url, METH_VARARGS | METH_CLASS,
        "Checks if a string looks like a DB url" },
    {"disappear",         (PyCFunction)dpy_DB_disappear, METH_NOARGS,
        "Remove all our traces from the database, if applicable." },
    {"reset",             (PyCFunction)dpy_DB_reset, METH_VARARGS,
        "Reset the database, removing all existing Db-All.e tables and re-creating them empty." },
    {"insert",            (PyCFunction)dpy_DB_insert, METH_VARARGS | METH_KEYWORDS,
        "(deprecated)Insert a record in the database" },
    {"insert_station_data", (PyCFunction)dpy_DB_insert_station_data, METH_VARARGS | METH_KEYWORDS,
        "Insert station values in the database" },
    {"insert_data",       (PyCFunction)dpy_DB_insert_data, METH_VARARGS | METH_KEYWORDS,
        "Insert data values in the database" },
    {"load",              (PyCFunction)dpy_DB_load, METH_VARARGS, R"(
        load(fp, encoding=None)

        Load a file object in the database. An encoding can optionally be
        provided as a string ("BUFR", "CREX", "AOF"). If encoding is None then
        load will try to autodetect based on the first byte of the file.
    )" },
    {"remove_station_data", (PyCFunction)dpy_DB_remove_station_data, METH_VARARGS,
        "Remove station variables from the database" },
    {"remove",            (PyCFunction)dpy_DB_remove, METH_VARARGS,
        "Remove variables from the database" },
    {"vacuum",            (PyCFunction)dpy_DB_vacuum, METH_NOARGS,
        "Perform database cleanup operations" },
    {"query_stations",    (PyCFunction)dpy_DB_query_stations, METH_VARARGS,
        "Query the station archive in the database; returns a Cursor" },
    {"query_station_data", (PyCFunction)dpy_DB_query_station_data, METH_VARARGS,
        "Query the station variables in the database; returns a Cursor" },
    {"query_data",        (PyCFunction)dpy_DB_query_data, METH_VARARGS,
        "Query the variables in the database; returns a Cursor" },
    {"query_summary",     (PyCFunction)dpy_DB_query_summary, METH_VARARGS,
        "Query the summary of the results of a query; returns a Cursor" },
    {"query_attrs",       (PyCFunction)dpy_DB_query_attrs, METH_VARARGS | METH_KEYWORDS,
        "Query attributes" },
    {"attr_query_station", (PyCFunction)dpy_DB_attr_query_station, METH_VARARGS,
        "Query attributes" },
    {"attr_query_data",   (PyCFunction)dpy_DB_attr_query_data, METH_VARARGS,
        "Query attributes" },
    {"attr_insert",       (PyCFunction)dpy_DB_attr_insert, METH_VARARGS | METH_KEYWORDS,
        "Insert new attributes into the database" },
    {"attr_insert_station", (PyCFunction)dpy_DB_attr_insert_station, METH_VARARGS,
        "Insert new attributes into the database" },
    {"attr_insert_data",  (PyCFunction)dpy_DB_attr_insert_data, METH_VARARGS,
        "Insert new attributes into the database" },
    {"attr_remove",       (PyCFunction)dpy_DB_attr_remove, METH_VARARGS | METH_KEYWORDS,
        "Remove attributes" },
    {"attr_remove_station", (PyCFunction)dpy_DB_attr_remove_station, METH_VARARGS,
        "Remove attributes" },
    {"attr_remove_data",  (PyCFunction)dpy_DB_attr_remove_data, METH_VARARGS,
        "Remove attributes" },
    {"export_to_file",    (PyCFunction)dpy_DB_export_to_file, METH_VARARGS | METH_KEYWORDS,
        "Export data matching a query as bulletins to a named file" },
    {NULL}
};

static int dpy_DB_init(dpy_DB* self, PyObject* args, PyObject* kw)
{
    // People should not invoke DB() as a constructor, but if they do,
    // this is better than a segfault later on
    PyErr_SetString(PyExc_NotImplementedError, "DB objects cannot be constructed explicitly");
    return -1;
}

static void dpy_DB_dealloc(dpy_DB* self)
{
    if (self->db)
        delete self->db;
}

static PyObject* dpy_DB_str(dpy_DB* self)
{
    /*
    std::string f = self->var.format("None");
    return PyUnicode_FromString(f.c_str());
    */
    return PyUnicode_FromString("DB");
}

static PyObject* dpy_DB_repr(dpy_DB* self)
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
    return PyUnicode_FromString("DB object");
}

PyTypeObject dpy_DB_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.DB",               // tp_name
    sizeof(dpy_DB),            // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_DB_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_DB_repr,     // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_DB_str,      // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e DB",             // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_DB_methods,            // tp_methods
    0,                         // tp_members
    dpy_DB_getsetters,         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_DB_init,     // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

int db_read_attrlist(PyObject* attrs, db::AttrList& codes)
{
    if (!attrs) return 0;

    pyo_unique_ptr iter(PyObject_GetIter(attrs));
    if (!iter) return -1;

    try {
        while (PyObject* iter_item = PyIter_Next(iter)) {
            pyo_unique_ptr item(iter_item);
            string name;
            if (string_from_python(item, name))
                return -1;
            codes.push_back(resolve_varcode(name));
        }
        return 0;
    } DBALLE_CATCH_RETURN_INT
}

dpy_DB* db_create(std::unique_ptr<DB> db)
{
    dpy_Record* attr_rec = record_create();
    if (!attr_rec) return NULL;

    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result)
    {
        Py_DECREF(attr_rec);
        return NULL;
    }

    result->db = db.release();
    result->attr_rec = attr_rec;
    return result;
}

void register_db(PyObject* m)
{
    common_init();

    dpy_DB_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DB_Type) < 0)
        return;

    Py_INCREF(&dpy_DB_Type);
    PyModule_AddObject(m, "DB", (PyObject*)&dpy_DB_Type);
}

}
}
