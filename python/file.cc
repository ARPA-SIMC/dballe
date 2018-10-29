#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "file.h"
#include "binarymessage.h"
#include "impl-utils.h"

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

    void close() override
    {
        m_file->close();
    }

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
        try {
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

            filename = string_from_python(repr);
            return 0;
        } DBALLE_CATCH_RETURN_INT
    }
};

/**
 * File wrapper that takes a file-like object, read its contents into memory,
 * and creates a dballe::File to read from that
 */
struct MemoryInFileWrapper : public BaseFileObjFileWrapper
{
    pyo_unique_ptr data = nullptr;

    void close() override
    {
        m_file->close();
        data.clear();
    }

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
    void close() override
    {
        m_file->close();
    }

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
            ::close(newfd);
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

struct GetName : Getter<dpy_File>
{
    constexpr static const char* name = "name";
    constexpr static const char* doc = "get the file name";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return string_to_python(self->file->file().pathname());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetEncoding : Getter<dpy_File>
{
    constexpr static const char* name = "encoding";
    constexpr static const char* doc = "get the file encoding";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            Encoding encoding = self->file->file().encoding();
            return string_to_python(File::encoding_name(encoding));
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Enter : MethNoargs<dpy_File>
{
    constexpr static const char* name = "__enter__";
    constexpr static const char* doc = "Context manager __enter__";
    static PyObject* run(Impl* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
    }
};

struct Exit : MethVarargs<dpy_File>
{
    constexpr static const char* name = "__exit__";
    constexpr static const char* doc = "Context manager __exit__";
    static PyObject* run(Impl* self, PyObject* args)
    {
        PyObject* exc_type;
        PyObject* exc_val;
        PyObject* exc_tb;
        if (!PyArg_ParseTuple(args, "OOO", &exc_type, &exc_val, &exc_tb))
            return nullptr;

        try {
            self->file->close();
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};


struct FileDefinition : public Binding<FileDefinition, dpy_File>
{
    constexpr static const char* name = "File";
    constexpr static const char* qual_name = "dballe.File";
    constexpr static const char* doc = "Message file read access. To write files, you can write BinaryMessage objects to normal Python files.";

    GetSetters<GetName, GetEncoding> getsetters;
    Methods<Enter, Exit> methods;

    static void _dealloc(Impl* self)
    {
        delete self->file;
        Py_TYPE(self)->tp_free(self);
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

FileDefinition* file_definition = nullptr;

}


extern "C" {
PyTypeObject* dpy_File_Type = nullptr;
}

namespace dballe {
namespace python {

dpy_File* file_create(std::unique_ptr<FileWrapper> wrapper)
{
    dpy_File* res = PyObject_New(dpy_File, dpy_File_Type);
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
        return std::unique_ptr<FileWrapper>(wrapper.release());
    }
    if (PyUnicode_Check(o)) {
        const char* v = PyUnicode_AsUTF8(o);
        if (!v) return nullptr;
        std::unique_ptr<NamedFileWrapper> wrapper(new NamedFileWrapper);
        wrapper->init(v, "rb");
        return std::unique_ptr<FileWrapper>(wrapper.release());
    }

    int fileno;
    if (file_get_fileno(o, fileno) == -1)
        return nullptr;

    if (fileno == -1)
    {
        std::unique_ptr<MemoryInFileWrapper> wrapper(new MemoryInFileWrapper);
        wrapper->init(o);
        return std::unique_ptr<FileWrapper>(wrapper.release());
    } else {
        std::unique_ptr<DupInFileWrapper> wrapper(new DupInFileWrapper);
        wrapper->init(o, fileno);
        return std::unique_ptr<FileWrapper>(wrapper.release());
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
        return std::unique_ptr<FileWrapper>(wrapper.release());
    }
    if (PyUnicode_Check(o)) {
        const char* v = PyUnicode_AsUTF8(o);
        if (!v) return nullptr;
        std::unique_ptr<NamedFileWrapper> wrapper(new NamedFileWrapper);
        wrapper->init(v, "rb", encoding);
        return std::unique_ptr<FileWrapper>(wrapper.release());
    }

    int fileno;
    if (file_get_fileno(o, fileno) == -1)
        return nullptr;

    if (fileno == -1)
    {
        std::unique_ptr<MemoryInFileWrapper> wrapper(new MemoryInFileWrapper);
        wrapper->init(o, encoding);
        return std::unique_ptr<FileWrapper>(wrapper.release());
    } else {
        std::unique_ptr<DupInFileWrapper> wrapper(new DupInFileWrapper);
        wrapper->init(o, fileno, encoding);
        return std::unique_ptr<FileWrapper>(wrapper.release());
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

int register_file(PyObject* m)
{
    if (common_init() != 0)
        return -1;

    file_definition = new FileDefinition;
    if (!(dpy_File_Type = file_definition->activate(m)))
        return -1;

    return 0;
}

}
}

