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

#if 0

template<typename Station>
static PyObject* _export_stations(const db::summary::StationEntries<Station>& stations)
{
    try {
        pyo_unique_ptr result(PyList_New(stations.size()));

        unsigned idx = 0;
        for (const auto& entry: stations)
        {
            pyo_unique_ptr station(to_python(entry.station));
            if (PyList_SetItem(result, idx, station.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_stations(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_stations(summary.stations());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _stations(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_stations(summary.stations());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_reports(const core::SortedSmallUniqueValueSet<std::string>& reports)
{
    try {
        pyo_unique_ptr result(PyList_New(reports.size()));

        unsigned idx = 0;
        for (const auto& v: reports)
        {
            pyo_unique_ptr value(string_to_python(v));
            if (PyList_SetItem(result, idx, value.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_reports(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_reports(summary.reports());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _reports(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_reports(summary.reports());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_levels(const core::SortedSmallUniqueValueSet<dballe::Level>& levels)
{
    try {
        pyo_unique_ptr result(PyList_New(levels.size()));

        unsigned idx = 0;
        for (const auto& v: levels)
        {
            pyo_unique_ptr level(level_to_python(v));
            if (PyList_SetItem(result, idx, level.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_levels(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_levels(summary.levels());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _levels(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_levels(summary.levels());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_tranges(const core::SortedSmallUniqueValueSet<dballe::Trange>& tranges)
{
    try {
        pyo_unique_ptr result(PyList_New(tranges.size()));

        unsigned idx = 0;
        for (const auto& v: tranges)
        {
            pyo_unique_ptr trange(trange_to_python(v));
            if (PyList_SetItem(result, idx, trange.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_tranges(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_tranges(summary.tranges());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _tranges(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_tranges(summary.tranges());
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* _export_varcodes(const core::SortedSmallUniqueValueSet<wreport::Varcode>& varcodes)
{
    try {
        pyo_unique_ptr result(PyList_New(varcodes.size()));

        unsigned idx = 0;
        for (const auto& v: varcodes)
        {
            pyo_unique_ptr varcode(varcode_to_python(v));
            if (PyList_SetItem(result, idx, varcode.release()))
                return nullptr;
            ++idx;
        }

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_varcodes(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_varcodes(summary.varcodes());
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _varcodes(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_varcodes(summary.varcodes());
    } DBALLE_CATCH_RETURN_PYO
}


template<typename Station>
static PyObject* _export_stats(const dballe::db::BaseSummary<Station>& summary)
{
    try {
        pyo_unique_ptr res(PyStructSequence_New(&dpy_stats_Type));
        if (!res) return nullptr;

        if (PyObject* v = datetime_to_python(summary.datetime_min()))
            PyStructSequence_SET_ITEM((PyObject*)res, 0, v);
        else
            return nullptr;

        if (PyObject* v = datetime_to_python(summary.datetime_max()))
            PyStructSequence_SET_ITEM((PyObject*)res, 1, v);
        else
            return nullptr;

        if (PyObject* v = PyLong_FromLong(summary.data_count()))
            PyStructSequence_SET_ITEM((PyObject*)res, 2, v);
        else
            return nullptr;

        return res.release();
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _all_stats(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->global_summary();
        return _export_stats(summary);
    } DBALLE_CATCH_RETURN_PYO
}

template<typename dpy_Explorer>
static PyObject* _stats(dpy_Explorer* self, void* closure)
{
    try {
        const auto& summary = self->explorer->active_summary();
        return _export_stats(summary);
    } DBALLE_CATCH_RETURN_PYO
}
#endif

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
                // TODO:
                // self->cur->to_record(*self->rec->rec);
                // Py_INCREF(self->rec);
                // return (PyObject*)self->rec;
                Py_RETURN_NONE;
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        } DBALLE_CATCH_RETURN_PYO
    }

#if 0
    static PyObject* _datetime(dpy_Message* self, void* closure)
    {
        try {
            return datetime_to_python(self->message->get_datetime());
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _coords(dpy_Message* self, void* closure)
    {
        try {
            return coords_to_python(self->message->get_coords());
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _ident(dpy_Message* self, void* closure)
    {
        try {
            return ident_to_python(self->message->get_ident());
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _network(dpy_Message* self, void* closure)
    {
        try {
            auto network = self->message->get_network();
            return PyUnicode_FromStringAndSize(network.data(), network.size());
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _get(dpy_Message* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "level", "trange", "code", nullptr };
        PyObject* pylevel = nullptr;
        PyObject* pytrange = nullptr;
        PyObject* pycode = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &pylevel, &pytrange, &pycode))
            return nullptr;

        try {
            Level level;
            if (level_from_python(pylevel, level) != 0)
                return nullptr;

            Trange trange;
            if (trange_from_python(pytrange, trange) != 0)
                return nullptr;

            Varcode code;
            if (varcode_from_python(pycode, code) != 0)
                return nullptr;

            const Var* res = self->message->get(level, trange, code);
            if (!res)
                Py_RETURN_NONE;
            else
                return (PyObject*)wrpy->var_create_copy(*res);
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _get_named(dpy_Message* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", nullptr };
        const char* name = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &name))
            return nullptr;

        try {
            const Var* res = self->message->get(name);
            if (!res)
                Py_RETURN_NONE;
            else
                return (PyObject*)wrpy->var_create_copy(*res);
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _set(dpy_Message* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "level", "trange", "var", nullptr };
        PyObject* pylevel = nullptr;
        PyObject* pytrange = nullptr;
        wrpy_Var* var = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOO!", const_cast<char**>(kwlist), &pylevel, &pytrange, wrpy->var_type, &var))
            return nullptr;

        try {
            Level level;
            if (level_from_python(pylevel, level) != 0)
                return nullptr;

            Trange trange;
            if (trange_from_python(pytrange, trange) != 0)
                return nullptr;

            self->message->set(level, trange, var->var);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }

    static PyObject* _set_named(dpy_Message* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", "var", nullptr };
        const char* name = nullptr;
        wrpy_Var* var = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "sO!", const_cast<char**>(kwlist), &name, wrpy->var_type, &var))
            return nullptr;

        try {
            self->message->set(name, var->var);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
#endif
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
#if 0
    {"type", (getter)_type, nullptr, "get message type", nullptr },
    {"datetime", (getter)_datetime, nullptr, "get message datetime", nullptr },
    {"coords", (getter)_coords, nullptr, "get message coordinates", nullptr },
    {"ident", (getter)_ident, nullptr, "get message station identifier", nullptr },
    {"network", (getter)_network, nullptr, "get message network", nullptr },
#endif
    {nullptr}
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

PyMethodDef FileDefinition::methods[] = {
#if 0
    {"get",          (PyCFunction)_get, METH_VARARGS | METH_KEYWORDS,
        "Get a Var given level, timerange, and varcode; returns None if not found" },
    {"get_named",    (PyCFunction)_get_named, METH_VARARGS | METH_KEYWORDS,
        "Get a Var given its shortcut name; returns None if not found" },
    {"set",          (PyCFunction)_set, METH_VARARGS | METH_KEYWORDS,
        "Set a Var given level and timerange" },
    {"set_named",    (PyCFunction)_set_named, METH_VARARGS | METH_KEYWORDS,
        "Set a Var given its shortcut name" },
#endif
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
}

extern "C" {

PyTypeObject dpy_File_Type = FileDefinition::type;

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

void register_file(PyObject* m)
{
    common_init();

    dpy_File_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_File_Type) < 0)
        return;
    Py_INCREF(&dpy_File_Type);
    PyModule_AddObject(m, "File", (PyObject*)&dpy_File_Type);
}

}
}

