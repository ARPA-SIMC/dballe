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
#include <dballe/core/defs.h>
#include "record.h"
#include "common.h"
#include "var.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static dba_keyword date_keys[6] = { DBA_KEY_YEAR, DBA_KEY_MONTH, DBA_KEY_DAY, DBA_KEY_HOUR, DBA_KEY_MIN, DBA_KEY_SEC };
static dba_keyword datemin_keys[6] = { DBA_KEY_YEARMIN, DBA_KEY_MONTHMIN, DBA_KEY_DAYMIN, DBA_KEY_HOURMIN, DBA_KEY_MINUMIN, DBA_KEY_SECMIN };
static dba_keyword datemax_keys[6] = { DBA_KEY_YEARMAX, DBA_KEY_MONTHMAX, DBA_KEY_DAYMAX, DBA_KEY_HOURMAX, DBA_KEY_MINUMAX, DBA_KEY_SECMAX };
static dba_keyword level_keys[4] = { DBA_KEY_LEVELTYPE1, DBA_KEY_L1, DBA_KEY_LEVELTYPE2, DBA_KEY_L2 };
static dba_keyword trange_keys[3] = { DBA_KEY_PINDICATOR, DBA_KEY_P1, DBA_KEY_P2 };

/*
 * Record iterator
 */

typedef struct {
    PyObject_HEAD
    // Pointer to the container record, to hold a reference to it
    dpy_Record* rec;
    std::vector<wreport::Var*>::const_iterator iter;
} dpy_RecordIter;

static void dpy_RecordIter_dealloc(dpy_RecordIter* self)
{
    Py_DECREF(self->rec);
}

static PyObject* dpy_RecordIter_iter(dpy_RecordIter* self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

static PyObject* dpy_RecordIter_iternext(dpy_RecordIter* self)
{
    if (self->iter == self->rec->rec.vars().end())
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    wreport::Varcode result = (*self->iter)->code();
    ++(self->iter);
    return format_varcode(result);
}

PyTypeObject dpy_RecordIter_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.RecordIter",           // tp_name
    sizeof(dpy_RecordIter),        // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_RecordIter_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    0,                         // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    0,                         // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER, // tp_flags
    "DB-All.e Record Iterator", // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)dpy_RecordIter_iter,      // tp_iter
    (iternextfunc)dpy_RecordIter_iternext, // tp_iternext
    0,                         // tp_methods
    0,                         // tp_members
    0,                         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    0,                         // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};


/*
 * Record
 */

static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val);
static int dpy_Record_contains(dpy_Record* self, PyObject *value);
static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key);

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

static PyObject* dpy_Record_copy(dpy_Record* self)
{
    dpy_Record* result = PyObject_New(dpy_Record, &dpy_Record_Type);
    if (!result) return NULL;
    result = (dpy_Record*)PyObject_Init((PyObject*)result, &dpy_Record_Type);
    new (&result->rec) Record(self->rec);
    return (PyObject*)result;
}

static PyObject* dpy_Record_keys(dpy_Record* self)
{
    const std::vector<wreport::Var*>& vars = self->rec.vars();

    PyObject* result = PyTuple_New(vars.size());
    if (!result) return NULL;

    for (size_t i = 0; i < vars.size(); ++i)
    {
        PyObject* v = format_varcode(vars[i]->code());
        if (!v)
        {
            Py_DECREF(result);
            return NULL;
        }

        if (!PyTuple_SetItem(result, i, v) == -1)
        {
            // No need to Py_DECREF v, because PyTuple_SetItem steals its
            // reference
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

static PyObject* dpy_Record_var(dpy_Record* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "code", NULL };
    const char* name = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|s", kwlist, &name))
        return NULL;

    try {
        wreport::Varcode code;
        if (name == NULL)
        {
            const char* scode = self->rec.get(DBA_KEY_VAR).enqc();
            code = WR_STRING_TO_VAR(scode + 1);
        } else
            code = resolve_varcode(name);

        return (PyObject*)var_create(self->rec.get(code));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_vars(dpy_Record* self)
{
    const std::vector<wreport::Var*>& vars = self->rec.vars();

    PyObject* result = PyTuple_New(vars.size());
    if (!result) return NULL;

    for (size_t i = 0; i < vars.size(); ++i)
    {
        PyObject* v = (PyObject*)var_create(*vars[i]);
        if (!v)
        {
            Py_DECREF(result);
            return NULL;
        }

        if (!PyTuple_SetItem(result, i, v) == -1)
        {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

static PyObject* dpy_Record_key(dpy_Record* self, PyObject* args)
{
    const char* name = NULL;

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    try {
        dba_keyword key = Record::keyword_byname(name);
        return (PyObject*)var_create(self->rec.get(key));
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_get(dpy_Record* self, PyObject *args, PyObject* kw)
{
    static char* kwlist[] = { "key", "default", NULL };
    PyObject* key;
    PyObject* def = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &def))
        return NULL;

    int has = dpy_Record_contains(self, key);
    if (has < 0) return NULL;
    if (!has)
    {
        Py_INCREF(def);
        return def;
    }

    return dpy_Record_getitem(self, key);
}

static PyObject* dpy_Record_update(dpy_Record* self, PyObject *args, PyObject *kw)
{
    if (kw)
    {
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(kw, &pos, &key, &value))
            if (dpy_Record_setitem(self, key, value) < 0)
                return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_Record_date_extremes(dpy_Record* self)
{
    int minvals[6];
    int maxvals[6];
    self->rec.parse_date_extremes(minvals, maxvals);

    PyObject* dt_min;
    PyObject* dt_max;

    if (minvals[0] != MISSING_INT)
    {
        dt_min = PyDateTime_FromDateAndTime(
                minvals[0], minvals[1], minvals[2],
                minvals[3], minvals[4], minvals[5], 0);
        if (!dt_min) return NULL;
    } else {
        dt_min = Py_None;
        Py_INCREF(dt_min);
    }

    if (maxvals[0] != MISSING_INT)
    {
        dt_max = PyDateTime_FromDateAndTime(
                maxvals[0], maxvals[1], maxvals[2],
                maxvals[3], maxvals[4], maxvals[5], 0);
        if (!dt_max) return NULL; // FIXME: deallocate dt_min
    } else {
        dt_max = Py_None;
        Py_INCREF(dt_max);
    }

    return Py_BuildValue("(NN)", dt_min, dt_max);
}

static PyObject* dpy_Record_set_station_context(dpy_Record* self)
{
    self->rec.set_ana_context();
    Py_RETURN_NONE;
}

static PyObject* dpy_Record_clear(dpy_Record* self)
{
    self->rec.clear();
    Py_RETURN_NONE;
}

static PyObject* dpy_Record_clear_vars(dpy_Record* self)
{
    self->rec.clear_vars();
    Py_RETURN_NONE;
}

static PyObject* dpy_Record_set_from_string(dpy_Record* self, PyObject *args)
{
    const char* str = NULL;
    if (!PyArg_ParseTuple(args, "s", &str))
        return NULL;

    try {
        self->rec.set_from_string(str);
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyMethodDef dpy_Record_methods[] = {
    {"clear", (PyCFunction)dpy_Record_clear, METH_NOARGS, "remove all data from the record" },
    {"clear_vars", (PyCFunction)dpy_Record_clear_vars, METH_NOARGS, "remove all variables from the record, leaving the keywords intact" },
    {"key", (PyCFunction)dpy_Record_key, METH_VARARGS, "return a var key from the record"},
    {"get", (PyCFunction)dpy_Record_get, METH_VARARGS | METH_KEYWORDS, "lookup a value, returning a fallback value (None by default) if unset" },
    {"copy", (PyCFunction)dpy_Record_copy, METH_NOARGS, "return a copy of the Record" },
    {"var", (PyCFunction)dpy_Record_var, METH_VARARGS | METH_KEYWORDS, "return a variable from the record. If no varcode is given, use record['var']" },
    {"keys", (PyCFunction)dpy_Record_keys, METH_NOARGS, "return a sequence with all the varcodes of the variables set on the Record. Note that this does not include keys." },
    {"vars", (PyCFunction)dpy_Record_vars, METH_NOARGS, "return a sequence with all the variables set on the Record. Note that this does not include keys." },
    {"update", (PyCFunction)dpy_Record_update, METH_VARARGS | METH_KEYWORDS, "set many record keys/vars in a single shot, via kwargs" },
    {"date_extremes", (PyCFunction)dpy_Record_date_extremes, METH_NOARGS, "get two datetime objects with the lower and upper bounds of the datetime period in this record" },
    {"set_station_context", (PyCFunction)dpy_Record_set_station_context, METH_NOARGS, "set the date, level and time range values to match the station data context" },
    {"set_from_string", (PyCFunction)dpy_Record_set_from_string, METH_VARARGS, "set values from a 'key=val' string" },
    {NULL}
};

static int dpy_Record_init(dpy_Record* self, PyObject* args, PyObject* kw)
{
    // Construct on preallocated memory
    new (&self->rec) dballe::Record;

    if (kw)
    {
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(kw, &pos, &key, &value))
            if (dpy_Record_setitem(self, key, value) < 0)
                return -1;
    }

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

static PyObject* rec_to_datetime(dpy_Record* self, dba_keyword* keys)
{
    try {
        int y = self->rec[keys[0]].enqi();
        int m = self->rec[keys[1]].enqi();
        int d = self->rec[keys[2]].enqi();
        int ho = self->rec[keys[3]].enqi();
        int mi = self->rec[keys[4]].enqi();
        // Second is optional, defaulting to 0
        int se = self->rec.get(keys[5], 0);
        return PyDateTime_FromDateAndTime(y, m, d, ho, mi, se, 0);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static int datetime_to_rec(dpy_Record* self, PyObject* dt, dba_keyword* keys)
{
    if (dt == NULL || dt == Py_None)
    {
        for (unsigned i = 0; i < 6; ++i)
            self->rec.unset(keys[i]);
    } else {
        if (!PyDateTime_Check(dt))
        {
            PyErr_SetString(PyExc_TypeError, "value must be an instance of datetime.datetime");
            return -1;
        }
        self->rec.set(keys[0], PyDateTime_GET_YEAR((PyDateTime_DateTime*)dt));
        self->rec.set(keys[1], PyDateTime_GET_MONTH((PyDateTime_DateTime*)dt));
        self->rec.set(keys[2], PyDateTime_GET_DAY((PyDateTime_DateTime*)dt));
        self->rec.set(keys[3], PyDateTime_DATE_GET_HOUR((PyDateTime_DateTime*)dt));
        self->rec.set(keys[4], PyDateTime_DATE_GET_MINUTE((PyDateTime_DateTime*)dt));
        self->rec.set(keys[5], PyDateTime_DATE_GET_SECOND((PyDateTime_DateTime*)dt));
    }
    return 0;
}

static PyObject* rec_keys_to_tuple(dpy_Record* self, dba_keyword* keys, unsigned len)
{
    PyObject* res = PyTuple_New(len);
    if (!res) return NULL;

    try {
        for (unsigned i = 0; i < len; ++i)
        {
            if (self->rec.peek_value(keys[i]))
            {
                int iv = self->rec[keys[i]].enqi();
                if (iv == MISSING_INT)
                {
                    Py_INCREF(Py_None);
                    PyTuple_SET_ITEM(res, i, Py_None);
                } else {
                    PyObject* v = PyInt_FromLong(iv);
                    if (!v) return NULL; // FIXME: deallocate res
                    PyTuple_SET_ITEM(res, i, v);
                }
            } else {
                Py_INCREF(Py_None);
                PyTuple_SET_ITEM(res, i, Py_None);
            }
        }
        return res;
    } catch (wreport::error& e) {
        Py_DECREF(res);
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        Py_DECREF(res);
        return raise_std_exception(se);
    }
}

static PyObject* rec_to_level(dpy_Record* self)
{
    return rec_keys_to_tuple(self, level_keys, 4);
}

static int rec_tuple_to_keys(dpy_Record* self, PyObject* val, dba_keyword* keys, unsigned len)
{
    if (val == NULL || val == Py_None)
    {
        for (unsigned i = 0; i < len; ++i)
            self->rec.unset(keys[i]);
    } else {
        if (!PySequence_Check(val))
        {
            PyErr_SetString(PyExc_TypeError, "value must be a sequence");
            return -1;
        }

        Py_ssize_t seq_len = PySequence_Length(val);
        if (seq_len > len)
        {
            PyErr_Format(PyExc_TypeError, "value must be a sequence of up to %d elements", len);
            return -1;
        }

        for (unsigned i = 0; i < len; ++i)
        {
            if (i < seq_len)
            {
                PyObject* v = PySequence_GetItem(val, i);
                if (v == NULL) return -1;
                if (v == Py_None)
                {
                    self->rec.unset(keys[i]);
                    Py_DECREF(v);
                } else {
                    int vi = PyInt_AsLong(v);
                    Py_DECREF(v);
                    if (vi == -1 && PyErr_Occurred()) return -1;
                    self->rec.set(keys[i], vi);
                }
            } else
                self->rec.unset(keys[i]);
        }
    }
    return 0;
}

static int level_to_rec(dpy_Record* self, PyObject* l)
{
    return rec_tuple_to_keys(self, l, level_keys, 4);
}

static PyObject* rec_to_trange(dpy_Record* self)
{
    return rec_keys_to_tuple(self, trange_keys, 3);
}

static int trange_to_rec(dpy_Record* self, PyObject* l)
{
    return rec_tuple_to_keys(self, l, trange_keys, 3);
}

static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key)
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
                return rec_to_datetime(self, date_keys);
            } else if (strcmp(varname, "datemin") == 0) {
                return rec_to_datetime(self, datemin_keys);
            } else if (strcmp(varname, "datemax") == 0) {
                return rec_to_datetime(self, datemax_keys);
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

static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val)
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
                return datetime_to_rec(self, val, date_keys);
            } else if (strcmp(varname, "datemin") == 0) {
                return datetime_to_rec(self, val, datemin_keys);
            } else if (strcmp(varname, "datemax") == 0) {
                return datetime_to_rec(self, val, datemax_keys);
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
    } else if (PyUnicode_Check(val)) {
        PyObject *utf8 = PyUnicode_AsUTF8String(val);
        const char* v = PyString_AsString(utf8);
        if (v == NULL)
        {
            Py_DECREF(utf8);
            return -1;
        }
        self->rec.set(varname, v);
        Py_DECREF(utf8);
    } else if (val == Py_None) {
        self->rec.unset(varname);
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
        return -1;
    }
    return 0;
}

static int all_keys_set(const Record& rec, dba_keyword* keys, unsigned len)
{
    for (unsigned i = 0; i < len; ++i)
        if (rec.peek_value(keys[i]) == NULL)
            return 0;
    return 1;
}

static int any_key_set(const Record& rec, dba_keyword* keys, unsigned len)
{
    for (unsigned i = 0; i < len; ++i)
        if (rec.peek_value(keys[i]) != NULL)
            return 1;
    return 0;
}

static int dpy_Record_len(dpy_Record* self)
{
    return self->rec.vars().size();
}
static int dpy_Record_contains(dpy_Record* self, PyObject *value)
{
    const char* varname = PyString_AsString(value);
    if (varname == NULL)
        return -1;

    switch (varname[0])
    {
        case 'd':
            // We don't bother checking the seconds, since they default to 0 if
            // missing
            if (strcmp(varname, "date") == 0)
                return all_keys_set(self->rec, date_keys, 5);
            else if (strcmp(varname, "datemin") == 0)
                return all_keys_set(self->rec, datemin_keys, 5);
            else if (strcmp(varname, "datemax") == 0)
                return all_keys_set(self->rec, datemax_keys, 5);
            break;
        case 'l':
            if (strcmp(varname, "level") == 0)
                return any_key_set(self->rec, level_keys, 4);
            break;
        case 't':
            if (strcmp(varname, "trange") == 0 || strcmp(varname, "timerange") == 0)
                return any_key_set(self->rec, trange_keys, 3);
            break;
    }

    return self->rec.peek_value(varname) == NULL ? 0 : 1;
}

static PySequenceMethods dpy_Record_sequence = {
    (lenfunc)dpy_Record_len,         // sq_length
    0,                               // sq_concat
    0,                               // sq_repeat
    0,                               // sq_item
    0,                               // sq_slice
    0,                               // sq_ass_item
    0,                               // sq_ass_slice
    (objobjproc)dpy_Record_contains, // sq_contains
};
static PyMappingMethods dpy_Record_mapping = {
    0,                                 // __len__
    (binaryfunc)dpy_Record_getitem,    // __getitem__
    (objobjargproc)dpy_Record_setitem, // __setitem__
};

static PyObject* dpy_Record_iter(dpy_Record* self)
{
    dpy_RecordIter* result = PyObject_New(dpy_RecordIter, &dpy_RecordIter_Type);
    if (!result) return NULL;

    result = (dpy_RecordIter*)PyObject_Init((PyObject*)result, &dpy_RecordIter_Type);
    Py_INCREF(self);
    result->rec = self;
    result->iter = self->rec.vars().begin();
    return (PyObject*)result;
}

static PyObject* dpy_Record_richcompare(dpy_Record* a, dpy_Record* b, int op)
{
    PyObject *result;

    // Make sure both arguments are Records.
    if (!(dpy_Record_Check(a) && dpy_Record_Check(b))) {
        result = Py_NotImplemented;
        goto out;
    }

    int cmp;
    switch (op) {
        case Py_LT: cmp = cmp <  0; break;
        case Py_LE: cmp = cmp <= 0; break;
        case Py_EQ: cmp = a->rec == b->rec; break;
        case Py_NE: cmp = a->rec != b->rec; break;
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
    &dpy_Record_sequence,      // tp_as_sequence
    &dpy_Record_mapping,       // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Record_str,  // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER, // tp_flags
    "DB-All.e Record",         // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    (richcmpfunc)dpy_Record_richcompare, // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)dpy_Record_iter, // tp_iter
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

dpy_Record* record_create()
{
    return (dpy_Record*)PyObject_CallObject((PyObject*)&dpy_Record_Type, NULL);
}

void register_record(PyObject* m)
{
    PyDateTime_IMPORT;

    dpy_Record_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Record_Type) < 0)
        return;
    Py_INCREF(&dpy_Record_Type);

    dpy_RecordIter_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_RecordIter_Type) < 0)
        return;
    Py_INCREF(&dpy_Record_Type);

    PyModule_AddObject(m, "Record", (PyObject*)&dpy_Record_Type);
}

}
}
