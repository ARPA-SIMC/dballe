#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "exporter.h"
#include "message.h"
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
struct to_binary : MethKwargs<dpy_Exporter>
{
    constexpr static const char* name = "to_binary";
    constexpr static const char* doc = "Encode a dballe.Message or a sequence of dballe.Message into a bytes object";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "contents", nullptr };
        PyObject* contents = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &contents))
            return nullptr;
        try {
            std::vector<std::shared_ptr<Message>> messages;
            if (dpy_Message_Check(contents))
            {
                messages.push_back(((dpy_Message*)contents)->message);
            } else if (PySequence_Check(contents)) {
                // Iterate sequence
                Py_ssize_t len = PySequence_Length(contents);
                if (len == -1) return nullptr;
                if (len == 0)
                {
                    PyErr_SetString(PyExc_ValueError, "to_binary requires a dballe.Message or a non-empty sequence of dballe.Message objects");
                    return nullptr;
                }
                for (Py_ssize_t i = 0; i < len; ++i)
                {
                    pyo_unique_ptr o(throw_ifnull(PySequence_ITEM(contents, i)));
                    if (!dpy_Message_Check(o))
                    {
                        PyErr_SetString(PyExc_TypeError, "to_binary requires a dballe.Message or a sequence of dballe.Message objects");
                        return nullptr;
                    }
                    messages.push_back(((dpy_Message*)o.get())->message);
                }
            } else {
                PyErr_SetString(PyExc_TypeError, "to_binary requires a dballe.Message or a sequence of dballe.Message objects");
                return nullptr;
            }

            std::string encoded = self->exporter->to_binary(messages);
            return PyBytes_FromStringAndSize(encoded.data(), encoded.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};


struct Definition : public Binding<Definition, dpy_Exporter>
{
    constexpr static const char* name = "Exporter";
    constexpr static const char* qual_name = "dballe.Exporter";
    constexpr static const char* doc = "Message exporter";

    GetSetters<> getsetters;
    Methods<to_binary> methods;

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

