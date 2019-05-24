#ifndef DBALLE_PYTHON_DATA_H
#define DBALLE_PYTHON_DATA_H

#include <Python.h>
#include <dballe/data.h>
#include <memory>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::Data* data;
} dpy_Data;

extern PyTypeObject* dpy_Data_Type;

#define dpy_Data_Check(ob) \
    (Py_TYPE(ob) == dpy_Data_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_Data_Type))

}


namespace dballe {
namespace python {

/**
 * Create a new dpy_Explorer
 */
dpy_Data* data_create();

/**
 * Create a new dpy_Explorer, taking over memory management
 */
dpy_Data* data_create(std::unique_ptr<dballe::Data> data);

void register_data(PyObject* m);

}
}

#endif

