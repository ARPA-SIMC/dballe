#include "values.h"
#include "core.h"
#include <string>

namespace dballe {
namespace python {

PyObject* cstring_to_python(const char* str)
{
    return throw_ifnull(PyUnicode_FromString(str));
}

PyObject* string_to_python(const std::string& str)
{
    return throw_ifnull(PyUnicode_FromStringAndSize(str.data(), str.size()));
}

std::string string_from_python(PyObject* o)
{
    if (!PyUnicode_Check(o))
    {
        PyErr_SetString(PyExc_TypeError, "value must be an instance of str");
        throw PythonException();
    }
    ssize_t size;
    const char* res = throw_ifnull(PyUnicode_AsUTF8AndSize(o, &size));
    return std::string(res, size);
}

const char* cstring_from_python(PyObject* o)
{
    if (PyUnicode_Check(o))
        return throw_ifnull(PyUnicode_AsUTF8(o));
    PyErr_SetString(PyExc_TypeError, "value must be an instance of str");
    throw PythonException();
}

PyObject* path_to_python(const std::filesystem::path& path)
{
    pyo_unique_ptr pathlib(throw_ifnull(PyImport_ImportModule("pathlib")));
    pyo_unique_ptr Path(throw_ifnull(PyObject_GetAttrString(pathlib, "Path")));
    pyo_unique_ptr arg(to_python(path.native()));
    // From python 3.9:
    // return throw_ifnull(PyObject_CallOneArg(Path, arg));
    return throw_ifnull(PyObject_CallFunctionObjArgs(Path, arg.get(), nullptr));
}

std::filesystem::path path_from_python(PyObject* o)
{
    if (PyUnicode_Check(o))
        return std::filesystem::path(cstring_from_python(o));

    if (!PyObject_HasAttrString(o, "as_posix"))
    {
        PyErr_SetString(PyExc_TypeError,
                        "value must be an instance of str or pathlib.Path");
        throw PythonException();
    }

    pyo_unique_ptr as_posix(
        throw_ifnull(PyObject_GetAttrString(o, "as_posix")));
    // From Python 3.9:
    // pyo_unique_ptr stringval(throw_ifnull(PyObject_CallNoArgs(as_posix)));
    pyo_unique_ptr stringval(
        throw_ifnull(PyObject_CallFunctionObjArgs(as_posix, nullptr)));
    return std::filesystem::path(cstring_from_python(stringval));
}

PyObject* bytes_to_python(const std::vector<uint8_t>& buffer)
{
    return throw_ifnull(
        PyBytes_FromStringAndSize((const char*)buffer.data(), buffer.size()));
}

bool bool_from_python(PyObject* o)
{
    int istrue = PyObject_IsTrue(o);
    if (istrue == -1)
        throw PythonException();
    return istrue == 1;
}

int int_from_python(PyObject* o)
{
    int res = PyLong_AsLong(o);
    if (PyErr_Occurred())
        throw PythonException();
    return res;
}

PyObject* int_to_python(int val) { return throw_ifnull(PyLong_FromLong(val)); }

PyObject* unsigned_int_to_python(unsigned int val)
{
    return throw_ifnull(PyLong_FromUnsignedLong(val));
}

PyObject* long_to_python(long int val)
{
    return throw_ifnull(PyLong_FromLong(val));
}

PyObject* unsigned_long_to_python(unsigned long val)
{
    return throw_ifnull(PyLong_FromUnsignedLong(val));
}

PyObject* long_long_to_python(long long val)
{
    return throw_ifnull(PyLong_FromLongLong(val));
}

PyObject* unsigned_long_long_to_python(unsigned long long val)
{
    return throw_ifnull(PyLong_FromUnsignedLongLong(val));
}

double double_from_python(PyObject* o)
{
    double res = PyFloat_AsDouble(o);
    if (res == -1.0 && PyErr_Occurred())
        throw PythonException();
    return res;
}

PyObject* double_to_python(double val)
{
    return throw_ifnull(PyFloat_FromDouble(val));
}

std::vector<std::string> stringlist_from_python(PyObject* o)
{
    pyo_unique_ptr iter(throw_ifnull(PyObject_GetIter(o)));

    std::vector<std::string> res;
    while (pyo_unique_ptr item = PyIter_Next(iter))
        res.emplace_back(from_python<std::string>(item));

    if (PyErr_Occurred())
        throw PythonException();

    return res;
}

PyObject* stringlist_to_python(const std::vector<std::string>& val)
{
    pyo_unique_ptr res(throw_ifnull(PyList_New(val.size())));
    Py_ssize_t idx = 0;
    for (const auto& str : val)
        PyList_SET_ITEM(res.get(), idx++, to_python(str));
    return res.release();
}

std::vector<std::filesystem::path> pathlist_from_python(PyObject* o)
{
    pyo_unique_ptr iter(throw_ifnull(PyObject_GetIter(o)));

    std::vector<std::filesystem::path> res;
    while (pyo_unique_ptr item = PyIter_Next(iter))
        res.emplace_back(from_python<std::filesystem::path>(item));

    if (PyErr_Occurred())
        throw PythonException();

    return res;
}

PyObject* pathlist_to_python(const std::vector<std::filesystem::path>& val)
{
    pyo_unique_ptr res(throw_ifnull(PyList_New(val.size())));
    Py_ssize_t idx = 0;
    for (const auto& path : val)
        PyList_SET_ITEM(res.get(), idx++, to_python(path));
    return res.release();
}

} // namespace python
} // namespace dballe
