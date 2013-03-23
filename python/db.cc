/*
 * python/db - DB-All.e DB python bindings
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include <Python.h>
#include <datetime.h>
#include "db.h"
#include "record.h"
#include "cursor.h"
#include "common.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

/*
static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val);
static int dpy_Record_contains(dpy_Record* self, PyObject *value);
static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key);

static PyObject* dpy_Var_code(dpy_Var* self, void* closure) { return format_varcode(self->var.code()); }
static PyObject* dpy_Var_isset(dpy_Var* self, void* closure) {
    if (self->var.isset())
        return Py_True;
    else
        return Py_False;
}
*/

static PyGetSetDef dpy_DB_getsetters[] = {
    //{"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    //{"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {NULL}
};

static PyObject* dpy_DB_connect(PyTypeObject *type, PyObject *args)
{
    const char* dsn;
    const char* user = "";
    const char* pass = "";
    if (!PyArg_ParseTuple(args, "s|ss", &dsn, &user, &pass))
        return NULL;

    auto_ptr<DB> db;
    try {
        db = DB::connect(dsn, user, pass);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_connect_from_file(PyTypeObject *type, PyObject *args)
{
    const char* fname;
    if (!PyArg_ParseTuple(args, "s", &fname))
        return NULL;

    auto_ptr<DB> db;
    try {
        db = DB::connect_from_file(fname);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_connect_from_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    auto_ptr<DB> db;
    try {
        db = DB::connect_from_url(url);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_connect_test(PyTypeObject *type)
{
    auto_ptr<DB> db;
    try {
        db = DB::connect_test();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_is_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    if (DB::is_url(url))
        Py_RETURN_TRUE;
    else
        Py_RETURN_TRUE;
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
virtual const std::string& rep_memo_from_cod(int rep_cod) = 0;
*/

static PyObject* dpy_DB_insert(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "record", "can_replace", "station_can_add", NULL };
    dpy_Record* record;
    int can_replace = 0;
    int station_can_add = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!|ii", kwlist, &dpy_Record_Type, &record, &can_replace, &station_can_add))
        return NULL;

    try {
        int res = self->db->insert(record->rec, can_replace, station_can_add);
        return PyInt_FromLong(res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_remove(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        self->db->remove(record->rec);
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

static PyObject* dpy_DB_query_reports(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_reports(record->rec);
        return (PyObject*)cursor_create(res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_stations(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_stations(record->rec);
        return (PyObject*)cursor_create(res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_levels(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_levels(record->rec);
        return (PyObject*)cursor_create(res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_tranges(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_tranges(record->rec);
        return (PyObject*)cursor_create(res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_variable_types(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_variable_types(record->rec);
        return (PyObject*)cursor_create(res);
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

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_data(record->rec);
        return (PyObject*)cursor_create(res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}
    /*
     * Query attributes
     *
     * @param reference_id
     *   The id (returned by Cursor::attr_reference_id()) used to refer to the variable we query
     * @param id_var
     *   The varcode of the variable related to the attributes to retrieve.  See @ref vartable.h
     * @param qcs
     *   The WMO codes of the QC values requested.  If it is empty, then all values
     *   are returned.
     * @param attrs
     *   The Record that will hold the resulting attributes
     * @return
    virtual unsigned query_attrs(int reference_id, wreport::Varcode id_var, const db::AttrList& qcs, Record& attrs) = 0;
    */
static PyObject* dpy_DB_query_attrs(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "reference_id", "varcode", "attrs", NULL };
    int reference_id;
    const char* varname;
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "is|O", kwlist, &reference_id, &varname, &attrs))
        return NULL;

    wreport::Varcode varcode = resolve_varcode(varname);

    // Read the attribute list, if provided
    db::AttrList qcs;
    if (attrs)
    {
        OwnedPyObject iter(PyObject_GetIter(attrs));
        if (iter == NULL)
            return NULL;
        while (PyObject* iter_item = PyIter_Next(iter)) {
            OwnedPyObject item(iter_item);
            const char* name = PyString_AsString(item);
            if (!name) return NULL;
            qcs.push_back(resolve_varcode(name));
        }
    }

    try {
        self->db->query_attrs(reference_id, varcode, qcs, self->attr_rec->rec);
        Py_INCREF(self->attr_rec);
        return (PyObject*)self->attr_rec;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_attr_insert(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "reference_id", "varcode", "attrs", "replace", NULL };
    int reference_id;
    const char* varname;
    dpy_Record* record;
    int can_replace = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "isO!|i", kwlist,
                &reference_id,
                &varname,
                &dpy_Record_Type, &record,
                &can_replace))
        return NULL;

    try {
        self->db->attr_insert(reference_id, resolve_varcode(varname), record->rec, can_replace);
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

    /*
    virtual void attr_remove(int id_context, wreport::Varcode id_var, const db::AttrList& qcs) = 0;
    virtual void import_msg(const Msg& msg, const char* repmemo, int flags) = 0;
    virtual void import_msgs(const Msgs& msgs, const char* repmemo, int flags) = 0;
    virtual void export_msgs(const Record& query, MsgConsumer& cons) = 0;
    virtual void dump(FILE* out) = 0;
    */

static PyMethodDef dpy_DB_methods[] = {
    {"connect",           (PyCFunction)dpy_DB_connect, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to an ODBC source" },
    {"connect_from_file", (PyCFunction)dpy_DB_connect_from_file, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to a SQLite file" },
    {"connect_from_url",  (PyCFunction)dpy_DB_connect_from_url, METH_VARARGS | METH_CLASS,
        "Create a DB as defined in an URL-like string" },
    {"connect_test",      (PyCFunction)dpy_DB_connect_test, METH_NOARGS | METH_CLASS,
        "Create a DB for running the test suite, as configured in the test environment" },
    {"is_url",            (PyCFunction)dpy_DB_is_url, METH_VARARGS | METH_CLASS,
        "Checks if a string looks like a DB url" },
    {"reset",             (PyCFunction)dpy_DB_reset, METH_VARARGS,
        "Reset the database, removing all existing Db-All.e tables and re-creating them empty." },
    {"insert",            (PyCFunction)dpy_DB_insert, METH_VARARGS | METH_KEYWORDS,
        "Insert a record in the database" },
    {"remove",            (PyCFunction)dpy_DB_remove, METH_VARARGS,
        "Remove records from the database" },
    {"vacuum",            (PyCFunction)dpy_DB_vacuum, METH_NOARGS,
        "Perform database cleanup operations" },
    {"query_reports",    (PyCFunction)dpy_DB_query_reports, METH_VARARGS,
        "Query the report information archive in the database; returns a Cursor" },
    {"query_stations",    (PyCFunction)dpy_DB_query_stations, METH_VARARGS,
        "Query the station archive in the database; returns a Cursor" },
    {"query_levels",      (PyCFunction)dpy_DB_query_levels, METH_VARARGS,
        "Query the level archive in the database; returns a Cursor" },
    {"query_tranges",     (PyCFunction)dpy_DB_query_tranges, METH_VARARGS,
        "Query the time range archive in the database; returns a Cursor" },
    {"query_variable_types", (PyCFunction)dpy_DB_query_variable_types, METH_VARARGS,
        "Query the variable types in the database; returns a Cursor" },
    {"query_data",        (PyCFunction)dpy_DB_query_data, METH_VARARGS,
        "Query the variables in the database; returns a Cursor" },
    {"attr_insert",       (PyCFunction)dpy_DB_attr_insert, METH_VARARGS | METH_KEYWORDS,
        "Insert new attributes into the database" },
    {"query_attrs",       (PyCFunction)dpy_DB_query_attrs, METH_VARARGS | METH_KEYWORDS,
        "Query attributes" },
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
    return PyString_FromString(f.c_str());
    */
    return PyString_FromString("DB");
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
    return PyString_FromString(res.c_str());
    */
    return PyString_FromString("DB object");
}

PyTypeObject dpy_DB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
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

dpy_DB* db_create(std::auto_ptr<DB> db)
{
    dpy_Record* attr_rec = record_create();
    if (!attr_rec) return NULL;

    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result)
    {
        Py_DECREF(attr_rec);
        return NULL;
    }

    result = (dpy_DB*)PyObject_Init((PyObject*)result, &dpy_DB_Type);
    result->db = db.release();
    result->attr_rec = attr_rec;
    return result;
}

void register_db(PyObject* m)
{
    PyDateTime_IMPORT;

    dpy_DB_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DB_Type) < 0)
        return;

    Py_INCREF(&dpy_DB_Type);
    PyModule_AddObject(m, "DB", (PyObject*)&dpy_DB_Type);
}

}
}

