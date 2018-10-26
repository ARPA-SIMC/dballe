#ifndef DBALLE_PYTHON_FILE_H
#define DBALLE_PYTHON_FILE_H

#include <Python.h>
#include <dballe/file.h>
#include <memory>

namespace dballe {
namespace python {

struct FileWrapper
{
protected:
    FileWrapper() = default;

public:
    FileWrapper(const FileWrapper&) = delete;
    FileWrapper(FileWrapper&&) = delete;
    virtual ~FileWrapper();
    FileWrapper& operator=(const FileWrapper&) = delete;
    FileWrapper& operator=(FileWrapper&&) = delete;

    virtual void close() = 0;
    virtual dballe::File& file() = 0;
};

}
}

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::python::FileWrapper* file;
} dpy_File;

PyAPI_DATA(PyTypeObject) dpy_File_Type;

#define dpy_File_Check(ob) \
    (Py_TYPE(ob) == &dpy_File_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_File_Type))


}


namespace dballe {
namespace python {

/**
 * Create a FileWrapper from a python object, open for reading, autodetecting the
 * encoding.
 *
 * The python object can be a string with the file name, or a file-like object.
 */
std::unique_ptr<FileWrapper> wrapper_r_from_object(PyObject* o);

/**
 * Create a FileWrapper from a python object, open for reading.
 *
 * The python object can be a string with the file name, or a file-like object.
 */
std::unique_ptr<FileWrapper> wrapper_r_from_object(PyObject* o, Encoding encoding);

/**
 * Create a dpy_File from a python object, open for reading, autodetecting the
 * encoding.
 *
 * The python object can be a string with the file name, or a file-like object.
 */
dpy_File* file_create_r_from_object(PyObject* o);

/**
 * Create a dpy_File from a python object, open for reading.
 *
 * The python object can be a string with the file name, or a file-like object.
 */
dpy_File* file_create_r_from_object(PyObject* o, Encoding encoding);

void register_file(PyObject* m);

}
}

#endif
