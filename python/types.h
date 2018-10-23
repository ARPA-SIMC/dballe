#ifndef DBALLE_PYTHON_TYPES_H
#define DBALLE_PYTHON_TYPES_H

#include <Python.h>
#include <wreport/varinfo.h>
#include <dballe/fwd.h>
#include <dballe/core/fwd.h>

extern "C" {

PyAPI_DATA(PyTypeObject) dpy_Level_Type;

#define dpy_Level_Check(ob) \
    (ob == Py_None || PyTuple_Check(ob) || \
     Py_TYPE(ob) == &dpy_Level_Type || PyType_IsSubtype(Py_TYPE(ob), &dpy_Level_Type))

PyAPI_DATA(PyTypeObject) dpy_Trange_Type;

#define dpy_Trange_Check(ob) \
    (ob == Py_None || PyTuple_Check(ob) || \
     Py_TYPE(ob) == &dpy_Trange_Type || PyType_IsSubtype(Py_TYPE(ob), &dpy_Trange_Type))

PyAPI_DATA(PyTypeObject) dpy_Station_Type;

#define dpy_DBStation_Check(ob) \
     (Py_TYPE(ob) == &dpy_DBStation_Type || PyType_IsSubtype(Py_TYPE(ob), &dpy_DBStation_Type))

PyAPI_DATA(PyTypeObject) dpy_DBStation_Type;

#define dpy_Station_Check(ob) \
     (Py_TYPE(ob) == &dpy_Station_Type || PyType_IsSubtype(Py_TYPE(ob), &dpy_Station_Type))

}

namespace dballe {
namespace python {

/// Convert a Level to a python Level structseq
PyObject* level_to_python(const Level& lev);

/// Convert a None, structseq or 4-tuple to a Level
int level_from_python(PyObject* o, Level& out);

/// Convert a Trange to a python Trange structseq
PyObject* trange_to_python(const Trange& tr);

/// Convert a None, structseq or 3-tuple to a Trange
int trange_from_python(PyObject* o, Trange& out);

/// Convert a Station to a python Station structseq
PyObject* station_to_python(const Station& s);

/// Convert a structseq to a Station
int station_from_python(PyObject* o, Station& out);

/// Convert a Station to a python Station structseq
PyObject* dbstation_to_python(const DBStation& s);

/// Convert a structseq to a Station
int dbstation_from_python(PyObject* o, DBStation& out);

/// Convert a varcode to a Python string
PyObject* varcode_to_python(wreport::Varcode code);


//template<typename T>
//PyObject* to_python(const T&);

inline PyObject* to_python(const Station& s) { return station_to_python(s); }
inline PyObject* to_python(const DBStation& s) { return dbstation_to_python(s); }


void register_types(PyObject* m);

}
}

#endif
