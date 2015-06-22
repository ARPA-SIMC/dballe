#include "var.h"
#include "common.h"
#include "varinfo.h"

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
#endif

using namespace std;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyObject* dpy_Var_code(dpy_Var* self, void* closure)
{
    return format_varcode(self->var.code());
}
static PyObject* dpy_Var_isset(dpy_Var* self, void* closure) {
    if (self->var.isset())
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}
static PyObject* dpy_Var_info(dpy_Var* self, void* closure) {
    return (PyObject*)varinfo_create(self->var.info());
}

static PyGetSetDef dpy_Var_getsetters[] = {
    {"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    {"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {"info", (getter)dpy_Var_info, NULL, "Varinfo for this variable", NULL },
    {NULL}
};

static PyObject* dpy_Var_enqi(dpy_Var* self)
{
    try {
        return PyInt_FromLong(self->var.enqi());
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Var_enqd(dpy_Var* self)
{
    try {
        return PyFloat_FromDouble(self->var.enqd());
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Var_enqc(dpy_Var* self)
{
    try {
        return PyUnicode_FromString(self->var.enqc());
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Var_enq(dpy_Var* self)
{
    return var_value_to_python(self->var);
}

static PyObject* dpy_Var_get(dpy_Var* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "default", NULL };
    PyObject* def = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", kwlist, &def))
        return NULL;
    if (self->var.isset())
        return var_value_to_python(self->var);
    else
    {
        Py_INCREF(def);
        return def;
    }
}

static PyObject* dpy_Var_format(dpy_Var* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "default", NULL };
    const char* def = "";
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|s", kwlist, &def))
        return NULL;
    std::string f = self->var.format(def);
    return PyUnicode_FromString(f.c_str());
}

static PyMethodDef dpy_Var_methods[] = {
    {"enqi", (PyCFunction)dpy_Var_enqi, METH_NOARGS, "get the value of the variable, as an int" },
    {"enqd", (PyCFunction)dpy_Var_enqd, METH_NOARGS, "get the value of the variable, as a float" },
    {"enqc", (PyCFunction)dpy_Var_enqc, METH_NOARGS, "get the value of the variable, as a str" },
    {"enq", (PyCFunction)dpy_Var_enq, METH_NOARGS, "get the value of the variable, as int, float or str according the variable definition" },
    {"get", (PyCFunction)dpy_Var_get, METH_VARARGS | METH_KEYWORDS, "get the value of the variable, with a default if it is unset" },
    {"format", (PyCFunction)dpy_Var_format, METH_VARARGS | METH_KEYWORDS, "format the value of the variable to a string" },
    {NULL}
};

static int dpy_Var_init(dpy_Var* self, PyObject* args, PyObject* kw)
{
    // People should not invoke Var() as a constructor, but if they do,
    // this is better than a segfault later on
    PyErr_SetString(PyExc_NotImplementedError, "Var objects cannot be constructed explicitly");
    return -1;
}

static void dpy_Var_dealloc(dpy_Var* self)
{
    // Explicitly call destructor
    self->var.~Var();
}

static PyObject* dpy_Var_str(dpy_Var* self)
{
    std::string f = self->var.format("None");
    return PyUnicode_FromString(f.c_str());
}

static PyObject* dpy_Var_repr(dpy_Var* self)
{
    string res = "Var('";
    res += varcode_format(self->var.code());
    if (self->var.info()->is_string())
    {
        res += "', '";
        res += self->var.format();
        res += "')";
    } else {
        res += "', ";
        res += self->var.format("None");
        res += ")";
    }
    return PyUnicode_FromString(res.c_str());
}

static int compare_values(const Var& a, const Var& b)
{
    // Keep in mind that varinfos could be different even if the varcodes
    // are the same, as is the case with varinfos modified by C modifiers
    // during BUFR decoding. If values are not strings, they should be
    // compared with enqd
    if (a.isset())
    {
        if (b.isset())
        {
            if (a.info()->is_string())
                return strcmp(a.value(), b.value());
            else {
                double diff = a.enqd() - b.enqd();
                if (diff < 0) return -1;
                if (diff > 0) return 1;
                return 0;
            }
        } else
            return 1;
    } else {
        if (b.isset())
            return -1;
        else
            return 0;
    }
}

static PyObject* dpy_Var_richcompare(dpy_Var* a, dpy_Var* b, int op)
{
    PyObject *result;
    int cmp = 0;

    // Make sure both arguments are Vars.
    if (!(dpy_Var_Check(a) && dpy_Var_Check(b))) {
        result = Py_NotImplemented;
        goto out;
    }

    // Start by numerically comparing codes
    cmp = a->var.code() - b->var.code();
    if (cmp == 0)
        // If they are the same, compare values.
        cmp = compare_values(a->var, b->var);

    switch (op) {
        case Py_LT: cmp = cmp <  0; break;
        case Py_LE: cmp = cmp <= 0; break;
        case Py_EQ: cmp = cmp == 0; break;
        case Py_NE: cmp = cmp != 0; break;
        case Py_GT: cmp = cmp >  0; break;
        case Py_GE: cmp = cmp >= 0; break;
        default:
            result = Py_NotImplemented;
        goto out;
    }
    result = cmp ? Py_True : Py_False;

out:
    Py_INCREF(result);
    return result;
}

PyTypeObject dpy_Var_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.Var",              // tp_name
    sizeof(dpy_Var),           // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_Var_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Var_repr,    // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Var_str,     // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    R"(
        Var holds a measured value, which can be integer, float or string, and
        a `dballe.Varinfo`_ with all available information (description, unit,
        precision, ...) related to it.

        Var objects cannot be created directly, and need to be instantiated via
        a `dballe.Vartable`_ object. To create a Var using the default DB-All.e
        vartable, use the method `dballe.var()`_.

        Examples::

            v = dballe.var("B12101", 32.5)
            # v.info returns detailed informations about the variable in a Varinfo object.
            print("%s: %s %s %s" % (v.code, str(v), v.info.unit, v.info.desc))
    )",                        // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    (richcmpfunc)dpy_Var_richcompare, // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_Var_methods,           // tp_methods
    0,                         // tp_members
    dpy_Var_getsetters,        // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_Var_init,    // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

PyObject* var_value_to_python(const wreport::Var& v)
{
    try {
        if (v.info()->is_string())
            return PyUnicode_FromString(v.enqc());
        else if (v.info()->scale == 0)
            return PyInt_FromLong(v.enqi());
        else
            return PyFloat_FromDouble(v.enqd());
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

dpy_Var* var_create(const wreport::Varinfo& v)
{
    dpy_Var* result = PyObject_New(dpy_Var, &dpy_Var_Type);
    if (!result) return NULL;
    new (&result->var) Var(v);
    return result;
}

dpy_Var* var_create(const wreport::Varinfo& v, int val)
{
    dpy_Var* result = PyObject_New(dpy_Var, &dpy_Var_Type);
    if (!result) return NULL;
    new (&result->var) Var(v, val);
    return result;
}

dpy_Var* var_create(const wreport::Varinfo& v, double val)
{
    dpy_Var* result = PyObject_New(dpy_Var, &dpy_Var_Type);
    if (!result) return NULL;
    new (&result->var) Var(v, val);
    return result;
}

dpy_Var* var_create(const wreport::Varinfo& v, const char* val)
{
    dpy_Var* result = PyObject_New(dpy_Var, &dpy_Var_Type);
    if (!result) return NULL;
    new (&result->var) Var(v, val);
    return result;
}

dpy_Var* var_create(const wreport::Var& v)
{
    dpy_Var* result = PyObject_New(dpy_Var, &dpy_Var_Type);
    if (!result) return NULL;
    new (&result->var) Var(v);
    return result;
}

void register_var(PyObject* m)
{
    dpy_Var_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Var_Type) < 0)
        return;

    Py_INCREF(&dpy_Var_Type);
    PyModule_AddObject(m, "Var", (PyObject*)&dpy_Var_Type);
}

}
}
