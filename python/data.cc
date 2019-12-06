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
key-value representation of a value with its associated metadata.

This is used when inserting values in a database, and can be indexed and
assigned using insert parameters: see :ref:`parms_insert` for a list.

Indexing by variable code also works. Assignment can take None, int, str,
float, or a wreport.Var object. Assigning a wreport.Var object with a different
varcode performs automatic unit conversion if possible.

For example::

    # Select B12001 values and convert them to B12101
    with tr.query_data({"var": "B12001"}) as cur:
        self.assertEqual(cur.remaining, 1)
        for rec in cur:
            data = rec.data
            rec.remove()
            # This converts units automatically
            data["B12101"] = data["12001"]
            del data["B12001"]
            tr.insert_data(data)
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
        std::string res = name;
        res += "(station:";
        res += self->data->station.to_string();
        res += ", datetime:";
        res += self->data->datetime.to_string();
        res += ", level:";
        res += self->data->level.to_string();
        res += ", trange:";
        res += self->data->trange.to_string();
        for (const auto& val: self->data->values)
        {
            res += ", ";
            res += varcode_format(val->code());
            res += ":";
            res += val->format();
        }
        res += ")";
        return PyUnicode_FromStringAndSize(res.data(), res.size());
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
                data_unsetpy(*self->data, key, len);
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
