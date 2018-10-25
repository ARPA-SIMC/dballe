#include <Python.h>
#include <wreport/python.h>
#include "common.h"
#include "types.h"
#include "record.h"
#include "db.h"
#include "cursor.h"
#if PY_MAJOR_VERSION >= 3
#include "message.h"
#include "explorer.h"
#endif
#include "dballe/types.h"
#include "dballe/var.h"
#include "config.h"

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_Check PyLong_Check
    #define Py_TPFLAGS_HAVE_ITER 0
#endif

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
    return (PyObject*)wrpy->varinfo_create(dballe::varinfo(varcode_parse(var_name)));
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
                return NULL;
            return (PyObject*)wrpy->var_create_d(dballe::varinfo(resolve_varcode(var_name)), v);
        } else if (PyInt_Check(val)) {
            long v = PyInt_AsLong(val);
            if (v == -1 && PyErr_Occurred())
                return NULL;
            return (PyObject*)wrpy->var_create_i(dballe::varinfo(resolve_varcode(var_name)), (int)v);
        } else if (
                PyUnicode_Check(val)
#if PY_MAJOR_VERSION >= 3
                || PyBytes_Check(val)
#else
                || PyString_Check(val)
#endif
                ) {
            string v;
            if (string_from_python(val, v))
                return NULL;
            return (PyObject*)wrpy->var_create_c(dballe::varinfo(resolve_varcode(var_name)), v.c_str());
        } else if (val == Py_None) {
            return (PyObject*)wrpy->var_create(dballe::varinfo(resolve_varcode(var_name)));
        } else {
            PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
            return NULL;
        }
    } else
        return (PyObject*)wrpy->var_create(dballe::varinfo(resolve_varcode(var_name)));
}

static PyObject* dballe_var(PyTypeObject *type, PyObject *args)
{
    try {
        return dballe_var_uncaught(type, args);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

#define get_int_or_missing(intvar, ovar) \
    int intvar; \
    if (ovar == Py_None) \
        intvar = MISSING_INT; \
    else { \
        intvar = PyInt_AsLong(ovar); \
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
    {"varinfo", (PyCFunction)dballe_varinfo, METH_VARARGS, "Query the DB-All.e variable table returning a Varinfo" },
    {"var", (PyCFunction)dballe_var, METH_VARARGS, "Query the DB-All.e variable table returning a Var, optionally initialized with a value" },
    {"describe_level", (PyCFunction)dballe_describe_level, METH_VARARGS | METH_KEYWORDS, "Return a string description for a level" },
    {"describe_trange", (PyCFunction)dballe_describe_trange, METH_VARARGS | METH_KEYWORDS, "Return a string description for a time range" },
    { NULL }
};

#if PY_MAJOR_VERSION >= 3
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
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit__dballe(void)
#else
PyMODINIT_FUNC init_dballe(void)
#endif
{
    using namespace dballe::python;

    PyObject* m;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&dballe_module);
#else
    m = Py_InitModule3("_dballe", dballe_methods,
            "DB-All.e Python interface.");
#endif

    register_types(m);
    register_record(m);
    register_db(m);
    register_cursor(m);

#if PY_MAJOR_VERSION >= 3
    register_message(m);
    register_explorer(m);

    return m;
#endif
}

}
