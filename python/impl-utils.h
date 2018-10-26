#ifndef DBALLE_PYTHON_UTILS_H
#define DBALLE_PYTHON_UTILS_H

#include <Python.h>
#include <array>

namespace {

template<typename IMPL>
struct impl_traits
{
};

template<typename IMPL>
struct Getter
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write getter documentation";
    constexpr static void* closure = nullptr;

    static PyObject* get(Impl* self, void* closure)
    {
        PyErr_Format(PyExc_NotImplementedError, "getter %s is not implemented", name);
        return nullptr;
    }
};

template<typename IMPL>
struct MethNoargs
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write method documentation";
    constexpr static int flags = METH_NOARGS;

    static PyObject* run(Impl* self)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", name);
        return nullptr;
    }
};

template<typename IMPL>
struct MethVarargs
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write method documentation";
    constexpr static int flags = METH_VARARGS;

    static PyObject* run(Impl* self, PyObject* args)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", name);
        return nullptr;
    }
};


/*
 * Automatically define a null-terminated array of PyGetSetDef
 */

template<typename Getter>
constexpr PyGetSetDef as_py_getset()
{
    return {(char*)Getter::name, (getter)Getter::get, nullptr, (char*)Getter::doc, Getter::closure};
}

template<typename... GETSETTERS>
struct GetSetters
{
    typedef std::array<PyGetSetDef, sizeof...(GETSETTERS) + 1> Data;
    Data m_data;
    GetSetters() : m_data({ as_py_getset<GETSETTERS>()..., {nullptr} })
    {
    }

    PyGetSetDef* as_py() { return const_cast<PyGetSetDef*>(m_data.data()); }
};

template<>
struct GetSetters<>
{
    PyGetSetDef* as_py() { return 0; }
};


/*
 * Automatically define a null-terminated array of PyMethodDef
 */

template<typename Method>
constexpr PyMethodDef as_py_method()
{
    return {(char*)Method::name, (PyCFunction)Method::run, Method::flags, (char*)Method::doc};
}

template<typename... METHODS>
struct Methods
{
    typedef std::array<PyMethodDef, sizeof...(METHODS) + 1> Data;
    Data m_data;
    Methods() : m_data({ as_py_method<METHODS>()..., {nullptr} })
    {
    }

    PyMethodDef* as_py() { return const_cast<PyMethodDef*>(m_data.data()); }
};

template<>
struct Methods<>
{
    PyMethodDef* as_py() { return 0; }
};

}

#endif
