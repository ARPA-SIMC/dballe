#include <Python.h>
#include "dballe/core/defs.h"
#include "dballe/core/query.h"
#include "dballe/core/data.h"
#include "dballe/core/var.h"
#include "record.h"
#include "common.h"
#include "types.h"
#include <vector>
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;


namespace dballe {
namespace python {

void read_query(PyObject* from_python, dballe::core::Query& query)
{
    query.clear();
    if (!from_python || from_python == Py_None)
        return;

    if (PyDict_Check(from_python))
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
        {
            std::string k = string_from_python(key);
            query_setpy(query, k.data(), k.size(), value);
        }
        query.validate();
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

void read_data(PyObject* from_python, dballe::core::Data& data)
{
    data.clear();
    if (!from_python || from_python == Py_None)
        return;

    if (PyDict_Check(from_python))
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
        {
            std::string k = string_from_python(key);
            data_setpy(data, k.data(), k.size(), value);
        }
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

void read_values(PyObject* from_python, dballe::Values& values)
{
    if (!from_python || from_python == Py_None)
    {
        values.clear();
        return;
    }

    if (PyDict_Check(from_python))
    {
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
            set_values_from_python(values, varcode_from_python(key), value);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dict or None");
    throw PythonException();
}

void set_var(PyObject* dict, const wreport::Var& var)
{
    char bcode[7];
    format_bcode(var.code(), bcode);
    pyo_unique_ptr pyvar((PyObject*)throw_ifnull(wrpy->var_create_copy(var)));
    if (PyDict_SetItemString(dict, bcode, pyvar))
        throw PythonException();
}

}
}
