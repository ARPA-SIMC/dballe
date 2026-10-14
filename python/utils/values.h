#ifndef DBALLE_PYTHON_VALUES_H
#define DBALLE_PYTHON_VALUES_H

#define PY_SSIZE_T_CLEAN
#include "core.h"
#include <Python.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace dballe::python {

/// Base template for all from_python shortcuts
template <typename T> inline T from_python(PyObject*)
{
    throw std::runtime_error("method not implemented");
}

inline PyObject* to_python(PyObject* o) { return o; }

inline PyObject* to_python(const pyo_unique_ptr& o) { return o.get(); }

/// Convert an utf8 string to a python str object
PyObject* cstring_to_python(const char* str);
inline PyObject* to_python(const char* s) { return cstring_to_python(s); }

/// Convert an utf8 string to a python str object
PyObject* string_to_python(const std::string& str);
inline PyObject* to_python(const std::string& s) { return string_to_python(s); }

/// Convert a python string to an utf8 string
std::string string_from_python(PyObject* o);
template <> inline std::string from_python<std::string>(PyObject* o)
{
    return string_from_python(o);
}

/// Convert a python string to an utf8 string
const char* cstring_from_python(PyObject* o);
template <> inline const char* from_python<const char*>(PyObject* o)
{
    return cstring_from_python(o);
}

/// Convert a path to a python pathlib.Path object
PyObject* path_to_python(const std::filesystem::path& path);
inline PyObject* to_python(const std::filesystem::path& path)
{
    return path_to_python(path);
}

/// Convert a python string or Path to a path
std::filesystem::path path_from_python(PyObject* o);
template <>
inline std::filesystem::path from_python<std::filesystem::path>(PyObject* o)
{
    return path_from_python(o);
}

/// Convert a buffer of data to python bytes
PyObject* bytes_to_python(const std::vector<uint8_t>& buffer);
inline PyObject* to_python(const std::vector<uint8_t>& buffer)
{
    return bytes_to_python(buffer);
}

/// Convert a Python object to an int
int int_from_python(PyObject* o);
template <> inline int from_python<int>(PyObject* o)
{
    return int_from_python(o);
}

/// Convert a Python object to an int
bool bool_from_python(PyObject* o);
template <> inline bool from_python<bool>(PyObject* o)
{
    return bool_from_python(o);
}

/// Convert an int to a Python object
PyObject* int_to_python(int val);
inline PyObject* to_python(int val) { return int_to_python(val); }

/// Convert an int to a Python object
PyObject* unsigned_int_to_python(unsigned int val);
inline PyObject* to_python(unsigned int val)
{
    return unsigned_int_to_python(val);
}

/// Convert a long to a Python object
PyObject* long_to_python(long val);
inline PyObject* to_python(long val) { return long_to_python(val); }

/// Convert an unsigned long to a Python object
PyObject* unsigned_long_to_python(unsigned long val);
inline PyObject* to_python(unsigned long val)
{
    return unsigned_long_to_python(val);
}

/// Convert a long to a Python object
PyObject* long_long_to_python(long long val);
inline PyObject* to_python(long long val) { return long_long_to_python(val); }

/// Convert an unsigned long to a Python object
PyObject* unsigned_long_long_to_python(unsigned long long val);
inline PyObject* to_python(unsigned long long val)
{
    return unsigned_long_long_to_python(val);
}

/// Convert a Python object to a double
double double_from_python(PyObject* o);
template <> inline double from_python<double>(PyObject* o)
{
    return double_from_python(o);
}

/// Convert a double to a Python object
PyObject* double_to_python(double val);
inline PyObject* to_python(double val) { return double_to_python(val); }

/// Read a string list from a Python object
std::vector<std::string> stringlist_from_python(PyObject* o);
template <>
inline std::vector<std::string>
from_python<std::vector<std::string>>(PyObject* o)
{
    return stringlist_from_python(o);
}

/// Convert a string list to a Python object
PyObject* stringlist_to_python(const std::vector<std::string>& val);
inline PyObject* to_python(const std::vector<std::string>& val)
{
    return stringlist_to_python(val);
}

/// Read a path list from a Python object
std::vector<std::filesystem::path> pathlist_from_python(PyObject* o);
template <>
inline std::vector<std::filesystem::path>
from_python<std::vector<std::filesystem::path>>(PyObject* o)
{
    return pathlist_from_python(o);
}

/// Convert a path list to a Python object
PyObject* pathlist_to_python(const std::vector<std::filesystem::path>& val);
inline PyObject* to_python(const std::vector<std::filesystem::path>& val)
{
    return pathlist_to_python(val);
}

} // namespace dballe::python

#endif
