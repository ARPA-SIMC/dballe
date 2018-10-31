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
    constexpr static const char* signature = "";
    constexpr static const char* returns = nullptr;
    constexpr static const char* summary = nullptr;
    constexpr static const char* doc = nullptr;
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
    constexpr static const char* signature = "";
    constexpr static const char* returns = nullptr;
    constexpr static const char* summary = nullptr;
    constexpr static const char* doc = nullptr;
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
    constexpr static const char* signature = "…";
    constexpr static const char* returns = nullptr;
    constexpr static const char* summary = nullptr;
    constexpr static const char* doc = nullptr;
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
    constexpr static const char* signature = "…";
    constexpr static const char* returns = nullptr;
    constexpr static const char* summary = nullptr;
    constexpr static const char* doc = nullptr;
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
    constexpr static const char* signature = "…";
    constexpr static const char* returns = nullptr;
    constexpr static const char* summary = nullptr;
    constexpr static const char* doc = nullptr;
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
    constexpr static const char* returns = "self";
    constexpr static const char* summary = "Context manager __enter__";
    constexpr static const char* doc = "Returns the object itself";
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
PyGetSetDef as_py_getset()
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
    char* doc = dballe::python::build_method_doc(Method::name, Method::signature, Method::returns, Method::summary, Method::doc);
    return {(char*)Method::name, (PyCFunction)Method::run, Method::flags, doc};
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

#if 0
    // Not supported in Centos 7, use this when we can use a newer C++ compiler
    PySequenceMethods _sequence_methods {
        .sq_length = (lenfunc)Derived::sq_length,
        .sq_concat = (binaryfunc)Derived::sq_concat,
        .sq_repeat = (ssizeargfunc)Derived::sq_repeat,
        .sq_item = (ssizeargfunc)Derived::sq_item,
        .sq_ass_item = (ssizeobjargproc)Derived::sq_ass_item,
        .sq_contains = (objobjproc)Derived::sq_contains,
        .sq_inplace_concat = (binaryfunc)Derived::sq_inplace_concat,
        .sq_inplace_repeat = (ssizeargfunc)Derived::sq_inplace_repeat,
    };
    PyMappingMethods _mapping_methods {
        .mp_length = (lenfunc)Derived::mp_length,
        .mp_subscript = (binaryfunc)Derived::mp_subscript,
        .mp_ass_subscript = (objobjargproc)Derived::mp_ass_subscript,
    };
#else
    PySequenceMethods _sequence_methods;
    PyMappingMethods _mapping_methods;

    Binding()
    {
        _sequence_methods.sq_length = (lenfunc)Derived::sq_length;
        _sequence_methods.sq_concat = (binaryfunc)Derived::sq_concat;
        _sequence_methods.sq_repeat = (ssizeargfunc)Derived::sq_repeat;
        _sequence_methods.sq_item = (ssizeargfunc)Derived::sq_item;
        _sequence_methods.sq_ass_item = (ssizeobjargproc)Derived::sq_ass_item;
        _sequence_methods.sq_contains = (objobjproc)Derived::sq_contains;
        _sequence_methods.sq_inplace_concat = (binaryfunc)Derived::sq_inplace_concat;
        _sequence_methods.sq_inplace_repeat = (ssizeargfunc)Derived::sq_inplace_repeat;
        _mapping_methods.mp_length = (lenfunc)Derived::mp_length;
        _mapping_methods.mp_subscript = (binaryfunc)Derived::mp_subscript;
        _mapping_methods.mp_ass_subscript = (objobjargproc)Derived::mp_ass_subscript;
    }
#endif

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

    constexpr static lenfunc sq_length = nullptr;
    constexpr static binaryfunc sq_concat = nullptr;
    constexpr static ssizeargfunc sq_repeat = nullptr;
    constexpr static ssizeargfunc sq_item = nullptr;
    constexpr static ssizeobjargproc sq_ass_item = nullptr;
    constexpr static objobjproc sq_contains = nullptr;
    constexpr static binaryfunc sq_inplace_concat = nullptr;
    constexpr static ssizeargfunc sq_inplace_repeat = nullptr;

    constexpr static lenfunc mp_length = nullptr;
    constexpr static binaryfunc mp_subscript = nullptr;
    constexpr static objobjargproc mp_ass_subscript = nullptr;

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

#if PY_MAJOR_VERSION <= 2
        long tp_flags = Py_TPFLAGS_DEFAULT;
#else
        unsigned long tp_flags = Py_TPFLAGS_DEFAULT;
#endif

#if PY_MAJOR_VERSION <= 2
        if ((void*)Derived::_iter)
            tp_flags |= Py_TPFLAGS_HAVE_ITER;
#endif

        PySequenceMethods* tp_as_sequence = nullptr;
        if ((void*)Derived::sq_length || (void*)Derived::sq_concat || (void*)Derived::sq_repeat ||
            (void*)Derived::sq_item || (void*)Derived::sq_ass_item || (void*)Derived::sq_contains ||
            (void*)Derived::sq_inplace_concat || (void*)Derived::sq_inplace_repeat )
        {
            tp_as_sequence = &_sequence_methods;
        }

        PyMappingMethods* tp_as_mapping = nullptr;
        if ((void*)Derived::mp_length || (void*)Derived::mp_subscript || (void*)Derived::mp_ass_subscript)
            tp_as_mapping = &_mapping_methods;

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
            tp_as_sequence,            // tp_as_sequence
            tp_as_mapping,             // tp_as_mapping
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
