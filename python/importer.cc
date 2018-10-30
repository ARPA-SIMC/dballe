#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include "common.h"
#include "importer.h"
#include "binarymessage.h"
#include "file.h"
#include "message.h"
#include "dballe/file.h"
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_Importer_Type = nullptr;
PyTypeObject* dpy_ImporterFile_Type = nullptr;
}

namespace {

namespace importerfile {
struct Definition : public Binding<Definition, dpy_ImporterFile>
{
    constexpr static const char* name = "ImporterFile";
    constexpr static const char* qual_name = "dballe.ImporterFile";
    constexpr static const char* doc = "Message importer iterating over a dballe.File contents";

    GetSetters<> getsetters;
    Methods<> methods;

    static void _dealloc(Impl* self)
    {
        Py_DECREF(self->importer);
        Py_DECREF(self->file);
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _iter(Impl* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
    }

    static PyObject* _iternext(Impl* self)
    {
        try {
            BinaryMessage binmsg = self->file->file->file().read();
            if (!binmsg)
            {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
            auto messages = self->importer->importer->from_binary(binmsg);
            pyo_unique_ptr res(throw_ifnull(PyTuple_New(messages.size())));
            for (size_t i = 0; i < messages.size(); ++i)
                PyTuple_SET_ITEM((PyTupleObject*)res.get(), i, (PyObject*)message_create(messages[i]));
            return res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

Definition* definition = nullptr;
}

namespace importer {
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

struct from_file : MethKwargs<dpy_Importer>
{
    constexpr static const char* name = "from_file";
    constexpr static const char* doc = "Wrap a dballe.File into a sequence of tuples of dballe.Message objects";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "file", nullptr };
        dpy_File* file = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O!", const_cast<char**>(kwlist), dpy_File_Type, &file))
            return nullptr;

        try {
            py_unique_ptr<dpy_ImporterFile> res = throw_ifnull(PyObject_New(dpy_ImporterFile, dpy_ImporterFile_Type));
            Py_INCREF(file);
            res->file = file;
            Py_INCREF(self);
            res->importer = self;
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Definition : public Binding<Definition, dpy_Importer>
{
    constexpr static const char* name = "Importer";
    constexpr static const char* qual_name = "dballe.Importer";
    constexpr static const char* doc = "Message importer";

    GetSetters<> getsetters;
    Methods<from_binary, from_file> methods;

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

    importerfile::definition = new importerfile::Definition;
    dpy_ImporterFile_Type = importerfile::definition->activate(m);

    importer::definition = new importer::Definition;
    dpy_Importer_Type = importer::definition->activate(m);
}

}
}

