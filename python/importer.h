#ifndef DBALLE_PYTHON_IMPORTER_H
#define DBALLE_PYTHON_IMPORTER_H

#include <Python.h>
#include <dballe/importer.h>
#include <memory>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::Importer* importer;
} dpy_Importer;

extern PyTypeObject* dpy_Importer_Type;

#define dpy_Importer_Check(ob) \
    (Py_TYPE(ob) == dpy_Importer_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_Importer_Type))

}


namespace dballe {
namespace python {

/**
 * Create a new dpy_Importer
 */
dpy_Importer* importer_create(Encoding encoding, const ImporterOptions& opts=ImporterOptions());

void register_importer(PyObject* m);

}
}

#endif


