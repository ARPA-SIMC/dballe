#ifndef DBALLE_PYTHON_DB_H
#define DBALLE_PYTHON_DB_H

#include <Python.h>
#include <dballe/db/db.h>
#include "record.h"

extern "C" {

typedef struct {
    PyObject_HEAD
    std::shared_ptr<dballe::DB> db;
} dpy_DB;

PyAPI_DATA(PyTypeObject) dpy_DB_Type;

#define dpy_DB_Check(ob) \
    (Py_TYPE(ob) == &dpy_DB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_DB_Type))


typedef struct {
    PyObject_HEAD
    std::shared_ptr<dballe::db::Transaction> db;
} dpy_Transaction;

PyAPI_DATA(PyTypeObject) dpy_Transaction_Type;

#define dpy_Transaction_Check(ob) \
    (Py_TYPE(ob) == &dpy_Transaction_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_Transaction_Type))

}

namespace dballe {
namespace python {

/**
 * Create a new dpy_DB, sharing memory management
 */
dpy_DB* db_create(std::shared_ptr<DB> db);

/**
 * Create a new dpy_Transaction, taking over memory management
 */
dpy_Transaction* transaction_create(std::shared_ptr<dballe::db::Transaction> transaction);

/**
 * Copy varcodes from a Python sequence to a db::AttrList
 */
int db_read_attrlist(PyObject* attrs, db::AttrList& codes);

bool db_load_fileobj(DB* db, PyObject* obj);

bool db_load_filelike(DB* db, PyObject* obj);

bool db_load_fileno(DB* db, PyObject* obj);

void register_db(PyObject* m);

}
}

#endif
