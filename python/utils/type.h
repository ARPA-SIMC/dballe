#ifndef DBALLE_PYTHON_TYPE_H
#define DBALLE_PYTHON_TYPE_H

#include "core.h"
#include <array>

namespace dballe {
namespace python {

template<typename Child, typename IMPL>
struct Getter
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc = "TODO: write getter documentation";
    constexpr static void* closure = nullptr;

    static PyObject* get(Impl* self, void* closure)
    {
        PyErr_Format(PyExc_NotImplementedError, "getter %s is not implemented", Child::name);
        return nullptr;
    }

    static constexpr PyGetSetDef def()
    {
        return PyGetSetDef {const_cast<char*>(Child::name), (getter)Child::get, nullptr, const_cast<char*>(Child::doc), Child::closure};
    }
};


template<typename... GETSETTERS>
struct GetSetters
{
    typedef std::array<PyGetSetDef, sizeof...(GETSETTERS) + 1> Data;
    Data m_data;
    GetSetters() : m_data({GETSETTERS::def()..., {nullptr}})
    {
    }

    PyGetSetDef* as_py() { return const_cast<PyGetSetDef*>(m_data.data()); }
};


/*
 * Base class for implementing python classes
 */

template<typename Child, typename IMPL>
struct Type
{
    typedef IMPL Impl;

#if 0
    // Not supported in Centos 7, use this when we can use a newer C++ compiler
    PySequenceMethods _sequence_methods {
        .sq_length = (lenfunc)Child::sq_length,
        .sq_concat = (binaryfunc)Child::sq_concat,
        .sq_repeat = (ssizeargfunc)Child::sq_repeat,
        .sq_item = (ssizeargfunc)Child::sq_item,
        .sq_ass_item = (ssizeobjargproc)Child::sq_ass_item,
        .sq_contains = (objobjproc)Child::sq_contains,
        .sq_inplace_concat = (binaryfunc)Child::sq_inplace_concat,
        .sq_inplace_repeat = (ssizeargfunc)Child::sq_inplace_repeat,
    };
    PyMappingMethods _mapping_methods {
        .mp_length = (lenfunc)Child::mp_length,
        .mp_subscript = (binaryfunc)ChilChild::mp_subscript,
        .mp_ass_subscript = (objobjargproc)Child::mp_ass_subscript,
    };
#else
    PySequenceMethods _sequence_methods;
    PyMappingMethods _mapping_methods;

    Type()
    {
        _sequence_methods.sq_length = (lenfunc)Child::sq_length;
        _sequence_methods.sq_concat = (binaryfunc)Child::sq_concat;
        _sequence_methods.sq_repeat = (ssizeargfunc)Child::sq_repeat;
        _sequence_methods.sq_item = (ssizeargfunc)Child::sq_item;
        _sequence_methods.sq_ass_item = (ssizeobjargproc)Child::sq_ass_item;
        _sequence_methods.sq_contains = (objobjproc)Child::sq_contains;
        _sequence_methods.sq_inplace_concat = (binaryfunc)Child::sq_inplace_concat;
        _sequence_methods.sq_inplace_repeat = (ssizeargfunc)Child::sq_inplace_repeat;
        _mapping_methods.mp_length = (lenfunc)Child::mp_length;
        _mapping_methods.mp_subscript = (binaryfunc)Child::mp_subscript;
        _mapping_methods.mp_ass_subscript = (objobjargproc)Child::mp_ass_subscript;
    }
#endif

    static PyObject* _str(Impl* self)
    {
        return PyUnicode_FromString(Child::name);
    }

    static PyObject* _repr(Impl* self)
    {
        std::string res = Child::qual_name;
        res += " object";
        return PyUnicode_FromString(res.c_str());
    }

    constexpr static getiterfunc _iter = nullptr;
    constexpr static iternextfunc _iternext = nullptr;
    constexpr static richcmpfunc _richcompare = nullptr;
    constexpr static hashfunc _hash = nullptr;
    constexpr static ternaryfunc _call = nullptr;

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
        PyErr_Format(PyExc_NotImplementedError, "%s objects cannot be constructed explicitly", Child::qual_name);
        return -1;
    }

    /**
     * Define this type in the python interpreter.
     *
     * It fills in \a type_object, and if module is provided, it also registers
     * the constructor in the module.
     */
    void define(PyTypeObject*& type_object, PyObject* module=nullptr)
    {
        Child* d = static_cast<Child*>(this);

        unsigned long tp_flags = Py_TPFLAGS_DEFAULT;

        PySequenceMethods* tp_as_sequence = nullptr;
        if ((void*)Child::sq_length || (void*)Child::sq_concat || (void*)Child::sq_repeat ||
            (void*)Child::sq_item || (void*)Child::sq_ass_item || (void*)Child::sq_contains ||
            (void*)Child::sq_inplace_concat || (void*)Child::sq_inplace_repeat )
        {
            tp_as_sequence = &_sequence_methods;
        }

        PyMappingMethods* tp_as_mapping = nullptr;
        if ((void*)Child::mp_length || (void*)Child::mp_subscript || (void*)Child::mp_ass_subscript)
            tp_as_mapping = &_mapping_methods;

        py_unique_ptr<PyTypeObject> type = new PyTypeObject {
            PyVarObject_HEAD_INIT(NULL, 0)
            Child::qual_name,        // tp_name
            sizeof(Impl), // tp_basicsize
            0,                         // tp_itemsize
            (destructor)Child::_dealloc, // tp_dealloc
            0,                         // tp_print
            0,                         // tp_getattr
            0,                         // tp_setattr
            0,                         // tp_reserved
            (reprfunc)Child::_repr,  // tp_repr
            0,                         // tp_as_number
            tp_as_sequence,            // tp_as_sequence
            tp_as_mapping,             // tp_as_mapping
            (hashfunc)Child::_hash,    // tp_hash
            (ternaryfunc)Child::_call, // tp_call
            (reprfunc)Child::_str,     // tp_str
            0,                         // tp_getattro
            0,                         // tp_setattro
            0,                         // tp_as_buffer
            tp_flags,                  // tp_flags
            Child::doc,              // tp_doc
            0,                         // tp_traverse
            0,                         // tp_clear
            (richcmpfunc)Child::_richcompare, // tp_richcompare
            0,                         // tp_weaklistoffset
            (getiterfunc)Child::_iter, // tp_iter
            (iternextfunc)Child::_iternext,  // tp_iternext
            d->methods.as_py(),        // tp_methods
            0,                         // tp_members
            d->getsetters.as_py(),     // tp_getset
            0,                         // tp_base
            0,                         // tp_dict
            0,                         // tp_descr_get
            0,                         // tp_descr_set
            0,                         // tp_dictoffset
            (initproc)Child::_init,  // tp_init
            0,                         // tp_alloc
            PyType_GenericNew,         // tp_new
        };

        if (PyType_Ready(type) != 0)
            throw PythonException();

        if (module)
        {
            type.incref();
            if (PyModule_AddObject(module, Child::name, (PyObject*)type.get()) != 0)
                throw PythonException();
        }
        type_object = type.release();
    }
};

}
}

#endif
