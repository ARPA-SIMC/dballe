#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "importer.h"
#include "binarymessage.h"
#include "message.h"
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

struct from_binary : MethKwargs<dpy_Importer>
{
    constexpr static const char* name = "from_binary";
    constexpr static const char* doc = "Decode a BinaryMessage to a tuple of dballe.Message objects";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "binmsg", nullptr };
        dpy_BinaryMessage* binmsg = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O!", const_cast<char**>(kwlist), dpy_BinaryMessage_Type, &binmsg))
            return nullptr;
        try {
            auto messages = self->importer->from_binary(binmsg->message);
            pyo_unique_ptr res(throw_ifnull(PyTuple_New(messages.size())));
            for (size_t i = 0; i < messages.size(); ++i)
                PyTuple_SET_ITEM((PyTupleObject*)res.get(), i, (PyObject*)message_create(messages[i]));
            return res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Definition : public Binding<Definition, dpy_Importer>
{
    constexpr static const char* name = "Importer";
    constexpr static const char* qual_name = "dballe.Importer";
    constexpr static const char* doc = "Message importer";

    GetSetters<> getsetters;
    Methods<from_binary> methods;

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

