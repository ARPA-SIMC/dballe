#ifndef DBALLE_PYTHON_IMPORTER_H
#define DBALLE_PYTHON_IMPORTER_H

#include <Python.h>
#include "dballe/importer.h"
#include "file.h"
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


typedef struct {
    PyObject_HEAD
    dpy_File* file;
    dpy_Importer* importer;
} dpy_ImporterFile;

extern PyTypeObject* dpy_ImporterFile_Type;

#define dpy_ImporterFile_Check(ob) \
    (Py_TYPE(ob) == dpy_ImporterFile_Type || \
     PyType_IsSubtype(Py_TYPE(ob), dpy_ImporterFile_Type))


}


namespace dballe {
namespace python {

/**
 * Create a new dpy_Importer
 */
dpy_Importer* importer_create(Encoding encoding, const dballe::ImporterOptions& opts=dballe::ImporterOptions::defaults);

void register_importer(PyObject* m);

}
}

#endif


