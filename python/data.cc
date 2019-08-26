#define _DBALLE_LIBRARY_CODE
#include "dballe/core/data.h"
#include "enq.h"
#include "data.h"
#include "common.h"
#include "types.h"
#include "utils/type.h"
#include "utils/methods.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

PyTypeObject* dpy_Data_Type = nullptr;

}

namespace {

namespace data {

struct Definition : public Type<Definition, dpy_Data>
{
    constexpr static const char* name = "Data";
    constexpr static const char* qual_name = "dballe.Data";
    constexpr static const char* doc = R"(
key-value representation of a value with its associated metadata
)";
    GetSetters<> getsetters;
    Methods<> methods;

    static void _dealloc(Impl* self)
    {
        delete self->data;
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _str(Impl* self)
    {
        return PyUnicode_FromString(name);
    }

    static PyObject* _repr(Impl* self)
    {
        string res = qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { nullptr };
        if (!PyArg_ParseTupleAndKeywords(args, kw, "", const_cast<char**>(kwlist)))
            return -1;

        try {
            self->data = new core::Data;
        } DBALLE_CATCH_RETURN_INT

        // TODO: support a dict-like constructor?

        return 0;
    }

    static PyObject* mp_subscript(Impl* self, PyObject* pykey)
    {
        try {
            Py_ssize_t len;
            const char* key = throw_ifnull(PyUnicode_AsUTF8AndSize(pykey, &len));
            Enqpy enq(key, len);
            data_enq_generic(*self->data, enq);
            if (enq.missing)
            {
                PyErr_Format(PyExc_KeyError, "key %s not found", key);
                throw PythonException();
            }
            return enq.res;
        } DBALLE_CATCH_RETURN_PYO
    }

    static int mp_ass_subscript(Impl* self, PyObject *pykey, PyObject *val)
    {
        try {
            Py_ssize_t len;
            const char* key = throw_ifnull(PyUnicode_AsUTF8AndSize(pykey, &len));
            if (val)
                data_setpy(*self->data, key, len, val);
            else
            {
                // TODO: if v is nullptr, delete/unset
                PyErr_Format(PyExc_NotImplementedError, "mp_ass_subscript unset %s", key);
                throw PythonException();
            }
            return 0;
        } DBALLE_CATCH_RETURN_INT
    }

};

Definition* definition = nullptr;
}

}

namespace dballe {
namespace python {

dpy_Data* data_create()
{
    unique_ptr<core::Data> data(new core::Data);
    return data_create(move(data));
}

dpy_Data* data_create(std::unique_ptr<core::Data> data)
{
    dpy_Data* result = PyObject_New(dpy_Data, dpy_Data_Type);
    if (!result) return nullptr;

    result->data = data.release();
    return result;
}


void register_data(PyObject* m)
{
    common_init();

    data::definition = new data::Definition;
    data::definition->define(dpy_Data_Type, m);
}

}
}

#include "data-access.tcc"
