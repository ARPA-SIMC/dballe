#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "binarymessage.h"
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

namespace {

struct GetEncoding : Getter<dpy_BinaryMessage>
{
    constexpr static const char* name = "encoding";
    constexpr static const char* doc = "message encoding";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            Encoding encoding = self->message.encoding;
            return string_to_python(File::encoding_name(encoding));
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetPathname : Getter<dpy_BinaryMessage>
{
    constexpr static const char* name = "pathname";
    constexpr static const char* doc = "pathname of the file the message came from, or None if unknown";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            if (self->message.pathname.empty())
                Py_RETURN_NONE;
            else
                return string_to_python(self->message.pathname);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetOffset : Getter<dpy_BinaryMessage>
{
    constexpr static const char* name = "offset";
    constexpr static const char* doc = "offset of the message in the input file, or None if unknown";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            if (self->message.offset == (off_t)-1)
                Py_RETURN_NONE;
            else
                return PyLong_FromSize_t((size_t)self->message.offset);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetIndex : Getter<dpy_BinaryMessage>
{
    constexpr static const char* name = "index";
    constexpr static const char* doc = "index of the message in the input file, or None if unknown";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            if (self->message.index == MISSING_INT)
                Py_RETURN_NONE;
            else
                return PyLong_FromLong((long)self->message.index);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Bytes : MethNoargs<dpy_BinaryMessage>
{
    constexpr static const char* name = "__bytes__";
    constexpr static const char* doc = "Returns the contents of this message as a bytes object";
    static PyObject* run(Impl* self)
    {
        return PyBytes_FromStringAndSize(self->message.data.data(), self->message.data.size());
    }
};


struct BinaryMessageDefinition : public Binding<BinaryMessageDefinition, dpy_BinaryMessage>
{
    constexpr static const char* name = "BinaryMessage";
    constexpr static const char* qual_name = "dballe.BinaryMessage";
    constexpr static const char* doc = "Binary message";

    GetSetters<GetEncoding, GetPathname, GetOffset, GetIndex> getsetters;
    Methods<Bytes> methods;

    static void _dealloc(Impl* self)
    {
        self->message.~BinaryMessage();
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
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
};

BinaryMessageDefinition* definition = nullptr;

}

extern "C" {
PyTypeObject dpy_BinaryMessage_Type;
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

    definition = new BinaryMessageDefinition;
    if (definition->activate(dpy_BinaryMessage_Type, m) != 0)
        return -1;

    return 0;
}

}
}
