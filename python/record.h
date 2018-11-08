#ifndef DBALLE_PYTHON_RECORD_H
#define DBALLE_PYTHON_RECORD_H

#include <Python.h>
#include <dballe/fwd.h>
#include <dballe/core/fwd.h>

namespace dballe {
struct Record;
}

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::core::Record* rec;
} dpy_Record;

extern PyTypeObject* dpy_Record_Type;

#define dpy_Record_Check(ob) \
    (Py_TYPE(ob) == dpy_Record_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_Record_Type))
}

namespace dballe {
namespace python {

/**
 * Access a Record from a python object.
 *
 * If the object is a dpy_Record object, returns a reference to the record inside.
 *
 * Otherwise, create a temporary Record
 */
class RecordAccess
{
protected:
    dballe::core::Record* temp = nullptr;
    dballe::core::Record* result = nullptr;

public:
    RecordAccess(PyObject*);
    RecordAccess(const RecordAccess&) = delete;
    RecordAccess(RecordAccess&&) = delete;
    ~RecordAccess();
    RecordAccess& operator=(const RecordAccess&) = delete;
    RecordAccess& operator=(RecordAccess&&) = delete;

    operator dballe::core::Record&() { return *result; }
    operator const dballe::core::Record&() const { return *result; }
};

void read_query(PyObject* from_python, dballe::Query& query);
bool _record_enqpython(const dballe::core::Record& rec, const char* key, unsigned len, PyObject*& result);

dpy_Record* record_create();

void register_record(PyObject* m);

}
}
#endif
