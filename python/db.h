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
    dballe::db::Transaction* db;
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
dpy_Transaction* transaction_create(std::unique_ptr<dballe::db::Transaction> transaction);

/**
 * Copy varcodes from a Python sequence to a db::AttrList
 */
int db_read_attrlist(PyObject* attrs, db::AttrList& codes);

void register_db(PyObject* m);

bool db_load_fileobj(DB* db, PyObject* obj);

bool db_load_filelike(DB* db, PyObject* obj);

bool db_load_fileno(DB* db, PyObject* obj);

}
}

#endif
