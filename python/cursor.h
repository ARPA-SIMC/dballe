#ifndef DBALLE_PYTHON_CURSOR_H
#define DBALLE_PYTHON_CURSOR_H

#include <Python.h>
#include <dballe/types.h>
#include <dballe/db/db.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/db/summary.h>
#include <dballe/core/var.h>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::db::v7::cursor::Stations* cur;
} dpy_CursorStationDB;

extern PyTypeObject* dpy_CursorStationDB_Type;

#define dpy_CursorStationDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorStationDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorStationDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::v7::cursor::StationData* cur;
} dpy_CursorStationDataDB;

extern PyTypeObject* dpy_CursorStationDataDB_Type;

#define dpy_CursorStationDataDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorStationDataDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorStationDataDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::v7::cursor::Data* cur;
} dpy_CursorDataDB;

extern PyTypeObject* dpy_CursorDataDB_Type;

#define dpy_CursorDataDB_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorDataDB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorDataDB_Type))


typedef struct {
    PyObject_HEAD
    dballe::db::v7::cursor::Summary* cur;
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


typedef struct {
    PyObject_HEAD
    dballe::impl::CursorMessage* cur;
    PyObject* curmsg;
} dpy_CursorMessage;

extern PyTypeObject* dpy_CursorMessage_Type;

#define dpy_CursorMessage_Check(ob) \
    (Py_TYPE(ob) == dpy_CursorMessage_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_CursorMessage_Type))

}

namespace dballe {
namespace python {

void _update_curmsg(dpy_CursorMessage& cur);

PyObject* enqpy(db::CursorStation& cur, const char* key, unsigned len);
PyObject* enqpy(db::CursorStationData& cur, const char* key, unsigned len);
PyObject* enqpy(db::CursorData& cur, const char* key, unsigned len);
PyObject* enqpy(db::CursorSummary& cur, const char* key, unsigned len);
template<typename Station>
PyObject* enqpy(db::summary::Cursor<Station>& cur, const char* key, unsigned len);
extern template PyObject* enqpy(db::summary::Cursor<Station>& cur, const char* key, unsigned len);
extern template PyObject* enqpy(db::summary::Cursor<DBStation>& cur, const char* key, unsigned len);
PyObject* enqpy(CursorMessage& cur, const char* key, unsigned len);

/**
 * Create a new dpy_Cursor, taking ownership of memory management
 */
dpy_CursorStationDB* cursor_create(std::unique_ptr<db::v7::cursor::Stations> cur);
dpy_CursorStationDataDB* cursor_create(std::unique_ptr<db::v7::cursor::StationData> cur);
dpy_CursorDataDB* cursor_create(std::unique_ptr<db::v7::cursor::Data> cur);
dpy_CursorSummaryDB* cursor_create(std::unique_ptr<db::v7::cursor::Summary> cur);
dpy_CursorSummarySummary* cursor_create(std::unique_ptr<db::summary::Cursor<Station>> cur);
dpy_CursorSummaryDBSummary* cursor_create(std::unique_ptr<db::summary::Cursor<DBStation>> cur);
dpy_CursorMessage* cursor_create(std::unique_ptr<dballe::CursorMessage> cur);

void register_cursor(PyObject* m);

}
}
#endif
