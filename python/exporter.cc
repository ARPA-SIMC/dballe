#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "exporter.h"
#include "dballe/file.h"
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_Exporter_Type = nullptr;
}

namespace {

#if 0
struct encoding : Getter<dpy_Exporter>
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

struct pathname : Getter<dpy_Exporter>
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

struct offset : Getter<dpy_Exporter>
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

struct index : Getter<dpy_Exporter>
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

struct __bytes__ : MethNoargs<dpy_Exporter>
{
    constexpr static const char* name = "__bytes__";
    constexpr static const char* doc = "Returns the contents of this message as a bytes object";
    static PyObject* run(Impl* self)
    {
        return PyBytes_FromStringAndSize(self->message.data.data(), self->message.data.size());
    }
};
#endif

struct Definition : public Binding<Definition, dpy_Exporter>
{
    constexpr static const char* name = "Exporter";
    constexpr static const char* qual_name = "dballe.Exporter";
    constexpr static const char* doc = "Message exporter";

    GetSetters<> getsetters;
    Methods<> methods;

    static void _dealloc(Impl* self)
    {
        delete self->exporter;
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "encoding", "template_name", "centre", "subcentre", "application", nullptr };
        const char* encoding = nullptr;
        const char* template_name = nullptr;
        int centre = -1;
        int subcentre = -1;
        int application = -1;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s|siii", const_cast<char**>(kwlist), &encoding, &template_name, &centre, &subcentre, &application))
            return -1;

        try {
            ExporterOptions opts;
            if (template_name) opts.template_name = template_name;
            if (centre != -1) opts.centre = centre;
            if (subcentre != -1) opts.subcentre = subcentre;
            if (application != -1) opts.application = application;
            self->exporter = Exporter::create(File::parse_encoding(encoding), opts).release();
        } DBALLE_CATCH_RETURN_INT
        return 0;
    }
};

Definition* definition = nullptr;

}

namespace dballe {
namespace python {

dpy_Exporter* exporter_create(Encoding encoding, const ExporterOptions& opts)
{
    dpy_Exporter* res = PyObject_New(dpy_Exporter, dpy_Exporter_Type);
    if (!res) return nullptr;
    res->exporter = Exporter::create(encoding, opts).release();
    return res;
}

void register_exporter(PyObject* m)
{
    common_init();

    definition = new Definition;
    dpy_Exporter_Type = definition->activate(m);
}

}
}

