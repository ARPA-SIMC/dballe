#define _DBALLE_LIBRARY_CODE
#include "common.h"
#include "importer.h"
#include "binarymessage.h"
#include "file.h"
#include "message.h"
#include "dballe/file.h"
#include "dballe/msg/msg.h"
#include "utils/type.h"

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

typedef MethGenericEnter<dpy_ImporterFile> __enter__;

struct __exit__ : MethVarargs<__exit__, dpy_ImporterFile>
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
            Py_XDECREF(self->importer);
            self->importer = nullptr;
            Py_XDECREF(self->file);
            self->file = nullptr;
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


struct Definition : public Type<Definition, dpy_ImporterFile>
{
    constexpr static const char* name = "ImporterFile";
    constexpr static const char* qual_name = "dballe.ImporterFile";
    constexpr static const char* doc = R"(
Message importer iterating over the contents of a a :class:`dballe.File`.

This is never instantiated explicitly, but is returned by
:func:`Importer.from_file()`.

It can be used in a context manager, and it is an iterable that yields tuples
of :class:`dballe.Message` objects.
)";

    GetSetters<> getsetters;
    Methods<__enter__, __exit__> methods;

    static void check_valid(Impl* self)
    {
        if (!self->file || !self->importer)
        {
            PyErr_SetString(PyExc_RuntimeError, "cannot access a dballe.ImporterFile after the with block where it was used");
            throw PythonException();
        }
    }

    static void _dealloc(Impl* self)
    {
        Py_XDECREF(self->importer);
        Py_XDECREF(self->file);
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _iter(Impl* self)
    {
        try {
            check_valid(self);
            Py_INCREF(self);
            return (PyObject*)self;
        } DBALLE_CATCH_RETURN_PYO
    }

    static PyObject* _iternext(Impl* self)
    {
        try {
            check_valid(self);
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
struct from_binary : MethKwargs<from_binary, dpy_Importer>
{
    constexpr static const char* name = "from_binary";
    constexpr static const char* signature = "binmsg: dballe.BinaryMessage";
    constexpr static const char* returns = "Sequence[dballe.BinaryMessage]";
    constexpr static const char* summary = "Decode a BinaryMessage to a tuple of dballe.Message objects";
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

struct from_file : MethKwargs<from_file, dpy_Importer>
{
    constexpr static const char* name = "from_file";
    constexpr static const char* signature = "file: Union[dballe.File, str, File]";
    constexpr static const char* returns = "dballe.ImporterFile";
    constexpr static const char* doc = R"(
Wrap a :class:`dballe.File` into a sequence of tuples of :class:`dballe.Message` objects.

`file` can be a :class:`dballe.File`, a file name, or a file-like object. A :class:`dballe.File`
is automatically constructed if needed, using the importer encoding.
)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "file", nullptr };
        PyObject* obj = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &obj))
            return nullptr;

        try {
            py_unique_ptr<dpy_File> file;

            if (dpy_File_Check(obj))
            {
                Py_INCREF(obj);
                file = (dpy_File*)obj;
            } else {
                file = file_create_r_from_object(obj, self->importer->encoding());
            }

            py_unique_ptr<dpy_ImporterFile> res = throw_ifnull(PyObject_New(dpy_ImporterFile, dpy_ImporterFile_Type));
            res->file = file.release();
            Py_INCREF(self);
            res->importer = self;
            return (PyObject*)res.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct Definition : public Type<Definition, dpy_Importer>
{
    constexpr static const char* name = "Importer";
    constexpr static const char* qual_name = "dballe.Importer";
    constexpr static const char* doc = R"(
Message importer.

This is the engine that decodes binary messages and interprets their contents
using a uniform data model.

Note that one binary message is often decoded to multiple data messages, in
case, for example, of compressed BUFR files.

Constructor: Importer(encoding: str, simplified: bool=True)

:arg encoding: can be :code:`"BUFR"` or :code:`"CREX"`.
:arg simplified: controls whether messages are constructed using standard levels and
                 time ranges, or using the exact levels and time ranges
                 contained in the input. For example, a simplified
                 intepretation of a synop message will place the temperature at
                 2M above ground, regardless of the reported sensor height. A
                 non-simplified import will place the temperature reading at
                 the reported sensor height.

When a message is imported in simplified mode, the actual context information
will be stored as data attributes.

Example usage::

    importer = dballe.Importer("BUFR")
    with importer.from_file("test.bufr") as f:
        for msgs in f:
            for msg in msgs:
                print("{m.report},{m.coords},{m.ident},{m.datetime},{m.type}".format(m=msg))

    importer = dballe.Importer("BUFR")
    with dballe.File("test.bufr", "BUFR") as f:
        for binmsg in f:
            msgs = importer.from_binary(binmsg)
            for msg in msgs:
                print("#{b.index}: {m.report},{m.coords},{m.ident},{m.datetime},{m.type}".format(b=binmsg, m=msg))
)";

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
            impl::ImporterOptions opts;
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

dpy_Importer* importer_create(Encoding encoding, const dballe::ImporterOptions& opts)
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
    importerfile::definition->define(dpy_ImporterFile_Type, m);

    importer::definition = new importer::Definition;
    importer::definition->define(dpy_Importer_Type, m);
}

}
}

