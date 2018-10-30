#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "importer.h"
#include "dballe/file.h"
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_Importer_Type = nullptr;
}

namespace {

#if 0
struct encoding : Getter<dpy_Importer>
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

struct pathname : Getter<dpy_Importer>
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

struct offset : Getter<dpy_Importer>
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

struct index : Getter<dpy_Importer>
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

struct __bytes__ : MethNoargs<dpy_Importer>
{
    constexpr static const char* name = "__bytes__";
    constexpr static const char* doc = "Returns the contents of this message as a bytes object";
    static PyObject* run(Impl* self)
    {
        return PyBytes_FromStringAndSize(self->message.data.data(), self->message.data.size());
    }
};
#endif

struct Definition : public Binding<Definition, dpy_Importer>
{
    constexpr static const char* name = "Importer";
    constexpr static const char* qual_name = "dballe.Importer";
    constexpr static const char* doc = "Message importer";

    GetSetters<> getsetters;
    Methods<> methods;

    static void _dealloc(Impl* self)
    {
        delete self->importer;
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "encoding", "simplified", nullptr };
        const char* encoding = nullptr;
        int simplified = -1;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s|p", const_cast<char**>(kwlist), &encoding, &simplified))
            return -1;

        try {
            ImporterOptions opts;
            if (simplified != -1)
                opts.simplified = simplified;
            self->importer = Importer::create(File::parse_encoding(encoding), opts).release();
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }
};

Definition* definition = nullptr;

}

namespace dballe {
namespace python {

dpy_Importer* importer_create(Encoding encoding, const ImporterOptions& opts)
{
    dpy_Importer* res = PyObject_New(dpy_Importer, dpy_Importer_Type);
    if (!res) return nullptr;
    res->importer = Importer::create(encoding, opts).release();
    return res;
}

void register_importer(PyObject* m)
{
    common_init();

    definition = new Definition;
    dpy_Importer_Type = definition->activate(m);
}

}
}

