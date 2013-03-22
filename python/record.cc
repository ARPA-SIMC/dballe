/*
 * python/record - DB-All.e Record python bindings
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include <Python.h>
#include <datetime.h>
#include "record.h"
#include "common.h"
#include "var.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

/*
static PyObject* dpy_Var_code(dpy_Var* self, void* closure) { return format_varcode(self->var.code()); }
static PyObject* dpy_Var_isset(dpy_Var* self, void* closure) {
    if (self->var.isset())
        return Py_True;
    else
        return Py_False;
}
*/

static PyGetSetDef dpy_Record_getsetters[] = {
    //{"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    //{"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {NULL}
};

/*
static PyObject* dpy_Var_enqi(dpy_Var* self)
{
    try {
        return PyInt_FromLong(self->var.enqi());
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Var_enqd(dpy_Var* self)
{
    try {
        return PyFloat_FromDouble(self->var.enqd());
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Var_enqc(dpy_Var* self)
{
    try {
        return PyString_FromString(self->var.enqc());
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Var_enq(dpy_Var* self)
{
    try {
        if (self->var.info()->is_string())
            return PyString_FromString(self->var.enqc());
        else if (self->var.info()->scale == 0)
            return PyInt_FromLong(self->var.enqi());
        else
            return PyFloat_FromDouble(self->var.enqd());
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}
*/

static PyMethodDef dpy_Record_methods[] = {
    /*
    {"enqi", (PyCFunction)dpy_Var_enqi, METH_NOARGS, "get the value of the variable, as an int" },
    {"enqd", (PyCFunction)dpy_Var_enqd, METH_NOARGS, "get the value of the variable, as a float" },
    {"enqc", (PyCFunction)dpy_Var_enqc, METH_NOARGS, "get the value of the variable, as a str" },
    {"enq", (PyCFunction)dpy_Var_enq, METH_NOARGS, "get the value of the variable, as the int, float or str according the variable definition" },
    */
    {NULL}
};

static int dpy_Record_init(dpy_Record* self, PyObject* args, PyObject* kw)
{
    // Construct on preallocated memory
    new (&self->rec) dballe::Record;
    return 0;
}

static void dpy_Record_dealloc(dpy_Record* self)
{
    // Explicitly call destructor
    self->rec.~Record();
}

static PyObject* dpy_Record_str(dpy_Record* self)
{
    /*
    std::string f = self->var.format("None");
    return PyString_FromString(f.c_str());
    */
    return PyString_FromString("Record");
}

static PyObject* dpy_Record_repr(dpy_Record* self)
{
    /*
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
    return PyString_FromString(res.c_str());
    */
    return PyString_FromString("Record object");
}

static PyObject* rec_to_datetime(dpy_Record* self,
        dba_keyword ky, dba_keyword km, dba_keyword kd,
        dba_keyword kho, dba_keyword kmi, dba_keyword kse)
{
    try {
        int y = self->rec[ky].enqi();
        int m = self->rec[km].enqi();
        int d = self->rec[kd].enqi();
        int ho = self->rec[kho].enqi();
        int mi = self->rec[kmi].enqi();
        int se = self->rec[kse].enqi();
        return PyDateTime_FromDateAndTime(y, m, d, ho, mi, se, 0);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static int datetime_to_rec(dpy_Record* self, PyObject* dt,
        dba_keyword ky, dba_keyword km, dba_keyword kd,
        dba_keyword kho, dba_keyword kmi, dba_keyword kse)
{
    if (dt == NULL)
    {
        self->rec.unset(ky);
        self->rec.unset(km);
        self->rec.unset(kd);
        self->rec.unset(kho);
        self->rec.unset(kmi);
        self->rec.unset(kse);
    } else {
        if (!PyDateTime_Check(dt))
        {
            PyErr_SetString(PyExc_TypeError, "value must be an instance of datetime.datetime");
            return -1;
        }
        self->rec.set(ky, PyDateTime_GET_YEAR((PyDateTime_DateTime*)dt));
        self->rec.set(km, PyDateTime_GET_MONTH((PyDateTime_DateTime*)dt));
        self->rec.set(kd, PyDateTime_GET_DAY((PyDateTime_DateTime*)dt));
        self->rec.set(kho, PyDateTime_DATE_GET_HOUR((PyDateTime_DateTime*)dt));
        self->rec.set(kmi, PyDateTime_DATE_GET_MINUTE((PyDateTime_DateTime*)dt));
        self->rec.set(kse, PyDateTime_DATE_GET_SECOND((PyDateTime_DateTime*)dt));
    }
    return 0;
}

static PyObject* rec_to_level(dpy_Record* self)
{
    try {
        int lt1 = self->rec[DBA_KEY_LEVELTYPE1].enqi();
        int l1 = self->rec[DBA_KEY_L1].enqi();
        int lt2 = self->rec[DBA_KEY_LEVELTYPE2].enqi();
        int l2 = self->rec[DBA_KEY_L2].enqi();
        return Py_BuildValue("(iiii)", lt1, l1, lt2, l2);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static int level_to_rec(dpy_Record* self, PyObject* l)
{
    if (l == NULL)
    {
        // FIXME: this needs None for the trailing unset parts
        self->rec.unset(DBA_KEY_LEVELTYPE1);
        self->rec.unset(DBA_KEY_L1);
        self->rec.unset(DBA_KEY_LEVELTYPE2);
        self->rec.unset(DBA_KEY_L2);
    } else {
        if (!PySequence_Check(l))
        {
            PyErr_SetString(PyExc_TypeError, "value must be a sequence");
            return -1;
        }

        Py_ssize_t len = PySequence_Length(l);
        if (len > 4)
        {
            PyErr_SetString(PyExc_TypeError, "value must be a sequence of up to 4 elements");
            return -1;
        }

        PyObject* v;
        int i;

        switch (len)
        {
            case 4:
                if ((v = PySequence_GetItem(l, 3)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_L2, i);
            case 3:
                if ((v = PySequence_GetItem(l, 2)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_LEVELTYPE2, i);
            case 2:
                if ((v = PySequence_GetItem(l, 1)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_L1, i);
            case 1:
                if ((v = PySequence_GetItem(l, 0)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_LEVELTYPE1, i);
        }
    }
    return 0;
}

static PyObject* rec_to_trange(dpy_Record* self)
{
    try {
        // FIXME: this needs None for the trailing unset parts
        int pind = self->rec[DBA_KEY_PINDICATOR].enqi();
        int p1 = self->rec[DBA_KEY_P1].enqi();
        int p2 = self->rec[DBA_KEY_P2].enqi();
        return Py_BuildValue("(iii)", pind, p1, p2);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static int trange_to_rec(dpy_Record* self, PyObject* l)
{
    if (l == NULL)
    {
        self->rec.unset(DBA_KEY_PINDICATOR);
        self->rec.unset(DBA_KEY_P1);
        self->rec.unset(DBA_KEY_P2);
    } else {
        if (!PySequence_Check(l))
        {
            PyErr_SetString(PyExc_TypeError, "value must be a sequence");
            return -1;
        }

        Py_ssize_t len = PySequence_Length(l);
        if (len > 3)
        {
            PyErr_SetString(PyExc_TypeError, "value must be a sequence of up to 3 elements");
            return -1;
        }

        PyObject* v;
        int i;

        switch (len)
        {
            case 3:
                if ((v = PySequence_GetItem(l, 2)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_P2, i);
            case 2:
                if ((v = PySequence_GetItem(l, 1)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_P1, i);
            case 1:
                if ((v = PySequence_GetItem(l, 0)) == NULL) return -1;
                i = PyInt_AsLong(v);
                if (i == -1 && PyErr_Occurred()) return -1;
                self->rec.set(DBA_KEY_PINDICATOR, i);
        }
    }
    return 0;
}

PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key)
{
    const char* varname = PyString_AsString(key);
    if (varname == NULL)
        return NULL;

    // Just look at the first character to see if we need to check for python
    // API specific keys
    switch (varname[0])
    {
        case 'd':
            if (strcmp(varname, "date") == 0)
            {
                return rec_to_datetime(self,
                            DBA_KEY_YEAR, DBA_KEY_MONTH, DBA_KEY_DAY,
                            DBA_KEY_HOUR, DBA_KEY_MIN, DBA_KEY_SEC);
            } else if (strcmp(varname, "datemin") == 0) {
                return rec_to_datetime(self,
                            DBA_KEY_YEARMIN, DBA_KEY_MONTHMIN, DBA_KEY_DAYMIN,
                            DBA_KEY_HOURMIN, DBA_KEY_MINUMIN, DBA_KEY_SECMIN);
            } else if (strcmp(varname, "datemax") == 0) {
                return rec_to_datetime(self,
                            DBA_KEY_YEARMAX, DBA_KEY_MONTHMAX, DBA_KEY_DAYMAX,
                            DBA_KEY_HOURMAX, DBA_KEY_MINUMAX, DBA_KEY_SECMAX);
            }
            break;
        case 'l':
            if (strcmp(varname, "level") == 0)
                return rec_to_level(self);
            break;
        case 't':
            if (strcmp(varname, "trange") == 0 || strcmp(varname, "timerange") == 0)
                return rec_to_trange(self);
            break;
    }

    const Var* var = self->rec.peek(varname);
    if (var == NULL)
    {
        PyErr_SetString(PyExc_KeyError, varname);
        return NULL;
    }

    if (!var->isset())
    {
        PyErr_SetString(PyExc_KeyError, varname);
        return NULL;
    }

    return var_value_to_python(*var);
}

int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val)
{
    const char* varname = PyString_AsString(key);
    if (varname == NULL)
        return -1;

    // Just look at the first character to see if we need to check for python
    // API specific keys
    switch (varname[0])
    {
        case 'd':
            if (strcmp(varname, "date") == 0)
            {
                return datetime_to_rec(self, val,
                            DBA_KEY_YEAR, DBA_KEY_MONTH, DBA_KEY_DAY,
                            DBA_KEY_HOUR, DBA_KEY_MIN, DBA_KEY_SEC);
            } else if (strcmp(varname, "datemin") == 0) {
                return datetime_to_rec(self, val,
                            DBA_KEY_YEARMIN, DBA_KEY_MONTHMIN, DBA_KEY_DAYMIN,
                            DBA_KEY_HOURMIN, DBA_KEY_MINUMIN, DBA_KEY_SECMIN);
            } else if (strcmp(varname, "datemax") == 0) {
                return datetime_to_rec(self, val,
                            DBA_KEY_YEARMAX, DBA_KEY_MONTHMAX, DBA_KEY_DAYMAX,
                            DBA_KEY_HOURMAX, DBA_KEY_MINUMAX, DBA_KEY_SECMAX);
            }
            break;
        case 'l':
            if (strcmp(varname, "level") == 0)
                return level_to_rec(self, val);
        case 't':
            if (strcmp(varname, "trange") == 0 || strcmp(varname, "timerange") == 0)
                return trange_to_rec(self, val);
    }

    if (val == NULL)
    {
        // del rec[val]
        self->rec.unset(varname);
        return 0;
    }

    if (PyFloat_Check(val))
    {
        double v = PyFloat_AsDouble(val);
        if (v == -1.0 && PyErr_Occurred())
            return -1;
        self->rec.set(varname, v);
    } else if (PyInt_Check(val)) {
        long v = PyInt_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            return -1;
        self->rec.set(varname, (int)v);
    } else if (PyString_Check(val)) {
        const char* v = PyString_AsString(val);
        if (v == NULL)
            return -1;
        self->rec.set(varname, v);
    } else if (val == Py_None) {
        self->rec.unset(varname);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str or None");
        return -1;
    }
    return 0;
}

static PyMappingMethods dpy_Record_mapping = {
    0,                                 // __len__
    (binaryfunc)dpy_Record_getitem,    // __getitem__
    (objobjargproc)dpy_Record_setitem, // __setitem__
};

PyTypeObject dpy_Record_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.Record",           // tp_name
    sizeof(dpy_Record),        // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_Record_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Record_repr, // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    &dpy_Record_mapping,       // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Record_str,  // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e Record",         // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_Record_methods,        // tp_methods
    0,                         // tp_members
    dpy_Record_getsetters,     // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_Record_init, // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

void register_record(PyObject* m)
{
    PyDateTime_IMPORT;

    dpy_Record_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Record_Type) < 0)
        return;

    Py_INCREF(&dpy_Record_Type);
    PyModule_AddObject(m, "Record", (PyObject*)&dpy_Record_Type);
}

}
}
