// SPDX-License-Identifier: BSD-3-Clause

#ifndef DBALLE_PYTHON_TYPE_H
#define DBALLE_PYTHON_TYPE_H

#include "core.h"
#include "methods.h"
#include <array>

namespace dballe::python {

template <typename Child, typename IMPL> struct Getter
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc  = "TODO: write getter documentation";
    constexpr static void* closure    = nullptr;

    static PyObject* get(Impl* self, void* closure)
    {
        PyErr_Format(PyExc_NotImplementedError, "%s getter is not implemented",
                     Child::name);
        return nullptr;
    }

    static constexpr PyGetSetDef def()
    {
        return PyGetSetDef{const_cast<char*>(Child::name), (getter)Child::get,
                           nullptr, const_cast<char*>(Child::doc),
                           Child::closure};
    }
};

template <typename Child, typename IMPL> struct GetSetter
{
    typedef IMPL Impl;
    constexpr static const char* name = "TODO";
    constexpr static const char* doc  = "TODO: write getsetter documentation";
    constexpr static void* closure    = nullptr;

    static PyObject* get(Impl* self, void* closure)
    {
        PyErr_Format(PyExc_NotImplementedError, "%s getter is not implemented",
                     Child::name);
        return nullptr;
    }

    static int set(Impl* self, PyObject* value, void* closure)
    {
        PyErr_Format(PyExc_NotImplementedError, "%s setter is not implemented",
                     Child::name);
        return -1;
    }

    static constexpr PyGetSetDef def()
    {
        return PyGetSetDef{const_cast<char*>(Child::name), (getter)Child::get,
                           (setter)Child::set, const_cast<char*>(Child::doc),
                           Child::closure};
    }
};

template <typename... GETSETTERS> struct GetSetters
{
    typedef std::array<PyGetSetDef, sizeof...(GETSETTERS) + 1> Data;
    Data m_data;
    GetSetters() : m_data({GETSETTERS::def()..., PyGetSetDef()}) {}

    PyGetSetDef* as_py() { return const_cast<PyGetSetDef*>(m_data.data()); }
};

template <typename Impl>
struct MethGenericEnter : MethNoargs<MethGenericEnter<Impl>, Impl>
{
    constexpr static const char* name    = "__enter__";
    constexpr static const char* returns = "self";
    constexpr static const char* summary = "Context manager __enter__";
    constexpr static const char* doc     = "Returns the object itself";
    static PyObject* run(Impl* self)
    {
        Py_INCREF(self);
        return (PyObject*)self;
    }
};

template <typename Impl>
struct MethGenericExit : MethKwargs<MethGenericExit<Impl>, Impl>
{
    constexpr static const char* name      = "__exit__";
    constexpr static const char* signature = "ext_type, ext_val, ext_tb";
    constexpr static const char* returns   = "";
    constexpr static const char* summary   = "";

    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = {"exc_type", "exc_val", "exc_tb",
                                       nullptr};
        PyObject* exc_type          = nullptr;
        PyObject* exc_val           = nullptr;
        PyObject* exc_tb            = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOO",
                                         const_cast<char**>(kwlist), &exc_type,
                                         &exc_val, &exc_tb))
            return nullptr;

        return self->python__exit__();
    }
};

/*
 * Base class for implementing python classes
 */

template <typename Child, typename IMPL> struct Type
{
    typedef IMPL Impl;

    Type() {}

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

    constexpr static getiterfunc _iter        = nullptr;
    constexpr static iternextfunc _iternext   = nullptr;
    constexpr static richcmpfunc _richcompare = nullptr;
    constexpr static hashfunc _hash           = nullptr;
    constexpr static ternaryfunc _call        = nullptr;

    constexpr static lenfunc sq_length              = nullptr;
    constexpr static binaryfunc sq_concat           = nullptr;
    constexpr static ssizeargfunc sq_repeat         = nullptr;
    constexpr static ssizeargfunc sq_item           = nullptr;
    constexpr static ssizeobjargproc sq_ass_item    = nullptr;
    constexpr static objobjproc sq_contains         = nullptr;
    constexpr static binaryfunc sq_inplace_concat   = nullptr;
    constexpr static ssizeargfunc sq_inplace_repeat = nullptr;

    constexpr static lenfunc mp_length              = nullptr;
    constexpr static binaryfunc mp_subscript        = nullptr;
    constexpr static objobjargproc mp_ass_subscript = nullptr;

    constexpr static unsigned long tp_flags = Py_TPFLAGS_DEFAULT;

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        // Default implementation raising NotImplementedError
        PyErr_Format(PyExc_NotImplementedError,
                     "%s objects cannot be constructed explicitly",
                     Child::qual_name);
        return -1;
    }

    /**
     * Define this type in the python interpreter.
     *
     * It fills in \a type_object, and if module is provided, it also registers
     * the constructor in the module.
     */
    void define(PyTypeObject*& type_object, PyObject* module = nullptr)
    {
        Child* d = static_cast<Child*>(this);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconditionally-supported"
#pragma GCC diagnostic ignored "-Wcast-qual"
        PyType_Slot slots[] = {
            {Py_tp_dealloc,        (void*)Child::_dealloc         },
            {Py_tp_repr,           (void*)Child::_repr            },
            {Py_tp_hash,           (void*)Child::_hash            },
            {Py_tp_call,           (void*)Child::_call            },
            {Py_tp_str,            (void*)Child::_str             },
            {Py_tp_doc,            (void*)PyDoc_STR(Child::doc)   },
            {Py_tp_richcompare,    (void*)Child::_richcompare     },
            {Py_tp_iter,           (void*)Child::_iter            },
            {Py_tp_iternext,       (void*)Child::_iternext        },
            {Py_tp_methods,        (void*)d->methods.as_py()      },
            {Py_tp_getset,         (void*)d->getsetters.as_py()   },
            {Py_tp_init,           (void*)Child::_init            },
            {Py_tp_new,            (void*)PyType_GenericNew       },
            {Py_sq_length,         (void*)Child::sq_length        },
            {Py_sq_concat,         (void*)Child::sq_concat        },
            {Py_sq_repeat,         (void*)Child::sq_repeat        },
            {Py_sq_item,           (void*)Child::sq_item          },
            {Py_sq_ass_item,       (void*)Child::sq_ass_item      },
            {Py_sq_contains,       (void*)Child::sq_contains      },
            {Py_sq_inplace_concat, (void*)Child::sq_inplace_concat},
            {Py_sq_inplace_repeat, (void*)Child::sq_inplace_repeat},
            {Py_mp_length,         (void*)Child::mp_length        },
            {Py_mp_subscript,      (void*)Child::mp_subscript     },
            {Py_mp_ass_subscript,  (void*)Child::mp_ass_subscript },
            {0,                    NULL                           }
        };
#pragma GCC diagnostic pop

        PyType_Spec spec = {Child::qual_name, sizeof(Impl), 0,
                            (unsigned long)Child::tp_flags, slots};
        pyo_unique_ptr type(throw_ifnull(PyType_FromSpec(&spec)));

        if (module)
        {
            // PyModule_AddObject steals a reference
            type.incref();
            if (PyModule_AddObject(module, Child::name, type) != 0)
                throw PythonException();
        }
        type_object = (PyTypeObject*)type.release();
    } // namespace dballe::python
};

} // namespace dballe::python

#endif
