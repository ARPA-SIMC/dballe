/*
 * python/cursor - DB-All.e DB cursor python bindings
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
#ifndef DBALLE_PYTHON_CURSOR_H
#define DBALLE_PYTHON_CURSOR_H

#include <Python.h>
#include <dballe/types.h>
#include <dballe/db/db.h>
#include <dballe/db/summary.h>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::db::CursorStation* cur;
} dpy_CursorStationDB;

extern PyTypeObject* dpy_CursorStationDB_Type;

#define dpy_CursorStationDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorStationDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorStationDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::CursorStationData* cur;
} dpy_CursorStationDataDB;

extern PyTypeObject* dpy_CursorStationDataDB_Type;

#define dpy_CursorStationDataDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorStationDataDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorStationDataDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::CursorData* cur;
} dpy_CursorDataDB;

extern PyTypeObject* dpy_CursorDataDB_Type;

#define dpy_CursorDataDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorDataDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorDataDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::CursorSummary* cur;
} dpy_CursorSummaryDB;

extern PyTypeObject* dpy_CursorSummaryDB_Type;

#define dpy_CursorSummaryDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorSummaryDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorSummaryDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::summary::Cursor<dballe::Station>* cur;
} dpy_CursorSummarySummary;

extern PyTypeObject* dpy_CursorSummarySummary_Type;

#define dpy_CursorSummarySummary_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorSummarySummary_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorSummarySummary_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::summary::Cursor<dballe::DBStation>* cur;
} dpy_CursorSummaryDBSummary;

extern PyTypeObject* dpy_CursorSummarySummary_Type;

#define dpy_CursorSummaryDBSummary_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorSummaryDBSummary_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorSummaryDBSummary_Type))

}

namespace dballe {
namespace python {

/**
 * Create a new dpy_Cursor, taking ownership of memory management
 */
dpy_CursorStationDB* cursor_create(std::unique_ptr<db::CursorStation> cur);
dpy_CursorStationDataDB* cursor_create(std::unique_ptr<db::CursorStationData> cur);
dpy_CursorDataDB* cursor_create(std::unique_ptr<db::CursorData> cur);
dpy_CursorSummaryDB* cursor_create(std::unique_ptr<db::CursorSummary> cur);
dpy_CursorSummarySummary* cursor_create(std::unique_ptr<db::summary::Cursor<Station>> cur);
dpy_CursorSummaryDBSummary* cursor_create(std::unique_ptr<db::summary::Cursor<DBStation>> cur);

void register_cursor(PyObject* m);

}
}
#endif
