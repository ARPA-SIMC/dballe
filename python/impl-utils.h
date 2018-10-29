#ifndef DBALLE_PYTHON_UTILS_H
#define DBALLE_PYTHON_UTILS_H

#include <Python.h>
#include "common.h"
#include <array>

namespace dballe {
namespace python {

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

struct ClassMethNoargs
{
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write method documentation";
    constexpr static int flags = METH_NOARGS | METH_CLASS;

    static PyObject* run(PyTypeObject* cls)
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

template<typename IMPL>
struct MethKwargs
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write method documentation";
    constexpr static int flags = METH_VARARGS | METH_KEYWORDS;

    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", name);
        return nullptr;
    }
};

struct ClassMethKwargs
{
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write method documentation";
    constexpr static int flags = METH_VARARGS | METH_KEYWORDS | METH_CLASS;

    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        PyErr_Format(PyExc_NotImplementedError, "method %s is not implemented", name);
        return nullptr;
    }
};

template<typename Impl>
struct MethGenericEnter : MethNoargs<Impl>
{
    constexpr static const char* name = "__enter__";
    constexpr static const char* doc = "Context manager __enter__";
    static PyObject* run(Impl* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
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


/*
 * Base helper class for python bindings
 */

template<typename Derived, typename IMPL>
struct Binding
{
    typedef IMPL Impl;

    static PyObject* _str(Impl* self)
    {
        return PyUnicode_FromString(Derived::name);
    }

    static PyObject* _repr(Impl* self)
    {
        std::string res = Derived::qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    constexpr static getiterfunc _iter = nullptr;
    constexpr static iternextfunc _iternext = nullptr;
    constexpr static richcmpfunc _richcompare = nullptr;
    constexpr static hashfunc _hash = nullptr;

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        // Default implementation raising NotImplementedError
        PyErr_Format(PyExc_NotImplementedError, "%s objects cannot be constructed explicitly", Derived::qual_name);
        return -1;
    }

    /**
     * Activate this type.
     *
     * It fills in \a type, and if module is provided, it also registers the
     * constructor in the module.
     */
    PyTypeObject* activate(PyObject* module=nullptr)
    {
        Derived* d = static_cast<Derived*>(this);

        unsigned long tp_flags = Py_TPFLAGS_DEFAULT;
#if PY_MAJOR_VERSION <= 2
        if (Derived::_iter)
            tp_flags |= Py_TPFLAGS_HAVE_ITER;
#endif

        py_unique_ptr<PyTypeObject> type = new PyTypeObject {
            PyVarObject_HEAD_INIT(NULL, 0)
            Derived::qual_name,        // tp_name
            sizeof(Impl), // tp_basicsize
            0,                         // tp_itemsize
            (destructor)Derived::_dealloc, // tp_dealloc
            0,                         // tp_print
            0,                         // tp_getattr
            0,                         // tp_setattr
            0,                         // tp_reserved
            (reprfunc)Derived::_repr,  // tp_repr
            0,                         // tp_as_number
            0,                         // tp_as_sequence
            0,                         // tp_as_mapping
            (hashfunc)Derived::_hash,  // tp_hash
            0,                         // tp_call
            (reprfunc)Derived::_str,   // tp_str
            0,                         // tp_getattro
            0,                         // tp_setattro
            0,                         // tp_as_buffer
            tp_flags,                  // tp_flags
            Derived::doc,              // tp_doc
            0,                         // tp_traverse
            0,                         // tp_clear
            (richcmpfunc)Derived::_richcompare, // tp_richcompare
            0,                         // tp_weaklistoffset
            (getiterfunc)Derived::_iter, // tp_iter
            (iternextfunc)Derived::_iternext,  // tp_iternext
            d->methods.as_py(),        // tp_methods
            0,                         // tp_members
            d->getsetters.as_py(),     // tp_getset
            0,                         // tp_base
            0,                         // tp_dict
            0,                         // tp_descr_get
            0,                         // tp_descr_set
            0,                         // tp_dictoffset
            (initproc)Derived::_init,  // tp_init
            0,                         // tp_alloc
            PyType_GenericNew,         // tp_new
        };

        if (PyType_Ready(type) != 0)
            throw PythonException();

        if (module)
        {
            type.incref();
            if (PyModule_AddObject(module, Derived::name, (PyObject*)type.get()) != 0)
                throw PythonException();
        }
        return type.release();
    }
};

}
}

}

#endif
