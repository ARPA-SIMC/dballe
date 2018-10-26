#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include <dballe/message.h>
#include "common.h"
#include "file.h"
#include "types.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;


namespace dballe {
namespace python {

FileWrapper::~FileWrapper() {}

struct NamedFileWrapper : public FileWrapper
{
    std::unique_ptr<dballe::File> m_file;
    dballe::File& file() override { return *m_file; }

    int init(const std::string& filename, const char* mode)
    {
        try {
            m_file = File::create(filename, mode);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    int init(const std::string& filename, const char* mode, Encoding encoding)
    {
        try {
            m_file = File::create(encoding, filename, mode);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }
};

struct BaseFileObjFileWrapper : public FileWrapper
{
    std::unique_ptr<dballe::File> m_file;
    std::string filename;

    dballe::File& file() override { return *m_file; }

    /**
     * Try to access the filename from the file object.
     *
     * Defaults to repr() if the object has no name attribute.
     */
    int read_filename(PyObject* o)
    {
        pyo_unique_ptr attr_name(PyObject_GetAttrString(o, "name"));
        if (attr_name)
        {
            if (PyUnicode_Check(attr_name))
            {
                const char* v = PyUnicode_AsUTF8(attr_name);
                if (v == nullptr)
                    return -1;
                filename = v;
                return 0;
            }
        }
        else
            PyErr_Clear();

        pyo_unique_ptr repr(PyObject_Repr(o));
        if (!repr) return -1;

        if (string_from_python(repr, filename))
            return -1;

        return 0;
    }
};

/**
 * File wrapper that takes a file-like object, read its contents into memory,
 * and creates a dballe::File to read from that
 */
struct MemoryInFileWrapper : public BaseFileObjFileWrapper
{
    pyo_unique_ptr data = nullptr;

    FILE* _fmemopen(PyObject* o)
    {
        if (read_filename(o) != 0)
            return nullptr;

        // Read the contents of the file objects into memory
        pyo_unique_ptr read_meth(PyObject_GetAttrString(o, "read"));
        pyo_unique_ptr read_args(PyTuple_New(0));
        data = PyObject_Call(read_meth, read_args, NULL);
        if (!data) return nullptr;
        if (!PyObject_TypeCheck(data, &PyBytes_Type))
        {
            PyErr_SetString(PyExc_ValueError, "read() function must return a bytes object");
            return nullptr;
        }

        // Access the buffer
        char* buf = nullptr;
        Py_ssize_t len = 0;
        if (PyBytes_AsStringAndSize(data, &buf, &len))
            return nullptr;

        // fmemopen the buffer
        FILE* in = fmemopen(buf, len, "r");
        if (!in)
        {
            PyErr_SetFromErrno(PyExc_OSError);
            return nullptr;
        }

        return in;
    }

    int init(PyObject* o)
    {
        FILE* in = _fmemopen(o);
        if (!in)
            return -1;
        try {
            m_file = File::create(in, true, filename);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    int init(PyObject* o, Encoding encoding)
    {
        FILE* in = _fmemopen(o);
        if (!in)
            return -1;
        try {
            m_file = File::create(encoding, in, true, filename);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }
};

struct DupInFileWrapper : public BaseFileObjFileWrapper
{
    FILE* _fdopen(PyObject* o, int fileno)
    {
        if (read_filename(o) != 0)
            return nullptr;

        // Duplicate the file descriptor because both python and libc will want to
        // close it
        int newfd = dup(fileno);
        if (newfd == -1)
        {
            if (filename.empty())
                PyErr_SetFromErrno(PyExc_OSError);
            else
                PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename.c_str());
            return nullptr;
        }

        FILE* in = fdopen(newfd, "rb");
        if (in == nullptr)
        {
            if (filename.empty())
                PyErr_SetFromErrno(PyExc_OSError);
            else
                PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename.c_str());
            close(newfd);
            return nullptr;
        }

        return in;
    }


    int init(PyObject* o, int fileno)
    {
        FILE* in = _fdopen(o, fileno);
        if (!in)
            return -1;
        try {
            m_file = File::create(in, true, filename);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    int init(PyObject* o, int fileno, Encoding encoding)
    {
        FILE* in = _fdopen(o, fileno);
        if (!in)
            return -1;
        try {
            m_file = File::create(encoding, in, true, filename);
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }
};

}
}

namespace {

struct FileDefinition
{
    static const char* name;
    static const char* qual_name;
    static PyGetSetDef getsetters[];
    static PyMethodDef methods[];
    static PyTypeObject type;

    static void _dealloc(dpy_File* self)
    {
        delete self->file;
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _str(dpy_File* self)
    {
        return PyUnicode_FromString(name);
    }

    static PyObject* _repr(dpy_File* self)
    {
        string res = qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(dpy_File* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "file", "encoding", nullptr };
        PyObject* py_file = nullptr;
        const char* encoding = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|s", const_cast<char**>(kwlist), &py_file, &encoding))
            return -1;

        try {
            if (encoding)
            {
                auto wrapper = wrapper_r_from_object(py_file, File::parse_encoding(encoding));
                if (!wrapper) return -1;
                self->file = wrapper.release();
            } else {
                auto wrapper = wrapper_r_from_object(py_file);
                if (!wrapper) return -1;
                self->file = wrapper.release();
            }
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    static PyObject* _enter(dpy_File* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
    }

    static PyObject* _exit(dpy_File* self, PyObject* args)
    {
        PyObject* exc_type;
        PyObject* exc_val;
        PyObject* exc_tb;
        if (!PyArg_ParseTuple(args, "OOO", &exc_type, &exc_val, &exc_tb))
            return nullptr;

        try {
            delete self->file;
            self->file = nullptr;
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }

    static PyObject* _name(dpy_File* self, void* closure)
    {
        try {
            return string_to_python(self->file->file().pathname());
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _encoding(dpy_File* self, void* closure)
    {
        try {
            Encoding encoding = self->file->file().encoding();
            return string_to_python(File::encoding_name(encoding));
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _iter(dpy_File* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
    }

    static PyObject* _iternext(dpy_File* self)
    {
        try {
            if (BinaryMessage msg = self->file->file().read())
            {
                return (PyObject*)binarymessage_create(std::move(msg));
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        } DBALLE_CATCH_RETURN_PYO
    }
};

const char* FileDefinition::name = "File";
const char* FileDefinition::qual_name = "dballe.File";

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
PyGetSetDef FileDefinition::getsetters[] = {
    {"name", (getter)_name, nullptr, "get the file name", nullptr },
    {"encoding", (getter)_encoding, nullptr, "get the file encoding", nullptr },
    {nullptr}
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

PyMethodDef FileDefinition::methods[] = {
    {"__enter__",         (PyCFunction)_enter, METH_NOARGS, "Context manager __enter__" },
    {"__exit__",          (PyCFunction)_exit, METH_VARARGS, "Context manager __exit__" },
    {nullptr}
};

PyTypeObject FileDefinition::type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    qual_name,                 // tp_name
    sizeof(dpy_File),          // tp_basicsize
    0,                         // tp_itemsize
    (destructor)_dealloc,      // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)_repr,           // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)_str,            // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e File",           // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)_iter,        // tp_iter
    (iternextfunc)_iternext,   // tp_iternext
    methods,                   // tp_methods
    0,                         // tp_members
    getsetters,                // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)_init,           // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};


struct BinaryMessageDefinition
{
    static const char* name;
    static const char* qual_name;
    static PyGetSetDef getsetters[];
    static PyMethodDef methods[];
    static PyTypeObject type;

    static void _dealloc(dpy_BinaryMessage* self)
    {
        self->message.~BinaryMessage();
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _str(dpy_BinaryMessage* self)
    {
        return PyUnicode_FromString(name);
    }

    static PyObject* _repr(dpy_BinaryMessage* self)
    {
        string res = qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(dpy_BinaryMessage* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "data", "encoding", "pathname", "offset", "index", nullptr };
        PyObject* py_data = nullptr;
        const char* encoding = nullptr;
        const char* pathname = nullptr;
        Py_ssize_t offset = -1;
        Py_ssize_t index = -1;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "Os|snn", const_cast<char**>(kwlist), &py_data, &encoding, &pathname, &offset, &index))
            return -1;

        try {
            pyo_unique_ptr py_bytes(PyBytes_FromObject(py_data));
            if (!py_bytes)
                return -1;

            char* buf;
            Py_ssize_t len;
            if (PyBytes_AsStringAndSize(py_bytes, &buf, &len) != 0)
                return -1;

            Encoding enc = File::parse_encoding(encoding);
            new (&(self->message)) BinaryMessage(enc);

            self->message.data.assign(buf, len);

            if (pathname) self->message.pathname = pathname;
            if (offset != -1) self->message.offset = offset;
            if (index != -1) self->message.index = index;
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }

    static PyObject* _encoding(dpy_BinaryMessage* self, void* closure)
    {
        try {
            Encoding encoding = self->message.encoding;
            return string_to_python(File::encoding_name(encoding));
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _pathname(dpy_BinaryMessage* self, void* closure)
    {
        try {
            return string_to_python(self->message.pathname);
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _offset(dpy_BinaryMessage* self, void* closure)
    {
        try {
            if (self->message.offset == (off_t)-1)
                Py_RETURN_NONE;
            else
                return PyLong_FromSize_t((size_t)self->message.offset);
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _index(dpy_BinaryMessage* self, void* closure)
    {
        try {
            if (self->message.index == MISSING_INT)
                Py_RETURN_NONE;
            else
                return PyLong_FromLong((long)self->message.index);
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _bytes(dpy_BinaryMessage* self)
    {
        Py_INCREF(self);
        return PyBytes_FromStringAndSize(self->message.data.data(), self->message.data.size());
    }
};

const char* BinaryMessageDefinition::name = "BinaryMessage";
const char* BinaryMessageDefinition::qual_name = "dballe.BinaryMessage";

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
PyGetSetDef BinaryMessageDefinition::getsetters[] = {
    {"encoding", (getter)_encoding, nullptr, "get the file encoding", nullptr },
    {"pathname", (getter)_pathname, nullptr, "get the input file pathname", nullptr },
    {"offset", (getter)_offset, nullptr, "get the offset of the message in the input file", nullptr },
    {"index", (getter)_index, nullptr, "get the index of the message in the input file", nullptr },
    {nullptr}
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

PyMethodDef BinaryMessageDefinition::methods[] = {
    {"__bytes__",    (PyCFunction)_bytes, METH_NOARGS, "Returns the bytes contents of this message" },
    {nullptr}
};

PyTypeObject BinaryMessageDefinition::type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    qual_name,                 // tp_name
    sizeof(dpy_BinaryMessage), // tp_basicsize
    0,                         // tp_itemsize
    (destructor)_dealloc,      // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)_repr,           // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)_str,            // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e BinaryMessage",  // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    methods,                   // tp_methods
    0,                         // tp_members
    getsetters,                // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)_init,           // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

extern "C" {

PyTypeObject dpy_File_Type = FileDefinition::type;
PyTypeObject dpy_BinaryMessage_Type = BinaryMessageDefinition::type;

}

namespace dballe {
namespace python {

dpy_File* file_create(std::unique_ptr<FileWrapper> wrapper)
{
    dpy_File* res = PyObject_New(dpy_File, &dpy_File_Type);
    if (!res) return nullptr;
    res->file = wrapper.release();
    return res;
}

/**
 * Try to call obj.fileno().
 *
 * Set fileno to -1 and returns 0 if the object does not support fileno().
 * Set fileno to the file descriptor and returns 0 if the object supports.
 * Returns -1 and leaves fileno unchanged if there has been an error.
 */
static int file_get_fileno(PyObject* o, int& fileno)
{
    // fileno_value = obj.fileno()
    pyo_unique_ptr fileno_meth(PyObject_GetAttrString(o, "fileno"));
    if (!fileno_meth)
    {
        PyErr_Clear();
        fileno = -1;
        return 0;
    }

    pyo_unique_ptr fileno_args(PyTuple_New(0));
    if (!fileno_args) return -1;

    PyObject* fileno_value = PyObject_Call(fileno_meth, fileno_args, nullptr);
    if (!fileno_value)
    {
        if (PyErr_ExceptionMatches(PyExc_AttributeError) || PyErr_ExceptionMatches(PyExc_IOError))
        {
            PyErr_Clear();
            fileno = -1;
            return 0;
        } else
            return -1;
    }

    // fileno = int(fileno_value)
    long res = PyLong_AsLong(fileno_value);
    if (PyErr_Occurred())
        return -1;

    fileno = res;
    return 0;
}

std::unique_ptr<FileWrapper> wrapper_r_from_object(PyObject* o)
{
    if (PyBytes_Check(o))
    {
        const char* v = PyBytes_AsString(o);
        if (!v) return nullptr;
        std::unique_ptr<NamedFileWrapper> wrapper(new NamedFileWrapper);
        wrapper->init(v, "rb");
        return wrapper;
    }
    if (PyUnicode_Check(o)) {
        const char* v = PyUnicode_AsUTF8(o);
        if (!v) return nullptr;
        std::unique_ptr<NamedFileWrapper> wrapper(new NamedFileWrapper);
        wrapper->init(v, "rb");
        return wrapper;
    }

    int fileno;
    if (file_get_fileno(o, fileno) == -1)
        return nullptr;

    if (fileno == -1)
    {
        std::unique_ptr<MemoryInFileWrapper> wrapper(new MemoryInFileWrapper);
        wrapper->init(o);
        return wrapper;
    } else {
        std::unique_ptr<DupInFileWrapper> wrapper(new DupInFileWrapper);
        wrapper->init(o, fileno);
        return wrapper;
    }
}

std::unique_ptr<FileWrapper> wrapper_r_from_object(PyObject* o, Encoding encoding)
{
    if (PyBytes_Check(o))
    {
        const char* v = PyBytes_AsString(o);
        if (!v) return nullptr;
        std::unique_ptr<NamedFileWrapper> wrapper(new NamedFileWrapper);
        wrapper->init(v, "rb", encoding);
        return wrapper;
    }
    if (PyUnicode_Check(o)) {
        const char* v = PyUnicode_AsUTF8(o);
        if (!v) return nullptr;
        std::unique_ptr<NamedFileWrapper> wrapper(new NamedFileWrapper);
        wrapper->init(v, "rb", encoding);
        return wrapper;
    }

    int fileno;
    if (file_get_fileno(o, fileno) == -1)
        return nullptr;

    if (fileno == -1)
    {
        std::unique_ptr<MemoryInFileWrapper> wrapper(new MemoryInFileWrapper);
        wrapper->init(o, encoding);
        return wrapper;
    } else {
        std::unique_ptr<DupInFileWrapper> wrapper(new DupInFileWrapper);
        wrapper->init(o, fileno, encoding);
        return wrapper;
    }
}

dpy_File* file_create_r_from_object(PyObject* o)
{
    auto wrapper = wrapper_r_from_object(o);
    if (!wrapper)
        return nullptr;
    return file_create(std::move(wrapper));
}

dpy_File* file_create_r_from_object(PyObject* o, Encoding encoding)
{
    auto wrapper = wrapper_r_from_object(o, encoding);
    if (!wrapper)
        return nullptr;
    return file_create(std::move(wrapper));
}

dpy_BinaryMessage* binarymessage_create(const BinaryMessage& message)
{
    dpy_BinaryMessage* res = PyObject_New(dpy_BinaryMessage, &dpy_BinaryMessage_Type);
    if (!res) return nullptr;
    new (&(res->message)) BinaryMessage(message);
    return res;
}

dpy_BinaryMessage* binarymessage_create(BinaryMessage&& message)
{
    dpy_BinaryMessage* res = PyObject_New(dpy_BinaryMessage, &dpy_BinaryMessage_Type);
    if (!res) return nullptr;
    new (&(res->message)) BinaryMessage(std::move(message));
    return res;
}


void register_file(PyObject* m)
{
    common_init();

    dpy_File_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_File_Type) < 0)
        return;
    Py_INCREF(&dpy_File_Type);
    PyModule_AddObject(m, "File", (PyObject*)&dpy_File_Type);

    dpy_BinaryMessage_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_BinaryMessage_Type) < 0)
        return;
    Py_INCREF(&dpy_BinaryMessage_Type);
    PyModule_AddObject(m, "BinaryMessage", (PyObject*)&dpy_BinaryMessage_Type);
}

}
}

