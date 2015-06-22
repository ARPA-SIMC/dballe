#include <Python.h>

namespace wreport {
struct Vartable;
}

extern "C" {

typedef struct {
    PyObject_HEAD
    const wreport::Vartable* table;
} dpy_Vartable;

PyAPI_DATA(PyTypeObject) dpy_Vartable_Type;

#define dpy_Vartable_Check(ob) \
    (Py_TYPE(ob) == &dpy_Vartable_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_Vartable_Type))

}

namespace dballe {
namespace python {

void register_vartable(PyObject* m);

}
}
