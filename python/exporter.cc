#define _DBALLE_LIBRARY_CODE
#include "common.h"
#include "exporter.h"
#include "message.h"
#include "dballe/file.h"
#include "dballe/msg/msg.h"
#include "utils/type.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_Exporter_Type = nullptr;
}

namespace {

struct to_binary : MethKwargs<to_binary, dpy_Exporter>
{
    constexpr static const char* name = "to_binary";
    constexpr static const char* signature = "contents: Union[dballe.Message, Sequence[dballe.Message], Iterable[dballe.Message]]";
    constexpr static const char* returns = "bytes";
    constexpr static const char* doc = R"(
Encode a dballe.Message or a sequence of dballe.Message into a bytes object.
)";

    [[noreturn]] static void throw_typeerror()
    {
        PyErr_SetString(PyExc_ValueError, "to_binary requires a dballe.Message or a non-empty sequence of dballe.Message objects, or an iterable of dballe.Message objects");
        throw PythonException();
    }

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
                if (len == 0) throw_typeerror();
                messages.reserve(len);
                for (Py_ssize_t i = 0; i < len; ++i)
                {
                    pyo_unique_ptr o(throw_ifnull(PySequence_ITEM(contents, i)));
                    if (!dpy_Message_Check(o)) throw_typeerror();
                    messages.push_back(((dpy_Message*)o.get())->message);
                }
            } else if (PyIter_Check(contents)) {
                // Iterate iterator
                pyo_unique_ptr iterator = throw_ifnull(PyObject_GetIter(contents));
                while (pyo_unique_ptr item = PyIter_Next(iterator))
                {
                    if (!dpy_Message_Check(item)) throw_typeerror();
                    messages.push_back(((dpy_Message*)item.get())->message);
                }
                if (PyErr_Occurred()) return nullptr;
            } else
                throw_typeerror();

            std::string encoded = self->exporter->to_binary(messages);
            return PyBytes_FromStringAndSize(encoded.data(), encoded.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};


struct Definition : public Type<Definition, dpy_Exporter>
{
    constexpr static const char* name = "Exporter";
    constexpr static const char* qual_name = "dballe.Exporter";
    constexpr static const char* doc = R"(
Message exporter.

This is the engine that can reconstruct a standard BUFR or CREX message from
the contents of a `dballe.Message`_.
)";

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
            impl::ExporterOptions opts;
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
    definition->define(dpy_Exporter_Type, m);
}

}
}

