#ifndef DBALLE_PYTHON_METHODS_H
#define DBALLE_PYTHON_METHODS_H

#include "core.h"
#include <array>

namespace dballe {
namespace python {

/**
 * Build a function docstring from its components.
 *
 * Returns a newly allocated string.
 */
std::string build_method_doc(const char* name, const char* signature, const char* returns, const char* summary, const char* doc);


/// Common infrastructure for all methods
template<typename Child>
struct BaseMethod
{
    constexpr static const char* name = "TODO";
    constexpr static const char* signature = "";
    constexpr static const char* returns = nullptr;
    constexpr static const char* summary = nullptr;
    constexpr static const char* doc = nullptr;

    static constexpr PyMethodDef def()
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
        return PyMethodDef {const_cast<char*>(Child::name), (PyCFunction)Child::run, Child::flags, Child::doc};
#pragma GCC diagnostic pop
    }

    static std::string docstring()
    {
        return build_method_doc(Child::name, Child::signature, Child::returns, Child::summary, Child::doc);
    }
};

/// Base implementation for a METH_NOARGS method
template<typename Child, typename IMPL>
struct MethNoargs : public BaseMethod<Child>
{
    typedef IMPL Impl;
    constexpr static int flags = METH_NOARGS;

    static PyObject* run(Impl* self)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", Child::name);
        return nullptr;
    }
};

/// Base implementation for a METH_NOARGS class method
template<typename Child>
struct ClassMethNoargs : public BaseMethod<Child>
{
    constexpr static int flags = METH_NOARGS | METH_CLASS;

    static PyObject* run(PyTypeObject* cls)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", Child::name);
        return nullptr;
    }
};

/// Base implementation for a METH_VARARGS method
template<typename Child, typename IMPL>
struct MethVarargs : public BaseMethod<Child>
{
    typedef IMPL Impl;
    constexpr static int flags = METH_VARARGS;

    static PyObject* run(Impl* self, PyObject* args)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", Child::name);
        return nullptr;
    }
};

/// Base implementation for a METH_KWARGS method
template<typename Child, typename IMPL>
struct MethKwargs : public BaseMethod<Child>
{
    typedef IMPL Impl;
    constexpr static int flags = METH_VARARGS | METH_KEYWORDS;

    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", Child::name);
        return nullptr;
    }
};

/// Base implementation for a METH_KWARGS class method
template<typename Child>
struct ClassMethKwargs : public BaseMethod<Child>
{
    constexpr static int flags = METH_VARARGS | METH_KEYWORDS | METH_CLASS;

    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", Child::name);
        return nullptr;
    }
};


template<typename... METHODS>
struct Methods
{
    typedef std::array<std::string, sizeof...(METHODS)> Docstrings;
    typedef std::array<PyMethodDef, sizeof...(METHODS) + 1> Defs;
    Docstrings docstrings;
    Defs m_defs;
    Methods() :
        docstrings({METHODS::docstring()...}),
        m_defs({METHODS::def()..., PyMethodDef()})
    {
        for (unsigned i = 0; i < m_defs.size() - 1; ++i)
            m_defs[i].ml_doc = docstrings[i].c_str();
    }

    PyMethodDef* as_py()
    {
        return const_cast<PyMethodDef*>(m_defs.data());
    }
};

template<>
struct Methods<>
{
    PyMethodDef* as_py() { return 0; }
};

}
}

#endif
