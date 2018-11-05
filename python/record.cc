#include <Python.h>
#include <dballe/core/record.h>
#include <dballe/core/defs.h>
#include <dballe/core/query.h>
#include "record.h"
#include "common.h"
#include "types.h"
#include <vector>
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_Record_Type = nullptr;
}

namespace {

/**
 * Set key=val in rec.
 *
 * Returns 0 on success, -1 on failures with a python exception set
 */
static void setpy(dballe::Record& rec, PyObject* key, PyObject* val)
{
    string name = string_from_python(key);

    // Check for shortcut keys
    if (name == "datetime" || name == "date")
    {
        if (name == "date")
            if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] instead of rec[\"date\"]", 1))
                throw PythonException();

        if (val && PySequence_Check(val))
            rec.set(datetimerange_from_python(val));
        else
            rec.set(datetime_from_python(val));
        return;
    }

    if (name == "datemin") {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] = (min, max) instead of rec[\"datemin\"]", 1))
            throw PythonException();
        DatetimeRange dtr = core::Record::downcast(rec).get_datetimerange();
        dtr.min = datetime_from_python(val);
        rec.set(dtr);
        return;
    }

    if (name == "datemax") {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"datetime\"] = (min, max) instead of rec[\"datemax\"]", 1))
            throw PythonException();
        DatetimeRange dtr = core::Record::downcast(rec).get_datetimerange();
        dtr.max = datetime_from_python(val);
        rec.set(dtr);
        return;
    }

    if (name == "level")
    {
        rec.set(level_from_python(val));
        return;
    }

    if (name == "trange" || name == "timerange")
    {
        if (name == "timerange")
            if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use rec[\"trange\"] instead of rec[\"timerange\"]", 1))
                throw PythonException();
        rec.set(trange_from_python(val));
        return;
    }

    if (!val)
    {
        // del rec[val]
        rec.unset(name.c_str());
        return;
    }

    if (PyFloat_Check(val))
    {
        double v = PyFloat_AsDouble(val);
        if (v == -1.0 && PyErr_Occurred())
            throw PythonException();
        rec.set(name.c_str(), v);
#if PY_MAJOR_VERSION <= 2
    } else if (PyInt_Check(val)) {
        long v = PyInt_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            throw PythonException();
        rec.set(name.c_str(), (int)v);
#else
    } else if (PyLong_Check(val)) {
        long v = PyLong_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            throw PythonException();
        rec.set(name.c_str(), (int)v);
#endif
    } else if (
            PyUnicode_Check(val)
#if PY_MAJOR_VERSION >= 3
            || PyBytes_Check(val)
#else
            || PyString_Check(val)
#endif
            ) {
        string value = string_from_python(val);
        rec.set(name.c_str(), value.c_str());
    } else if (val == Py_None) {
        rec.unset(name.c_str());
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
        throw PythonException();
    }
}

static PyObject* __getitem__(dpy_Record* self, PyObject* key)
{
    try {
        string varname = string_from_python(key);

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

        core::dba_keyword rec_key = core::Record::keyword_byname_len(varname.c_str(), varname.size());
        if (rec_key != core::DBA_KEY_ERROR)
        {
            const Var* var = core::Record::downcast(*self->rec).get(varname.c_str());
            if (!var)
                Py_RETURN_NONE;

            if (!var->isset())
                Py_RETURN_NONE;

            return wrpy->var_value_to_python(*var);
        }

        wreport::Varcode code = resolve_varcode(varname);
        const Var* var = self->rec->get_var(code);
        if (!var)
            Py_RETURN_NONE;

        if (!var->isset())
            Py_RETURN_NONE;

        return wrpy->var_value_to_python(*var);
    } DBALLE_CATCH_RETURN_PYO
}

static const char* level_keys[4] = { "leveltype1", "l1", "leveltype2", "l2" };
static const char* trange_keys[3] = { "pindicator", "p1", "p2" };
static int any_key_set(const Record& rec, const char** keys, unsigned len)
{
    for (unsigned i = 0; i < len; ++i)
        if (rec.isset(keys[i]))
            return 1;
    return 0;
}


static int __in__(dpy_Record* self, PyObject *value)
{
    try {
        string varname = string_from_python(value);

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
    } DBALLE_CATCH_RETURN_INT
}


struct copy : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "copy";
    constexpr static const char* returns = "Record";
    constexpr static const char* summary = "return a deep copy of the Record";
    static PyObject* run(Impl* self)
    {
        try {
            py_unique_ptr<dpy_Record> result(throw_ifnull(PyObject_New(dpy_Record, dpy_Record_Type)));
            result->rec = self->rec->clone().release();
            return (PyObject*)result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct clear : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "clear";
    constexpr static const char* summary = "remove all data from the record";
    static PyObject* run(Impl* self)
    {
        try {
            self->rec->clear();
            Py_RETURN_NONE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct clear_vars : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "clear_vars";
    constexpr static const char* summary = "remove all variables from the record, leaving the keywords intact";
    static PyObject* run(Impl* self)
    {
        try {
            self->rec->clear_vars();
            Py_RETURN_NONE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct to_dict : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "to_dict";
    constexpr static const char* returns = "dict";
    constexpr static const char* summary = "return a dict with all the key: value assignments set in the Record.";
    static PyObject* run(Impl* self)
    {
        try {
            pyo_unique_ptr result(throw_ifnull(PyDict_New()));
            core::Record::downcast(*self->rec).foreach_key([&](const char* key, const wreport::Var& val) {
                pyo_unique_ptr py_val(throw_ifnull(wrpy->var_value_to_python(val)));
                if (PyDict_SetItemString(result, key, py_val.get()))
                    throw PythonException();
            });
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct keys : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "keys";
    constexpr static const char* returns = "List[str]";
    constexpr static const char* summary = "return a list with all the keys set in the Record.";
    static PyObject* run(Impl* self)
    {
        try {
            pyo_unique_ptr result(throw_ifnull(PyList_New(0)));

            core::Record::downcast(*self->rec).foreach_key([&](const char* key, const wreport::Var&) {
                pyo_unique_ptr item(throw_ifnull(PyUnicode_FromString(key)));
                if (PyList_Append(result, item) != 0)
                    throw PythonException();
            });

            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct items : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "items";
    constexpr static const char* returns = "List[(str, Any)]";
    constexpr static const char* summary = "return a list with all the (key, value) tuples set in the Record.";
    static PyObject* run(Impl* self)
    {
        try {
            pyo_unique_ptr result(throw_ifnull(PyList_New(0)));

            core::Record::downcast(*self->rec).foreach_key([&](const char* key, const wreport::Var& val) {
                pyo_unique_ptr py_key(throw_ifnull(PyUnicode_FromString(key)));
                pyo_unique_ptr py_val(throw_ifnull(wrpy->var_value_to_python(val)));
                pyo_unique_ptr item(throw_ifnull(PyTuple_Pack(2, py_key.get(), py_val.get())));
                if (PyList_Append(result, item))
                    throw PythonException();
            });

            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct varitems : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "varitems";
    constexpr static const char* returns = "List[(str, dballe.Var)]";
    constexpr static const char* summary = "return a list with all the (key, `dballe.Var`_) tuples set in the Record.";
    static PyObject* run(Impl* self)
    {
        try {
            pyo_unique_ptr result(throw_ifnull(PyList_New(0)));
            core::Record::downcast(*self->rec).foreach_key([&](const char* key, const wreport::Var& val) {
                pyo_unique_ptr py_key(throw_ifnull(PyUnicode_FromString(key)));
                pyo_unique_ptr py_val(throw_ifnull((PyObject*)wrpy->var_create_copy(val)));
                pyo_unique_ptr item(throw_ifnull(PyTuple_Pack(2, py_key.get(), py_val.get())));
                if (PyList_Append(result, item))
                    throw PythonException();
            });
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct var : MethKwargs<dpy_Record>
{
    constexpr static const char* name = "var";
    constexpr static const char* signature = "str";
    constexpr static const char* returns = "dballe.Var";
    constexpr static const char* summary = "return a `dballe.Var`_ from the record, given its key.";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", nullptr };
        const char* name = NULL;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &name))
            return nullptr;
        try {
            const wreport::Var* var = core::Record::downcast(*self->rec).get(name);
            if (!var)
            {
                PyErr_SetString(PyExc_KeyError, name);
                return nullptr;
            }
            return throw_ifnull((PyObject*)wrpy->var_create_copy(*var));
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct update : MethKwargs<dpy_Record>
{
    constexpr static const char* name = "update";
    constexpr static const char* signature = "key: str: value: Any, …";
    constexpr static const char* summary = "set many record keys/vars in a single shot, via kwargs";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        try {
            if (kw)
            {
                PyObject *key, *value;
                Py_ssize_t pos = 0;

                while (PyDict_Next(kw, &pos, &key, &value))
                    setpy(*self->rec, key, value);
            }
            Py_RETURN_NONE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct get : MethKwargs<dpy_Record>
{
    constexpr static const char* name = "get";
    constexpr static const char* signature = "key: str, default: Any=None";
    constexpr static const char* returns = "Any";
    constexpr static const char* summary = "lookup a value, returning a fallback value (None by default) if unset";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "key", "default", NULL };
        PyObject* key;
        PyObject* def = Py_None;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", const_cast<char**>(kwlist), &key, &def))
            return nullptr;

        try {
            int has = __in__(self, key);
            if (has < 0) return NULL;
            if (!has)
            {
                Py_INCREF(def);
                return def;
            }

            return __getitem__(self, key);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct attrs : MethKwargs<dpy_Record>
{
    constexpr static const char* name = "attrs";
    constexpr static const char* signature = "code: str";
    constexpr static const char* returns = "Dict[str, dballe.Var]";
    constexpr static const char* summary = "return a dict with the attributes of the given varcode";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "code", NULL };
        const char* code = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &code))
            return nullptr;

        try {
            wreport::Varcode varcode = resolve_varcode(code);
            const wreport::Var* var = self->rec->get_var(varcode);
            if (!var)
            {
                PyErr_SetString(PyExc_KeyError, name);
                return nullptr;
            }
            pyo_unique_ptr result(throw_ifnull(PyDict_New()));
            for (const Var* a = var->next_attr(); a != nullptr; a = a->next_attr())
            {
                string key = wreport::varcode_format(a->code());
                pyo_unique_ptr val(throw_ifnull((PyObject*)wrpy->var_create_copy(*a)));
                if (PyDict_SetItemString(result, key.c_str(), val.get()))
                    return nullptr;
            }
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct key : MethKwargs<dpy_Record>
{
    constexpr static const char* name = "key";
    constexpr static const char* signature = "name: str";
    constexpr static const char* returns = "dballe.Var";
    constexpr static const char* summary = "return a `dballe.Var`_ from the record, given its key. (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use Record.var instead of Record.key", 1))
            return nullptr;

        static const char* kwlist[] = { "name", nullptr };
        const char* name = NULL;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &name))
            return nullptr;
        try {
            const wreport::Var* var = core::Record::downcast(*self->rec).get(name);
            if (!var)
            {
                PyErr_SetString(PyExc_KeyError, name);
                return nullptr;
            }
            return throw_ifnull((PyObject*)wrpy->var_create_copy(*var));
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct vars : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "vars";
    constexpr static const char* returns = "Sequence[dballe.Var]";
    constexpr static const char* doc = "return a sequence with all the variables set on the Record. Note that this does not include keys (deprecated)";
    static PyObject* run(Impl* self)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.vars() may disappear in a future version of DB-All.e, and no replacement is planned", 1))
            return nullptr;

        try {
            const std::vector<wreport::Var*>& vars = core::Record::downcast(*self->rec).vars();
            pyo_unique_ptr result(throw_ifnull(PyTuple_New(vars.size())));
            for (size_t i = 0; i < vars.size(); ++i)
            {
                pyo_unique_ptr v(throw_ifnull((PyObject*)wrpy->var_create_copy(*vars[i])));
                if (PyTuple_SetItem(result, i, v.release()))
                    return nullptr;
            }
            return result.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct date_extremes : MethNoargs<dpy_Record>
{
    constexpr static const char* name = "date_extremes";
    constexpr static const char* returns = "Tuple[datetime.datetime, datetime.datetimer]";
    constexpr static const char* summary = "get two datetime objects with the lower and upper bounds of the datetime period in this record (deprecated)";
    static PyObject* run(Impl* self)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.date_extremes may disappear in a future version of DB-All.e, and no replacement is planned", 1))
            return nullptr;

        try {
            DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();
            pyo_unique_ptr dt_min(datetime_to_python(dtr.min));
            pyo_unique_ptr dt_max(datetime_to_python(dtr.max));
            return Py_BuildValue("(NN)", dt_min.release(), dt_max.release());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct set_from_string : MethKwargs<dpy_Record>
{
    constexpr static const char* name = "set_from_string";
    constexpr static const char* signature = "str";
    constexpr static const char* summary = "set values from a 'key=val' string (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.set_from_string() may disappear in a future version of DB-All.e, and no replacement is planned", 1))
            return nullptr;

        const char* str = nullptr;
        if (!PyArg_ParseTuple(args, "s", &str))
            return nullptr;

        try {
            core::Record::downcast(*self->rec).set_from_string(str);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


struct Definition : public Binding<Definition, dpy_Record>
{
    constexpr static const char* name = "Record";
    constexpr static const char* qual_name = "dballe.Record";
    constexpr static const char* doc = R"(
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
        are set as if Record.update(\*\*kwargs) had been called. Also, any
        function that accepts a Record as input, can also accept a Python dict
        with the Record keys.

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
    )";

    GetSetters<> getsetters;
    Methods<
        copy, clear, clear_vars,
        keys, items, varitems, to_dict,
        get, var, attrs,
        update,
        // Deprecated
        key, vars, date_extremes, set_from_string
    > methods;

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        // Construct on preallocated memory
        try {
            self->rec = Record::create().release();

            if (kw)
            {
                PyObject *key, *value;
                Py_ssize_t pos = 0;

                while (PyDict_Next(kw, &pos, &key, &value))
                    setpy(*self->rec, key, value);
            }

            return 0;
        } DBALLE_CATCH_RETURN_INT
    }

    static void _dealloc(Impl* self)
    {
        delete self->rec;
        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* _richcompare(Impl* a, Impl* b, int op)
    {
        // Make sure both arguments are Records.
        if (!(dpy_Record_Check(a) && dpy_Record_Check(b)))
#if PY_MAJOR_VERSION >= 3
            Py_RETURN_NOTIMPLEMENTED;
#else
        {
            Py_INCREF(Py_NotImplemented);
            return Py_NotImplemented;
        }
#endif

        switch (op) {
            case Py_EQ: if (*a->rec == *b->rec) Py_RETURN_TRUE; else Py_RETURN_FALSE;
            case Py_NE: if (*a->rec != *b->rec) Py_RETURN_TRUE; else Py_RETURN_FALSE;
#if PY_MAJOR_VERSION >= 3
            default: Py_RETURN_NOTIMPLEMENTED;
#else
            default:
                Py_INCREF(Py_NotImplemented);
                return Py_NotImplemented;
#endif
        }
    }

    static PyObject* _iter(dpy_Record* self)
    {
        try {
            pyo_unique_ptr keys(keys::run(self));
            if (!keys) return nullptr;
            pyo_unique_ptr iter(PyObject_GetIter(keys));
            if (!iter) return nullptr;
            return iter.release();
        } DBALLE_CATCH_RETURN_PYO
    }

#if 0
    static PyObject* _iternext(Impl* self)
    {
        try {
            ensure_valid_cursor(self);
            if (self->cur->next())
            {
                self->cur->to_record(*self->rec->rec);
                Py_INCREF(self->rec);
                return (PyObject*)self->rec;
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        } DBALLE_CATCH_RETURN_PYO
    }
#endif

    static int sq_contains(dpy_Record* self, PyObject *value)
    {
        return __in__(self, value);
    }

    static PyObject* mp_subscript(dpy_Record* self, PyObject* key)
    {
        return __getitem__(self, key);
    }

    static int mp_ass_subscript(dpy_Record* self, PyObject *key, PyObject *val)
    {
        try {
            setpy(*self->rec, key, val);
            return 0;
        } DBALLE_CATCH_RETURN_INT
    }

};

Definition* definition = nullptr;

}

namespace dballe {
namespace python {

RecordAccess::RecordAccess(PyObject* o)
{
    if (!o || o == Py_None)
    {
        temp = new dballe::core::Record;
        result = temp;
        return;
    }

    if (dpy_Record_Check(o))
    {
        result = ((dpy_Record*)o)->rec;
        return;
    }

    if (PyDict_Check(o))
    {
        temp = new dballe::core::Record;
        result = temp;
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(o, &pos, &key, &value))
            setpy(*temp, key, value);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dballe.Record or dict or None");
    throw PythonException();
}

RecordAccess::~RecordAccess()
{
    delete temp;
}

void read_query(PyObject* from_python, dballe::Query& query)
{
    auto& q = core::Query::downcast(query);

    if (!from_python || from_python == Py_None)
    {
        q.clear();
        return;
    }

    if (dpy_Record_Check(from_python))
    {
        query.set_from_record(*((dpy_Record*)from_python)->rec);
        return;
    }

    if (PyDict_Check(from_python))
    {
        dballe::core::Record rec;
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(from_python, &pos, &key, &value))
            setpy(rec, key, value);
        query.set_from_record(rec);
        return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected dballe.Record or dict or None");
    throw PythonException();
}

dpy_Record* record_create()
{
    return (dpy_Record*)throw_ifnull(PyObject_CallObject((PyObject*)dpy_Record_Type, NULL));
}

void register_record(PyObject* m)
{
    common_init();

    definition = new Definition;
    dpy_Record_Type = definition->activate(m);
}

}
}
