#ifndef DBALLE_PYTHON_TYPES_H
#define DBALLE_PYTHON_TYPES_H

#include <Python.h>

extern "C" {

PyAPI_DATA(PyTypeObject) dpy_Level_Type;

#define dpy_Level_Check(ob) \
    (ob == Py_None || PyTuple_Check(ob) || \
     Py_TYPE(ob) == &dpy_Level_Type || PyType_IsSubtype(Py_TYPE(ob), &dpy_Level_Type))

PyAPI_DATA(PyTypeObject) dpy_Trange_Type;

#define dpy_Trange_Check(ob) \
    (ob == Py_None || PyTuple_Check(ob) || \
     Py_TYPE(ob) == &dpy_Trange_Type || PyType_IsSubtype(Py_TYPE(ob), &dpy_Trange_Type))

}

namespace dballe {
struct Datetime;
struct DatetimeRange;
struct Level;
struct Trange;

namespace python {

/// Convert a Level to a python 4-tuple
PyObject* level_to_python(const Level& lev);

/// Convert a 4-tuple to a Level
int level_from_python(PyObject* o, Level& out);

/// Convert a Trange to a python 3-tuple
PyObject* trange_to_python(const Trange& tr);

/// Convert a 3-tuple to a Trange
int trange_from_python(PyObject* o, Trange& out);

void register_types(PyObject* m);

}
}

#endif
