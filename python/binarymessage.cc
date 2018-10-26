#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "binarymessage.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

namespace {

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
            if (self->message.pathname.empty())
                Py_RETURN_NONE;
            else
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

PyTypeObject dpy_BinaryMessage_Type = BinaryMessageDefinition::type;

}

namespace dballe {
namespace python {

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


int register_binarymessage(PyObject* m)
{
    if (common_init() != 0)
        return -1;

    dpy_BinaryMessage_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_BinaryMessage_Type) < 0)
        return -1;
    Py_INCREF(&dpy_BinaryMessage_Type);
    PyModule_AddObject(m, "BinaryMessage", (PyObject*)&dpy_BinaryMessage_Type);

    return 0;
}

}
}


