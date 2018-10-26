#ifndef DBALLE_PYTHON_BINARYMESSAGE_H
#define DBALLE_PYTHON_BINARYMESSAGE_H

#include <Python.h>
#include <dballe/file.h>
#include <memory>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::BinaryMessage message;
} dpy_BinaryMessage;

PyAPI_DATA(PyTypeObject) dpy_BinaryMessage_Type;

#define dpy_BinaryMessage_Check(ob) \
    (Py_TYPE(ob) == &dpy_BinaryMessage_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_BinaryMessage_Type))

}


namespace dballe {
namespace python {

/**
 * Create a new dpy_BinaryMessage
 */
dpy_BinaryMessage* binarymessage_create(const BinaryMessage& message);

/**
 * Create a new dpy_BinaryMessage
 */
dpy_BinaryMessage* binarymessage_create(BinaryMessage&& message);

int register_binarymessage(PyObject* m);

}
}

#endif

