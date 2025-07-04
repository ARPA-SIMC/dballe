#ifndef DBALLE_PYTHON_MESSAGE_H
#define DBALLE_PYTHON_MESSAGE_H

#include "utils/core.h"
#include <dballe/message.h>
#include <memory>

extern "C" {

struct dbapy_c_api;

typedef struct
{
    PyObject_HEAD std::shared_ptr<dballe::Message> message;
} dpy_Message;

extern PyTypeObject* dpy_Message_Type;

#define dpy_Message_Check(ob)                                                  \
    (Py_TYPE(ob) == dpy_Message_Type ||                                        \
     PyType_IsSubtype(Py_TYPE(ob), dpy_Message_Type))
}

namespace dballe {
namespace python {

/**
 * Parse a python object into a message type.
 *
 * Currently, only strings are accepted
 */
int read_message_type(PyObject* from_python, dballe::MessageType& type);

/// Convert an MessageType to a python object
PyObject* message_type_to_python(MessageType type);

/**
 * Create a dpy_Message with a new message of the given type
 */
dpy_Message* message_create(MessageType type);

/**
 * Create a dpy_Message referencing the given message
 */
dpy_Message* message_create(std::shared_ptr<dballe::Message> message);

void register_message(PyObject* m, dbapy_c_api& c_api);

} // namespace python
} // namespace dballe

#endif
