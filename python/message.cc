#define _DBALLE_LIBRARY_CODE
#include <Python.h>
#include <dballe/message.h>
#include <wreport/python.h>
#include "common.h"
#include "message.h"
#include "types.h"
#include "impl-utils.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

namespace {

struct GetType : Getter<dpy_Message>
{
    constexpr static const char* name = "type";
    constexpr static const char* doc = "message type";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return message_type_to_python(self->message->get_type());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetDatetime : Getter<dpy_Message>
{
    constexpr static const char* name = "datetime";
    constexpr static const char* doc = "message datetime";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return datetime_to_python(self->message->get_datetime());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetCoords : Getter<dpy_Message>
{
    constexpr static const char* name = "coords";
    constexpr static const char* doc = "message coordinates";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return coords_to_python(self->message->get_coords());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetIdent : Getter<dpy_Message>
{
    constexpr static const char* name = "ident";
    constexpr static const char* doc = "message mobile station identifier";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            return ident_to_python(self->message->get_ident());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct GetNetwork : Getter<dpy_Message>
{
    constexpr static const char* name = "network";
    constexpr static const char* doc = "message network";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            auto network = self->message->get_network();
            return PyUnicode_FromStringAndSize(network.data(), network.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};


struct get : MethKwargs<dpy_Message>
{
    constexpr static const char* name = "get";
    constexpr static const char* doc = "Get a Var given level, timerange, and varcode; returns None if not found";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "level", "trange", "code", nullptr };
        PyObject* pylevel = nullptr;
        PyObject* pytrange = nullptr;
        PyObject* pycode = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &pylevel, &pytrange, &pycode))
            return nullptr;

        try {
            Level level = level_from_python(pylevel);
            Trange trange = trange_from_python(pytrange);
            Varcode code = varcode_from_python(pycode);

            const Var* res = self->message->get(level, trange, code);
            if (!res)
                Py_RETURN_NONE;
            else
                return (PyObject*)wrpy->var_create_copy(*res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct get_named : MethKwargs<dpy_Message>
{
    constexpr static const char* name = "get_named";
    constexpr static const char* doc = "Get a Var given its shortcut name; returns None if not found";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", nullptr };
        const char* name = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &name))
            return nullptr;

        try {
            const Var* res = self->message->get(name);
            if (!res)
                Py_RETURN_NONE;
            else
                return (PyObject*)wrpy->var_create_copy(*res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct set : MethKwargs<dpy_Message>
{
    constexpr static const char* name = "set";
    constexpr static const char* doc = "Set a Var given level and timerange";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "level", "trange", "var", nullptr };
        PyObject* pylevel = nullptr;
        PyObject* pytrange = nullptr;
        wrpy_Var* var = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOO!", const_cast<char**>(kwlist), &pylevel, &pytrange, wrpy->var_type, &var))
            return nullptr;

        try {
            Level level = level_from_python(pylevel);
            Trange trange = trange_from_python(pytrange);
            self->message->set(level, trange, var->var);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

struct set_named : MethKwargs<dpy_Message>
{
    constexpr static const char* name = "set_named";
    constexpr static const char* doc = "Set a Var given its shortcut name";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", "var", nullptr };
        const char* name = nullptr;
        wrpy_Var* var = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "sO!", const_cast<char**>(kwlist), &name, wrpy->var_type, &var))
            return nullptr;

        try {
            self->message->set(name, var->var);
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

#if 0
    /**
     * Add or replace a value
     *
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     * @param code
     *   The Varcode of the destination value.  If it is different than the
     *   varcode of var, a conversion will be attempted.
     * @param var
     *   The Var with the value to set
     */
    void set(const Level& lev, const Trange& tr, wreport::Varcode code, const wreport::Var& var);

    /**
     * Add or replace a value
     *
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     * @param var
     *   The Var with the value to set
     */
    void set(const Level& lev, const Trange& tr, const wreport::Var& var);

    /**
     * Add or replace a value, taking ownership of the source variable without
     * copying it.
     *
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     * @param var
     *   The Var with the value to set.  This Message will take ownership of memory
     *   management.
     */
    void set(const Level& lev, const Trange& tr, std::unique_ptr<wreport::Var> var);

    /**
     * Iterate the contents of the message
     */
    virtual bool foreach_var(std::function<bool(const Level&, const Trange&, const wreport::Var&)>) const = 0;
#endif

struct MessageDefinition : public Binding<MessageDefinition, dpy_Message>
{
    constexpr static const char* name = "Message";
    constexpr static const char* qual_name = "dballe.Message";
    constexpr static const char* doc = "Decoded message";
    GetSetters<GetType, GetDatetime, GetCoords, GetIdent, GetNetwork> getsetters;
    Methods<get, get_named, set, set_named> methods;

    static void _dealloc(Impl* self)
    {
        self->message.~shared_ptr<Message>();
        Py_TYPE(self)->tp_free(self);
    }

    static int _init(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "type", nullptr };
        PyObject* py_message_type = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &py_message_type))
            return -1;

        try {
            MessageType type;
            if (read_message_type(py_message_type, type) == -1)
                return -1;

            new (&(self->message)) std::shared_ptr<Message>(Message::create(type));
        } DBALLE_CATCH_RETURN_INT

        return 0;
    }
};

MessageDefinition* definition = nullptr;

}

extern "C" {
PyTypeObject* dpy_Message_Type = nullptr;
}

namespace dballe {
namespace python {

int read_message_type(PyObject* from_python, dballe::MessageType& type)
{
    try {
        if (PyUnicode_Check(from_python))
        {
            const char* v = PyUnicode_AsUTF8(from_python);
            if (v == nullptr) return -1;

            if (strcasecmp(v, "generic") == 0)
                type = MessageType::GENERIC;
            else if (strcasecmp(v, "synop") == 0)
                type = MessageType::SYNOP;
            else if (strcasecmp(v, "pilot") == 0)
                type = MessageType::PILOT;
            else if (strcasecmp(v, "temp") == 0)
                type = MessageType::TEMP;
            else if (strcasecmp(v, "temp_ship") == 0)
                type = MessageType::TEMP_SHIP;
            else if (strcasecmp(v, "airep") == 0)
                type = MessageType::AIREP;
            else if (strcasecmp(v, "amdar") == 0)
                type = MessageType::AMDAR;
            else if (strcasecmp(v, "acars") == 0)
                type = MessageType::ACARS;
            else if (strcasecmp(v, "ship") == 0)
                type = MessageType::SHIP;
            else if (strcasecmp(v, "buoy") == 0)
                type = MessageType::BUOY;
            else if (strcasecmp(v, "metar") == 0)
                type = MessageType::METAR;
            else if (strcasecmp(v, "sat") == 0)
                type = MessageType::SAT;
            else if (strcasecmp(v, "pollution") == 0)
                type = MessageType::POLLUTION;
            else
            {
                PyErr_Format(PyExc_ValueError, "%R is not a valid MessageType value", from_python);
                return -1;
            }
            return 0;
        }
    } DBALLE_CATCH_RETURN_INT

    PyErr_SetString(PyExc_TypeError, "Expected str");
    return -1;
}

PyObject* message_type_to_python(MessageType type)
{
    const char* formatted = format_message_type(type);
    return PyUnicode_FromString(formatted);
}

dpy_Message* message_create(MessageType type)
{
    dpy_Message* res = PyObject_New(dpy_Message, dpy_Message_Type);
    if (!res) return nullptr;
    new (&(res->message)) std::shared_ptr<Message>(Message::create(type));
    return res;
}

dpy_Message* message_create(std::shared_ptr<Message> message)
{
    dpy_Message* res = PyObject_New(dpy_Message, dpy_Message_Type);
    if (!res) return nullptr;
    new (&(res->message)) std::shared_ptr<Message>(message);
    return res;
}

void register_message(PyObject* m)
{
    common_init();

    definition = new MessageDefinition;
    dpy_Message_Type = definition->activate(m);
}

}
}
