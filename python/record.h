#ifndef DBALLE_PYTHON_RECORD_H
#define DBALLE_PYTHON_RECORD_H

#include <Python.h>

namespace dballe {
struct Record;
}

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::Record* rec;
    bool station_context;
} dpy_Record;

PyAPI_DATA(PyTypeObject) dpy_Record_Type;

#define dpy_Record_Check(ob) \
    (Py_TYPE(ob) == &dpy_Record_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_Record_Type))
}

namespace dballe {
namespace python {

dpy_Record* record_create();

void register_record(PyObject* m);

}
}
#endif
