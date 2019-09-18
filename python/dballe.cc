#include "config.h"
#include <wreport/python.h>
#include "common.h"
#include "types.h"
#include "data.h"
#include "db.h"
#include "cursor.h"
#include "binarymessage.h"
#include "file.h"
#include "message.h"
#include "importer.h"
#include "exporter.h"
#include "explorer.h"
#include "utils/wreport.h"
#include "dballe/python.h"
#include "dballe/types.h"
#include "dballe/var.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyObject* dballe_varinfo(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    const char* var_name;
    if (!PyArg_ParseTuple(args, "s", &var_name))
        return NULL;
    return wreport_api.varinfo_create(dballe::varinfo(varcode_parse(var_name)));
}

static PyObject* dballe_var_uncaught(PyTypeObject *type, PyObject *args)
{
    const char* var_name;
    PyObject* val = 0;
    if (!PyArg_ParseTuple(args, "s|O", &var_name, &val))
        return NULL;
    if (val)
    {
        if (PyFloat_Check(val))
        {
            double v = PyFloat_AsDouble(val);
            if (v == -1.0 && PyErr_Occurred())
                return nullptr;
            return wreport_api.var_create(dballe::varinfo(resolve_varcode(var_name)), v);
        } else if (PyLong_Check(val)) {
            long v = PyLong_AsLong(val);
            if (v == -1 && PyErr_Occurred())
                return nullptr;
            return wreport_api.var_create(dballe::varinfo(resolve_varcode(var_name)), (int)v);
        } else if (PyUnicode_Check(val) || PyBytes_Check(val)) {
            string v = string_from_python(val);
            return wreport_api.var_create(dballe::varinfo(resolve_varcode(var_name)), v);
        } else if (wreport_api.var_check(val)) {
            wreport::Var& src = wreport_api.var(val);
            return wreport_api.var_create(dballe::varinfo(resolve_varcode(var_name)), src);
        } else if (val == Py_None) {
            return wreport_api.var_create(dballe::varinfo(resolve_varcode(var_name)));
        } else {
            PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
            return NULL;
        }
    } else
        return wreport_api.var_create(dballe::varinfo(resolve_varcode(var_name)));
}

static PyObject* dballe_var(PyTypeObject *type, PyObject *args)
{
    try {
        return dballe_var_uncaught(type, args);
    } DBALLE_CATCH_RETURN_PYO
}

#define get_int_or_missing(intvar, ovar) \
    int intvar; \
    if (ovar == Py_None) \
        intvar = MISSING_INT; \
    else { \
        intvar = PyLong_AsLong(ovar); \
        if (intvar == -1 && PyErr_Occurred()) \
            return NULL; \
    }


static PyObject* dballe_describe_level(PyTypeObject *type, PyObject *args, PyObject* kw)
{
    static const char* kwlist[] = { "ltype1", "l1", "ltype2", "l2", NULL };
    PyObject* oltype1 = Py_None;
    PyObject* ol1 = Py_None;
    PyObject* oltype2 = Py_None;
    PyObject* ol2 = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|OOO", const_cast<char**>(kwlist), &oltype1, &ol1, &oltype2, &ol2))
        return NULL;

    get_int_or_missing(ltype1, oltype1);
    get_int_or_missing(l1, ol1);
    get_int_or_missing(ltype2, oltype2);
    get_int_or_missing(l2, ol2);

    Level lev(ltype1, l1, ltype2, l2);
    string desc = lev.describe();
    return PyUnicode_FromString(desc.c_str());
}

static PyObject* dballe_describe_trange(PyTypeObject *type, PyObject *args, PyObject* kw)
{
    static const char* kwlist[] = { "pind", "p1", "p2", NULL };
    PyObject* opind = Py_None;
    PyObject* op1 = Py_None;
    PyObject* op2 = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|OO", const_cast<char**>(kwlist), &opind, &op1, &op2))
        return NULL;

    get_int_or_missing(pind, opind);
    get_int_or_missing(p1, op1);
    get_int_or_missing(p2, op2);

    Trange tr(pind, p1, p2);
    string desc = tr.describe();
    return PyUnicode_FromString(desc.c_str());
}

static PyMethodDef dballe_methods[] = {
    {"varinfo", (PyCFunction)dballe_varinfo, METH_VARARGS, R"(
varinfo(str) -> str

Query the DB-All.e variable table returning a Varinfo)" },
    {"var", (PyCFunction)dballe_var, METH_VARARGS, R"(
var(code, val: Any=None) -> dballe.Var

Query the DB-All.e variable table returning a Var, optionally initialized with a value)" },
    {"describe_level", (PyCFunction)dballe_describe_level, METH_VARARGS | METH_KEYWORDS, R"(
describe_level(ltype1: int, l1: int=None, ltype2: int=None, l2: int=None) -> str

Return a string description for a level)" },
    {"describe_trange", (PyCFunction)dballe_describe_trange, METH_VARARGS | METH_KEYWORDS, R"(
describe_trange(pind: int, p1: int=None, p2: int=None) -> str

Return a string description for a time range)" },
    { NULL }
};


static PyModuleDef dballe_module = {
    PyModuleDef_HEAD_INIT,
    "_dballe",       /* m_name */
    "DB-All.e Python interface.",  /* m_doc */
    -1,             /* m_size */
    dballe_methods, /* m_methods */
    NULL,           /* m_reload */
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL,           /* m_free */

};

PyMODINIT_FUNC PyInit__dballe(void)
{
    using namespace dballe::python;

    static dbapy_c_api c_api;

    try {
        memset(&c_api, 0, sizeof(dbapy_c_api));
        c_api.version_major = 1;
        c_api.version_minor = 0;

        pyo_unique_ptr m(PyModule_Create(&dballe_module));
        PyModule_AddStringConstant(m, "__version__", PACKAGE_VERSION);
        register_types(m);
        register_data(m);
        register_binarymessage(m);
        register_file(m);
        register_message(m, c_api);
        register_importer(m);
        register_exporter(m);
        register_db(m);
        register_cursor(m);
        register_explorer(m);

        // Create a Capsule containing the API struct's address
        pyo_unique_ptr c_api_object(throw_ifnull(PyCapsule_New((void *)&c_api, "_dballe._C_API", nullptr)));
        int res = PyModule_AddObject(m, "_C_API", c_api_object.release());
        if (res)
            return nullptr;

        return m.release();
    } DBALLE_CATCH_RETURN_PYO
}

}
