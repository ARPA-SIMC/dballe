#include <Python.h>
#include <dballe/core/record.h>
#include <dballe/core/defs.h>
#include "record.h"
#include "common.h"
#include "types.h"
#include <vector>
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

static const char* level_keys[4] = { "leveltype1", "l1", "leveltype2", "l2" };
static const char* trange_keys[3] = { "pindicator", "p1", "p2" };
static int any_key_set(const Record& rec, const char** keys, unsigned len)
{
    for (unsigned i = 0; i < len; ++i)
        if (rec.isset(keys[i]))
            return 1;
    return 0;
}


/*
 * Record
 */

static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val);
static int dpy_Record_contains(dpy_Record* self, PyObject *value);
static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key);

static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key)
{
    string varname;
    if (string_from_python(key, varname))
        return nullptr;

    try {
        // Just look at the first character to see if we need to check for python
        // API specific keys
        switch (varname[0])
        {
            case 'd':
                if (varname == "datetime" || varname == "date")
                {
                    if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                        return NULL;
                    auto dt = core::Record::downcast(*self->rec).get_datetime();
                    if (!dt.is_missing()) return datetime_to_python(dt);
                    PyErr_SetString(PyExc_KeyError, varname.c_str());
                    return NULL;
                } else if (varname == "datemin") {
                    if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                        return NULL;
                    auto dt = core::Record::downcast(*self->rec).get_datetimerange().min;
                    if (!dt.is_missing()) return datetime_to_python(dt);
                    PyErr_SetString(PyExc_KeyError, varname.c_str());
                    return NULL;
                } else if (varname == "datemax") {
                    if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                        return NULL;
                    auto dt = core::Record::downcast(*self->rec).get_datetimerange().max;
                    if (!dt.is_missing()) return datetime_to_python(dt);
                    PyErr_SetString(PyExc_KeyError, varname.c_str());
                    return NULL;
                }
                break;
            case 'l':
                if (varname == "level")
                {
                    if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                        return NULL;
                    auto lev = core::Record::downcast(*self->rec).get_level();
                    if (!lev.is_missing()) return level_to_python(lev);
                    PyErr_SetString(PyExc_KeyError, varname.c_str());
                    return NULL;
                }
                break;
            case 't':
                if (varname == "trange" || varname == "timerange")
                {
                    if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                        return NULL;
                    auto tr = core::Record::downcast(*self->rec).get_trange();
                    if (!tr.is_missing()) return trange_to_python(tr);
                    PyErr_SetString(PyExc_KeyError, varname.c_str());
                    return NULL;
                }
                break;
        }

        const Var* var = self->rec->get(varname.c_str());
        if (var == NULL)
            Py_RETURN_NONE;

        if (!var->isset())
            Py_RETURN_NONE;

        return wrpy->var_value_to_python(*var);
    } DBALLE_CATCH_RETURN_PYO
}

static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val)
{
    string varname;
    if (string_from_python(key, varname))
        return -1;

    try {
        // Check for shortcut keys
        if (varname == "datetime" || varname == "date")
        {
            if (varname == "date")
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] instead of rec[\"date\"]", 1))
                    return res;

            if (val && PySequence_Check(val))
            {
                DatetimeRange dtr;
                if (datetimerange_from_python(val, dtr)) return -1;
                self->rec->set(dtr);
            } else {
                Datetime dt;
                if (datetime_from_python(val, dt)) return -1;
                self->rec->set(dt);
            }
            self->station_context = false;
            return 0;
        }

        if (varname == "datemin") {
            if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] = (min, max) instead of rec[\"datemin\"]", 1))
                return res;
            DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();
            if (datetime_from_python(val, dtr.min)) return -1;
            self->rec->set(dtr);
            self->station_context = false;
            return 0;
        }

        if (varname == "datemax") {
            if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] = (min, max) instead of rec[\"datemax\"]", 1))
                return res;
            DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();
            if (datetime_from_python(val, dtr.max)) return -1;
            self->rec->set(dtr);
            self->station_context = false;
            return 0;
        }

        if (varname == "level")
        {
            Level lev;
            if (level_from_python(val, lev)) return -1;
            self->station_context = false;
            self->rec->set(lev);
            return 0;
        }

        if (varname == "trange" || varname == "timerange")
        {
            if (varname == "timerange")
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"trange\"] instead of rec[\"timerange\"]", 1))
                    return res;
            Trange tr;
            if (trange_from_python(val, tr)) return -1;
            self->station_context = false;
            self->rec->set(tr);
            return 0;
        }

        if (!val)
        {
            // del rec[val]
            self->rec->unset(varname.c_str());
            return 0;
        }

        if (PyFloat_Check(val))
        {
            double v = PyFloat_AsDouble(val);
            if (v == -1.0 && PyErr_Occurred())
                return -1;
            self->rec->set(varname.c_str(), v);
        } else if (PyInt_Check(val)) {
            long v = PyInt_AsLong(val);
            if (v == -1 && PyErr_Occurred())
                return -1;
            self->rec->set(varname.c_str(), (int)v);
        } else if (
                PyUnicode_Check(val)
#if PY_MAJOR_VERSION >= 3
                || PyBytes_Check(val)
#else
                || PyString_Check(val)
#endif
                ) {
            string value;
            if (string_from_python(val, value))
                return -1;
            self->rec->set(varname.c_str(), value.c_str());
        } else if (val == Py_None) {
            self->rec->unset(varname.c_str());
        } else {
            PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
            return -1;
        }
        return 0;
    } DBALLE_CATCH_RETURN_INT
}

static int dpy_Record_contains(dpy_Record* self, PyObject *value)
{
    string varname;
    if (string_from_python(value, varname))
        return -1;

    switch (varname[0])
    {
        case 'd':
            // We don't bother checking the seconds, since they default to 0 if
            // missing
            if (varname == "date")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return -1;
                return !core::Record::downcast(*self->rec).get_datetime().is_missing();
            }
            else if (varname == "datemin")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return -1;
                return !core::Record::downcast(*self->rec).get_datetimerange().min.is_missing();
            }
            else if (varname == "datemax")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return -1;
                return !core::Record::downcast(*self->rec).get_datetimerange().max.is_missing();
            }
            break;
        case 'l':
            if (varname == "level")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return -1;
                return any_key_set(*self->rec, level_keys, 4);
            }
            break;
        case 't':
            if (varname == "trange" || varname == "timerange")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return -1;
                return any_key_set(*self->rec, trange_keys, 3);
            }
            break;
    }

    return self->rec->isset(varname.c_str()) ? 1 : 0;
}

static PyObject* dpy_Record_copy(dpy_Record* self)
{
    dpy_Record* result = PyObject_New(dpy_Record, &dpy_Record_Type);
    if (!result) return NULL;
    try {
        result->rec = self->rec->clone().release();
        result->station_context = self->station_context;
        return (PyObject*)result;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_clear(dpy_Record* self)
{
    try {
        self->rec->clear();
        self->station_context = false;
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_clear_vars(dpy_Record* self)
{
    try {
        self->rec->clear_vars();
        Py_RETURN_NONE;
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_keys(dpy_Record* self)
{
    pyo_unique_ptr result(PyList_New(0));
    if (!result) return nullptr;

    try {
        bool fail = false;
        self->rec->foreach_key([&](const char* key, const wreport::Var&) {
            if (fail) return;

            pyo_unique_ptr item(PyUnicode_FromString(key));
            if (!item) { fail = true; return; }

            if (PyList_Append(result, item)) { fail = true; return; }
        });

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_items(dpy_Record* self)
{
    pyo_unique_ptr result(PyList_New(0));
    if (!result) return nullptr;

    try {
        bool fail = false;
        self->rec->foreach_key([&](const char* key, const wreport::Var& val) {
            if (fail) return;

            pyo_unique_ptr py_key(PyUnicode_FromString(key));
            if (!py_key) { fail = true; return; }
            pyo_unique_ptr py_val(wrpy->var_value_to_python(val));
            if (!py_val) { fail = true; return; }
            pyo_unique_ptr item(PyTuple_Pack(2, py_key.get(), py_val.get()));
            if (!item) { fail = true; return; }

            if (PyList_Append(result, item)) { fail = true; return; }
        });

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_to_dict(dpy_Record* self)
{
    pyo_unique_ptr result(PyDict_New());
    if (!result) return nullptr;

    try {
        bool fail = false;
        self->rec->foreach_key([&](const char* key, const wreport::Var& val) {
            if (fail) return;

            pyo_unique_ptr py_val(wrpy->var_value_to_python(val));
            if (!py_val) { fail = true; return; }

            if (PyDict_SetItemString(result, key, py_val.get()))
            {
                fail = true;
                return;
            }
        });

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_varitems(dpy_Record* self)
{
    pyo_unique_ptr result(PyList_New(0));
    if (!result) return nullptr;

    try {
        bool fail = false;
        self->rec->foreach_key([&](const char* key, const wreport::Var& val) {
            if (fail) return;

            pyo_unique_ptr py_key(PyUnicode_FromString(key));
            if (!py_key) { fail = true; return; }
            pyo_unique_ptr py_val((PyObject*)wrpy->var_create_copy(val));
            if (!py_val) { fail = true; return; }
            pyo_unique_ptr item(PyTuple_Pack(2, py_key.get(), py_val.get()));
            if (!item) { fail = true; return; }

            if (PyList_Append(result, item)) { fail = true; return; }
        });

        return result.release();
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_var(dpy_Record* self, PyObject* args)
{
    const char* name = NULL;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    try {
        return (PyObject*)wrpy->var_create_copy((*self->rec)[name]);
    } DBALLE_CATCH_RETURN_PYO
}

static PyObject* dpy_Record_key(dpy_Record* self, PyObject* args)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Record.var(name) instead of Record.key(name)", 1))
        return nullptr;
    const char* name = NULL;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;

    try {
        return (PyObject*)wrpy->var_create_copy((*self->rec)[name]);
    } DBALLE_CATCH_RETURN_PYO
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

static PyObject* dpy_Record_get(dpy_Record* self, PyObject *args, PyObject* kw)
{
    static const char* kwlist[] = { "key", "default", NULL };
    PyObject* key;
    PyObject* def = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", const_cast<char**>(kwlist), &key, &def))
        return nullptr;

    try {
        int has = dpy_Record_contains(self, key);
        if (has < 0) return NULL;
        if (!has)
        {
            Py_INCREF(def);
            return def;
        }

        return dpy_Record_getitem(self, key);
    } DBALLE_CATCH_RETURN_PYO
}


static PyObject* dpy_Record_vars(dpy_Record* self)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.vars() may disappear in a future version of DB-All.e, and no replacement is planned", 1))
        return nullptr;

    const std::vector<wreport::Var*>& vars = core::Record::downcast(*self->rec).vars();

    pyo_unique_ptr result(PyTuple_New(vars.size()));
    if (!result) return NULL;

    try {
        for (size_t i = 0; i < vars.size(); ++i)
        {
            pyo_unique_ptr v((PyObject*)wrpy->var_create_copy(*vars[i]));
            if (!v) return nullptr;

            if (PyTuple_SetItem(result, i, v.release()))
                return nullptr;
        }
        return result.release();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_date_extremes(dpy_Record* self)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.date_extremes may disappear in a future version of DB-All.e, and no replacement is planned", 1))
        return NULL;

    try {
        DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();

        pyo_unique_ptr dt_min(datetime_to_python(dtr.min));
        pyo_unique_ptr dt_max(datetime_to_python(dtr.max));

        if (!dt_min || !dt_max) return NULL;

        return Py_BuildValue("(NN)", dt_min.release(), dt_max.release());
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_set_station_context(dpy_Record* self)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.set_station_context is deprecated in favour of using DB.query_station_data", 1))
        return NULL;
    try {
        self->rec->set_datetime(Datetime());
        self->rec->set_level(Level());
        self->rec->set_trange(Trange());
        self->station_context = true;
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_set_from_string(dpy_Record* self, PyObject *args)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.set_from_string() may disappear in a future version of DB-All.e, and no replacement is planned", 1))
        return nullptr;

    const char* str = NULL;
    if (!PyArg_ParseTuple(args, "s", &str))
        return NULL;

    try {
        core::Record::downcast(*self->rec).set_from_string(str);
        self->station_context = false;
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyMethodDef dpy_Record_methods[] = {
    {"copy", (PyCFunction)dpy_Record_copy, METH_NOARGS, "return a deep copy of the Record" },
    {"clear", (PyCFunction)dpy_Record_clear, METH_NOARGS, "remove all data from the record" },
    {"clear_vars", (PyCFunction)dpy_Record_clear_vars, METH_NOARGS, "remove all variables from the record, leaving the keywords intact" },
    {"keys", (PyCFunction)dpy_Record_keys, METH_NOARGS, "return a list with all the keys set in the Record." },
    {"items", (PyCFunction)dpy_Record_items, METH_NOARGS, "return a list with all the (key, value) tuples set in the Record." },
    {"to_dict", (PyCFunction)dpy_Record_to_dict, METH_NOARGS, "return a dict with all the key: value assignments set in the Record." },
    {"varitems", (PyCFunction)dpy_Record_varitems, METH_NOARGS, "return a list with all the (key, `dballe.Var`_) tuples set in the Record." },
    {"var", (PyCFunction)dpy_Record_var, METH_VARARGS, "return a `dballe.Var`_ from the record, given its key." },
    {"update", (PyCFunction)dpy_Record_update, METH_VARARGS | METH_KEYWORDS, "set many record keys/vars in a single shot, via kwargs" },
    {"get", (PyCFunction)dpy_Record_get, METH_VARARGS | METH_KEYWORDS, "lookup a value, returning a fallback value (None by default) if unset" },

    // Deprecated
    {"key", (PyCFunction)dpy_Record_var, METH_VARARGS, "(deprecated) return a `dballe.Var`_ from the record, given its key." },
    {"vars", (PyCFunction)dpy_Record_vars, METH_NOARGS, "(deprecated) return a sequence with all the variables set on the Record. Note that this does not include keys." },
    {"date_extremes", (PyCFunction)dpy_Record_date_extremes, METH_NOARGS, "(deprecated) get two datetime objects with the lower and upper bounds of the datetime period in this record" },
    {"set_station_context", (PyCFunction)dpy_Record_set_station_context, METH_NOARGS, "(deprecated) set the date, level and time range values to match the station data context" },
    {"set_from_string", (PyCFunction)dpy_Record_set_from_string, METH_VARARGS, "(deprecated) set values from a 'key=val' string" },

    {NULL}
};

static int dpy_Record_init(dpy_Record* self, PyObject* args, PyObject* kw)
{
    // Construct on preallocated memory
    self->rec = Record::create().release();
    self->station_context = false;

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
    delete self->rec;
    self->rec = 0;
}

static PyObject* dpy_Record_str(dpy_Record* self)
{
    /*
    std::string f = self->var.format("None");
    return PyString_FromString(f.c_str());
    */
    return PyUnicode_FromString("Record");
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
    return PyUnicode_FromString("Record object");
}

static PySequenceMethods dpy_Record_sequence = {
    0,                               // sq_length
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
    pyo_unique_ptr keys(dpy_Record_keys(self));
    if (!keys) return nullptr;
    pyo_unique_ptr iter(PyObject_GetIter(keys));
    if (!iter) return nullptr;
    return iter.release();
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
        case Py_EQ: cmp = *a->rec == *b->rec; break;
        case Py_NE: cmp = *a->rec != *b->rec; break;
        default:
            // https://www.python.org/dev/peps/pep-0207/
            // If the function cannot compare the particular combination of objects, it
            // should return a new reference to Py_NotImplemented.
            result = Py_NotImplemented;
            goto out;
    }
    result = cmp ? Py_True : Py_False;

out:
    Py_INCREF(result);
    return result;
}

PyTypeObject dpy_Record_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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
    R"(
        A record holds a number of key->value mappings, similar to a dict.

        Keys are strings, from a fixed set of keywords defined in DB-All.e and
        documented in the Fortran API documentation, plus all varcodes from the
        DB-All.e BUFR/CREX B table. Using unknown keys raises an exception.

        Values are values internally stored inside `dballe.Var`_ objects, with
        a `dballe.Varinfo`_ corresponding to the key. Record's mapping
        interface directly exposes the variable values, but the `dballe.Var`_
        objects are still accessible using Record.var(key).

        A Record is used to make queries to the database, and read results.

        When creating a new record, keyword arguments can be passed and they
        are set as if Record.update(\*\*kwargs) had been called.

        When setting items, either via __setitem__ and via update(), there are
        special keys that allow to set many items in a single call::

            # Set year, month, day, hour, min, sec
            rec["datetime"] = None
            rec["datetime"] = datetime.datetime(2015, 06, 21, 12, 0, 0)
            # Set yearmin..secmin and yearmax..secmax
            rec["datetime"] = (None, None)
            rec["datetime"] = (datetime.datetime(2014), None)
            rec["datetime"] = (None, datetime.datetime(2015))
            rec["datetime"] = (datetime.datetime(2014), datetime.datetime(2015))
            # Set leveltype1, l1, leveltype2, l2
            rec["level"] = None
            rec["level"] = (103, 2000)
            rec["level"] = (103, 2000, 103, 10000)
            # Set pindicator, p1, p2
            rec["trange"] = None
            rec["trange"] = (205, 0, 10800)

        Examples::

            rec = Record(lat=44.05, lon=11.03, B12101=22.1)

            # Key lookup returns the value of the corresponding variable
            print(rec["lat"], rec["B12101"])

            # Use .var() to get the variable itself; note that it can return
            # None if the value is not set
            print(rec.var("lat").info.desc, rec.var("B12101").info.desc)

            # Iterating a record iterates all keys
            for key in rec:
                print(key, rec.get(key, "undefined"), rec.var(key).info.desc)

            # Use .vars() to get a list of only the varcode keys
            for varcode in rec.vars():
                print(varcode, rec.get(varcode, "undefined"), rec.var(varcode).info.desc)
    )",                        // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    (richcmpfunc)dpy_Record_richcompare, // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)dpy_Record_iter, // tp_iter
    0,                         // tp_iternext
    dpy_Record_methods,        // tp_methods
    0,                         // tp_members
    0,                         // tp_getset
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
    common_init();

    dpy_Record_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Record_Type) < 0)
        return;
    Py_INCREF(&dpy_Record_Type);

    PyModule_AddObject(m, "Record", (PyObject*)&dpy_Record_Type);
}

}
}
