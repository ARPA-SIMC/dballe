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

/// Convert a Coords to a python (lat, lon) tuple
PyObject* coords_to_python(const Coords& coords);

/// Convert a Coords to a python string or None
PyObject* ident_to_python(const Ident& ident);

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

#if PY_MAJOR_VERSION >= 3
/// Convert a Python object to a Varcode
int varcode_from_python(PyObject* o, wreport::Varcode& code);
#endif

inline PyObject* to_python(const wreport::Varcode& s) { return varcode_to_python(s); }
inline PyObject* to_python(const Coords& s) { return coords_to_python(s); }
inline PyObject* to_python(const Ident& s) { return ident_to_python(s); }
inline PyObject* to_python(const Level& s) { return level_to_python(s); }
inline PyObject* to_python(const Trange& s) { return trange_to_python(s); }
inline PyObject* to_python(const Station& s) { return station_to_python(s); }
inline PyObject* to_python(const DBStation& s) { return dbstation_to_python(s); }

int register_types(PyObject* m);

}
}

#endif
