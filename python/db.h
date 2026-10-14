#ifndef DBALLE_PYTHON_DB_H
#define DBALLE_PYTHON_DB_H

#include "utils/core.h"
#include <dballe/db/db.h>

extern "C" {

typedef struct
{
    PyObject_HEAD std::shared_ptr<dballe::db::DB> db;
} dpy_DB;

extern PyTypeObject* dpy_DB_Type;

#define dpy_DB_Check(ob)                                                       \
    (Py_TYPE(ob) == dpy_DB_Type || PyType_IsSubtype(Py_TYPE(ob), dpy_DB_Type))

typedef struct
{
    PyObject_HEAD std::shared_ptr<dballe::db::Transaction> db;
} dpy_Transaction;

extern PyTypeObject* dpy_Transaction_Type;

#define dpy_Transaction_Check(ob)                                              \
    (Py_TYPE(ob) == dpy_Transaction_Type ||                                    \
     PyType_IsSubtype(Py_TYPE(ob), dpy_Transaction_Type))
}

namespace dballe {
namespace python {

/**
 * Create a new dpy_DB, sharing memory management
 */
dpy_DB* db_create(std::shared_ptr<db::DB> db);

/**
 * Create a new dpy_Transaction, taking over memory management
 */
dpy_Transaction*
transaction_create(std::shared_ptr<dballe::db::Transaction> transaction);

/**
 * Copy varcodes from a Python sequence to a db::AttrList
 */
db::AttrList db_read_attrlist(PyObject* attrs);

void register_db(PyObject* m);

} // namespace python
} // namespace dballe
#endif
