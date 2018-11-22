#include "cursor.h"
#include "types.h"
#include "dballe/db/v7/cursor.h"
#include "dballe/db/v7/cache.h"

namespace dballe {
namespace python {

PyObject* enqpy(db::CursorStation& cur, const char* key, unsigned len)
{
    if (const dballe::db::v7::cursor::Stations* c = dynamic_cast<const dballe::db::v7::cursor::Stations*>(&cur))
    {
        switch (key) { // mklookup
            case "priority":    return dballe_int_to_python(c->get_priority());
            case "rep_memo":    return string_to_python(c->cur->station.report);
            case "report":      return string_to_python(c->cur->station.report);
            case "ana_id":      return dballe_int_to_python(c->cur->station.id);
            case "mobile":      if (c->cur->station.ident.is_missing()) Py_RETURN_FALSE; else Py_RETURN_TRUE;
            case "ident":       return ident_to_python(c->cur->station.ident);
            case "lat":         return dballe_int_lat_to_python(c->cur->station.coords.lat);
            case "lon":         return dballe_int_lon_to_python(c->cur->station.coords.lon);
            case "coords":      return coords_to_python(c->cur->station.coords);
            case "station":     return dbstation_to_python(c->cur->station);
            default:            PyErr_Format(PyExc_KeyError, "key %s not found", key); throw PythonException();
        }
    } else {
        PyErr_SetString(PyExc_NotImplementedError, "enqpy called on a db::Cursor which is not a db::v7 Cursor");
        throw PythonException();
    }
}

PyObject* enqpy(db::CursorStationData& cur, const char* key, unsigned len)
{
    if (const dballe::db::v7::cursor::StationData* c = dynamic_cast<const dballe::db::v7::cursor::StationData*>(&cur))
    {
        switch (key) { // mklookup
            case "priority":    return dballe_int_to_python(c->get_priority());
            case "rep_memo":    return string_to_python(c->cur->station.report);
            case "report":      return string_to_python(c->cur->station.report);
            case "ana_id":      return dballe_int_to_python(c->cur->station.id);
            case "mobile":      if (c->cur->station.ident.is_missing()) Py_RETURN_FALSE; else Py_RETURN_TRUE;
            case "ident":       return ident_to_python(c->cur->station.ident);
            case "lat":         return dballe_int_lat_to_python(c->cur->station.coords.lat);
            case "lon":         return dballe_int_lon_to_python(c->cur->station.coords.lon);
            case "coords":      return coords_to_python(c->cur->station.coords);
            case "station":     return dbstation_to_python(c->cur->station);
            case "var":         return (PyObject*)throw_ifnull(wrpy->var_create_copy(*c->cur->value));
            case "context_id":  return dballe_int_to_python(c->cur->value.data_id);
            case "attrs":       return attrs_to_python(*c->cur->value);
            default:            PyErr_Format(PyExc_KeyError, "key %s not found", key); throw PythonException();
        }
    } else {
        PyErr_SetString(PyExc_NotImplementedError, "enqpy called on a db::Cursor which is not a db::v7 Cursor");
        throw PythonException();
    }
}

PyObject* enqpy(db::CursorData& cur, const char* key, unsigned len)
{
    if (const dballe::db::v7::cursor::Data* c = dynamic_cast<const dballe::db::v7::cursor::Data*>(&cur))
    {
        switch (key) { // mklookup
            case "priority":    return dballe_int_to_python(c->get_priority());
            case "rep_memo":    return string_to_python(c->cur->station.report);
            case "report":      return string_to_python(c->cur->station.report);
            case "ana_id":      return dballe_int_to_python(c->cur->station.id);
            case "mobile":      if (c->cur->station.ident.is_missing()) Py_RETURN_FALSE; else Py_RETURN_TRUE;
            case "ident":       return ident_to_python(c->cur->station.ident);
            case "lat":         return dballe_int_lat_to_python(c->cur->station.coords.lat);
            case "lon":         return dballe_int_lon_to_python(c->cur->station.coords.lon);
            case "year":        return throw_ifnull(PyLong_FromLong(c->cur->datetime.year));
            case "month":       return throw_ifnull(PyLong_FromLong(c->cur->datetime.month));
            case "day":         return throw_ifnull(PyLong_FromLong(c->cur->datetime.day));
            case "hour":        return throw_ifnull(PyLong_FromLong(c->cur->datetime.hour));
            case "min":         return throw_ifnull(PyLong_FromLong(c->cur->datetime.minute));
            case "sec":         return throw_ifnull(PyLong_FromLong(c->cur->datetime.second));
            case "leveltype1":  return dballe_int_to_python(c->get_levtr().level.ltype1);
            case "l1":          return dballe_int_to_python(c->get_levtr().level.l1);
            case "leveltype2":  return dballe_int_to_python(c->get_levtr().level.ltype2);
            case "l2":          return dballe_int_to_python(c->get_levtr().level.l2);
            case "pindicator":  return dballe_int_to_python(c->get_levtr().trange.pind);
            case "p1":          return dballe_int_to_python(c->get_levtr().trange.p1);
            case "p2":          return dballe_int_to_python(c->get_levtr().trange.p2);
            case "coords":      return coords_to_python(c->cur->station.coords);
            case "station":     return dbstation_to_python(c->cur->station);
            case "datetime":    return datetime_to_python(c->cur->datetime);
            case "level":       return level_to_python(c->get_levtr().level);
            case "trange":      return trange_to_python(c->get_levtr().trange);
            case "var":         return (PyObject*)throw_ifnull(wrpy->var_create_copy(*c->cur->value));
            case "attrs":       return attrs_to_python(*c->cur->value);
            case "context_id":  return dballe_int_to_python(c->cur->value.data_id);
            default:            PyErr_Format(PyExc_KeyError, "key %s not found", key); throw PythonException();
        }
    } else {
        PyErr_SetString(PyExc_NotImplementedError, "enqpy called on a db::Cursor which is not a db::v7 Cursor");
        throw PythonException();
    }
}

PyObject* enqpy(db::CursorSummary& cur, const char* key, unsigned len)
{
    if (const dballe::db::v7::cursor::Summary* c = dynamic_cast<const dballe::db::v7::cursor::Summary*>(&cur))
    {
        switch (key) { // mklookup
            case "priority":    return dballe_int_to_python(c->get_priority());
            case "rep_memo":    return string_to_python(c->cur->station.report);
            case "report":      return string_to_python(c->cur->station.report);
            case "ana_id":      return dballe_int_to_python(c->cur->station.id);
            case "mobile":      if (c->cur->station.ident.is_missing()) Py_RETURN_FALSE; else Py_RETURN_TRUE;
            case "ident":       return ident_to_python(c->cur->station.ident);
            case "lat":         return dballe_int_lat_to_python(c->cur->station.coords.lat);
            case "lon":         return dballe_int_lon_to_python(c->cur->station.coords.lon);
            case "yearmax":     if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.max.year));
            case "yearmin":     if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.min.year));
            case "monthmax":    if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.max.month));
            case "monthmin":    if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.min.month));
            case "daymax":      if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.max.day));
            case "daymin":      if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.min.day));
            case "hourmax":     if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.max.hour));
            case "hourmin":     if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.min.hour));
            case "minumax":     if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.max.minute));
            case "minumin":     if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.min.minute));
            case "secmax":      if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.max.second));
            case "secmin":      if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(c->cur->dtrange.min.second));
            case "leveltype1":  return dballe_int_to_python(c->get_levtr().level.ltype1);
            case "l1":          return dballe_int_to_python(c->get_levtr().level.l1);
            case "leveltype2":  return dballe_int_to_python(c->get_levtr().level.ltype2);
            case "l2":          return dballe_int_to_python(c->get_levtr().level.l2);
            case "pindicator":  return dballe_int_to_python(c->get_levtr().trange.pind);
            case "p1":          return dballe_int_to_python(c->get_levtr().trange.p1);
            case "p2":          return dballe_int_to_python(c->get_levtr().trange.p2);
            case "var":         return varcode_to_python(c->cur->code);
            case "count":       return dballe_int_to_python(c->cur->count);
            case "coords":      return coords_to_python(c->cur->station.coords);
            case "station":     return dbstation_to_python(c->cur->station);
            case "datetimemin": if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return datetime_to_python(c->cur->dtrange.min);
            case "datetimemax": if (c->cur->dtrange.is_missing()) Py_RETURN_NONE; else return datetime_to_python(c->cur->dtrange.max);
            case "level":       return level_to_python(c->get_levtr().level);
            case "trange":      return trange_to_python(c->get_levtr().trange);
            default:            PyErr_Format(PyExc_KeyError, "key %s not found", key); throw PythonException();
        }
    } else {
        PyErr_SetString(PyExc_NotImplementedError, "enqpy called on a db::Cursor which is not a db::v7 Cursor");
        throw PythonException();
    }
}

namespace {
int get_station_id(const Station& station) { return MISSING_INT; }
int get_station_id(const DBStation& station) { return station.id; }
}

template<typename Station>
PyObject* enqpy(db::summary::Cursor<Station>& cur, const char* key, unsigned len)
{
    switch (key) { // mklookup
        case "priority":    Py_RETURN_NONE;
        case "rep_memo":    return string_to_python(cur.cur->station_entry.station.report);
        case "report":      return string_to_python(cur.cur->station_entry.station.report);
        case "ana_id":      return dballe_int_to_python(get_station_id(cur.cur->station_entry.station));
        case "mobile":      if (cur.cur->station_entry.station.ident.is_missing()) Py_RETURN_FALSE; else Py_RETURN_TRUE;
        case "ident":       return ident_to_python(cur.cur->station_entry.station.ident);
        case "lat":         return dballe_int_lat_to_python(cur.cur->station_entry.station.coords.lat);
        case "lon":         return dballe_int_lon_to_python(cur.cur->station_entry.station.coords.lon);
        case "yearmax":     if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.max.year));
        case "yearmin":     if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.min.year));
        case "monthmax":    if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.max.month));
        case "monthmin":    if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.min.month));
        case "daymax":      if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.max.day));
        case "daymin":      if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.min.day));
        case "hourmax":     if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.max.hour));
        case "hourmin":     if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.min.hour));
        case "minumax":     if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.max.minute));
        case "minumin":     if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.min.minute));
        case "secmax":      if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.max.second));
        case "secmin":      if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return throw_ifnull(PyLong_FromLong(cur.cur->var_entry.dtrange.min.second));
        case "leveltype1":  return dballe_int_to_python(cur.cur->var_entry.var.level.ltype1);
        case "l1":          return dballe_int_to_python(cur.cur->var_entry.var.level.l1);
        case "leveltype2":  return dballe_int_to_python(cur.cur->var_entry.var.level.ltype2);
        case "l2":          return dballe_int_to_python(cur.cur->var_entry.var.level.l2);
        case "pindicator":  return dballe_int_to_python(cur.cur->var_entry.var.trange.pind);
        case "p1":          return dballe_int_to_python(cur.cur->var_entry.var.trange.p1);
        case "p2":          return dballe_int_to_python(cur.cur->var_entry.var.trange.p2);
        case "var":         return varcode_to_python(cur.cur->var_entry.var.varcode);
        case "count":       return dballe_int_to_python(cur.cur->var_entry.count);
        case "coords":      return coords_to_python(cur.cur->station_entry.station.coords);
        case "station":     return to_python(cur.cur->station_entry.station);
        case "datetimemin": if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return datetime_to_python(cur.cur->var_entry.dtrange.min);
        case "datetimemax": if (cur.cur->var_entry.dtrange.is_missing()) Py_RETURN_NONE; else return datetime_to_python(cur.cur->var_entry.dtrange.max);
        case "level":       return level_to_python(cur.cur->var_entry.var.level);
        case "trange":      return trange_to_python(cur.cur->var_entry.var.trange);
        default:            PyErr_Format(PyExc_KeyError, "key %s not found", key); throw PythonException();
    }
}

template PyObject* enqpy(db::summary::Cursor<Station>& cur, const char* key, unsigned len);
template PyObject* enqpy(db::summary::Cursor<DBStation>& cur, const char* key, unsigned len);

}
}
