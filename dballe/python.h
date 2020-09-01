#ifndef DBALLE_PYTHON_H
#define DBALLE_PYTHON_H

#include <dballe/fwd.h>
#include <memory>

#ifndef PyObject_HEAD
// Forward-declare PyObjetc and PyTypeObject
// see https://mail.python.org/pipermail/python-dev/2003-August/037601.html
extern "C" {
struct _object;
typedef _object PyObject;
struct _typeobject;
typedef _typeobject PyTypeObject;
}
#endif

extern "C" {

/**
 * C++ functions exported by the wreport python bindings, to be used by other
 * C++ bindings.
 *
 * To use them, retrieve a pointer to the struct via the Capsule system:
 * \code
 * dbapy_c_api* dbapy = (dbapy_c_api*)PyCapsule_Import("_dballe._C_API", 0);
 * \endcode
 * 
 */
struct dbapy_c_api {

// API version 1.x

    /// C API major version (updated on incompatible changes)
    unsigned version_major;

    /// C API minor version (updated on backwards-compatible changes)
    unsigned version_minor;

    /// dballe.Message type
    PyTypeObject* message_type;

    /// Create a dballe.Message with a new empty message of the given type
    PyObject* (*message_create_new)(dballe::MessageType);

    /// Create a dballe.Message referencing the given message
    PyObject* (*message_create)(std::shared_ptr<dballe::Message>);
};

}

#endif
