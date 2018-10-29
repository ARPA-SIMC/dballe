#ifndef DBALLE_PYTHON_RECORD_H
#define DBALLE_PYTHON_RECORD_H

#include <Python.h>
#include <dballe/fwd.h>

namespace dballe {
struct Record;
}

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::Record* rec;
} dpy_Record;

PyAPI_DATA(PyTypeObject) dpy_Record_Type;

#define dpy_Record_Check(ob) \
    (Py_TYPE(ob) == &dpy_Record_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_Record_Type))
}

namespace dballe {
namespace python {

class RecordAccess
{
protected:
    dballe::Record* temp = nullptr;
    dballe::Record* result = nullptr;

public:
    RecordAccess() = default;
    RecordAccess(const RecordAccess&) = delete;
    RecordAccess(RecordAccess&&) = delete;
    ~RecordAccess();
    RecordAccess& operator=(const RecordAccess&) = delete;
    RecordAccess& operator=(RecordAccess&&) = delete;

    int init(PyObject*);

    operator dballe::Record&() { return *result; }
    operator const dballe::Record&() const { return *result; }
};

int read_query(PyObject* from_python, dballe::Query& query);

dpy_Record* record_create();

void register_record(PyObject* m);

}
}
#endif
