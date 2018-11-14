#ifndef DBALLE_PYTHON_TYPES_H
#define DBALLE_PYTHON_TYPES_H

#include <Python.h>
#include <wreport/varinfo.h>
#include <dballe/types.h>
#include <dballe/core/values.h>
#include "common.h"
#include <set>

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::Level val;
} dpy_Level;

extern PyTypeObject* dpy_Level_Type;

#define dpy_Level_Check(ob) \
    (ob == Py_None || PyTuple_Check(ob) || \
     Py_TYPE(ob) == dpy_Level_Type || PyType_IsSubtype(Py_TYPE(ob), dpy_Level_Type))


typedef struct {
    PyObject_HEAD
    dballe::Trange val;
} dpy_Trange;

extern PyTypeObject* dpy_Trange_Type;

#define dpy_Trange_Check(ob) \
    (ob == Py_None || PyTuple_Check(ob) || \
     Py_TYPE(ob) == dpy_Trange_Type || PyType_IsSubtype(Py_TYPE(ob), dpy_Trange_Type))


typedef struct {
    PyObject_HEAD
    dballe::Station val;
} dpy_Station;

extern PyTypeObject* dpy_Station_Type;

#define dpy_Station_Check(ob) \
     (Py_TYPE(ob) == dpy_Station_Type || PyType_IsSubtype(Py_TYPE(ob), dpy_Station_Type))


typedef struct {
    PyObject_HEAD
    dballe::DBStation val;
} dpy_DBStation;

extern PyTypeObject* dpy_DBStation_Type;

#define dpy_DBStation_Check(ob) \
     (Py_TYPE(ob) == dpy_DBStation_Type || PyType_IsSubtype(Py_TYPE(ob), dpy_DBStation_Type))

}

namespace dballe {
namespace python {

/// Convert a Datetime to a python datetime object
PyObject* datetime_to_python(const Datetime& dt);
inline PyObject* to_python(const Datetime& dt) { return datetime_to_python(dt); }

/// Convert a python datetime object to a Datetime
Datetime datetime_from_python(PyObject* dt);
template<> inline Datetime from_python<Datetime>(PyObject* o) { return datetime_from_python(o); }

/// Convert a sequence of two python datetime objects to a DatetimeRange
DatetimeRange datetimerange_from_python(PyObject* dt);
template<> inline DatetimeRange from_python<DatetimeRange>(PyObject* o) { return datetimerange_from_python(o); }

// inline PyObject* to_python(const DatetimeRange& dtr) { return datetimerange_to_python(dtr); }

/// Convert a Coords to a python (lat, lon) tuple
PyObject* coords_to_python(const Coords& coords);
inline PyObject* to_python(const Coords& s) { return coords_to_python(s); }

/// Convert two python object (one for lat, one for lon) to a Coords
Coords coords_from_python(PyObject* lat, PyObject* lon);

/// Convert a Ident to a python string or None
PyObject* ident_to_python(const Ident& ident);
inline PyObject* to_python(const Ident& s) { return ident_to_python(s); }

/// Convert a python object to an Ident
Ident ident_from_python(PyObject* o);
template<> inline Ident from_python<Ident>(PyObject* o) { return ident_from_python(o); }

/// Convert a Level to a python Level structseq
PyObject* level_to_python(const Level& lev);
inline PyObject* to_python(const Level& s) { return level_to_python(s); }

/// Convert a None, structseq or 4-tuple to a Level
Level level_from_python(PyObject* o);
template<> inline Level from_python<Level>(PyObject* o) { return level_from_python(o); }

/// Convert a Trange to a python Trange structseq
PyObject* trange_to_python(const Trange& tr);
inline PyObject* to_python(const Trange& s) { return trange_to_python(s); }

/// Convert a None, structseq or 3-tuple to a Trange
Trange trange_from_python(PyObject* o);
template<> inline Trange from_python<Trange>(PyObject* o) { return trange_from_python(o); }

/// Convert a Station to a python Station structseq
PyObject* station_to_python(const Station& s);
inline PyObject* to_python(const Station& s) { return station_to_python(s); }

/// Convert a structseq to a Station
Station station_from_python(PyObject* o);
template<> inline Station from_python<Station>(PyObject* o) { return station_from_python(o); }

/// Convert a Station to a python Station structseq
PyObject* dbstation_to_python(const DBStation& s);
inline PyObject* to_python(const DBStation& s) { return dbstation_to_python(s); }

/// Convert a structseq to a Station
DBStation dbstation_from_python(PyObject* o);
template<> inline DBStation from_python<DBStation>(PyObject* o) { return dbstation_from_python(o); }

/// Convert a varcode to a Python string
PyObject* varcode_to_python(wreport::Varcode code);
inline PyObject* to_python(const wreport::Varcode& s) { return varcode_to_python(s); }

#if PY_MAJOR_VERSION >= 3
/// Convert a Python object to a Varcode
wreport::Varcode varcode_from_python(PyObject* o);
template<> inline wreport::Varcode from_python<wreport::Varcode>(PyObject* o) { return varcode_from_python(o); }
#endif

std::string dballe_nullable_string_from_python(PyObject* o);

int dballe_int_lat_from_python(PyObject* o);
int dballe_int_lon_from_python(PyObject* o);
void set_lat_from_python(PyObject* o, Coords& coords);
void set_lon_from_python(PyObject* o, Coords& coords);
unsigned short datetime_int16_from_python(PyObject* o);
unsigned char datetime_int8_from_python(PyObject* o);

template<typename Values>
void set_values_from_python(Values& values, wreport::Varcode code, PyObject* val);
extern template void set_values_from_python(core::Values& values, wreport::Varcode code, PyObject* val);
extern template void set_values_from_python(core::DBValues& values, wreport::Varcode code, PyObject* val);

std::set<wreport::Varcode> varcodes_from_python(PyObject* o);

void register_types(PyObject* m);

}
}

#endif
