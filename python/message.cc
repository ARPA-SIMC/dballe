#define _DBALLE_LIBRARY_CODE
#include <dballe/message.h>
#include <dballe/msg/msg.h>
#include <wreport/python.h>
#include "common.h"
#include "message.h"
#include "types.h"
#include "cursor.h"
#include "utils/type.h"
#include "utils/methods.h"
#include "utils/values.h"
#include "utils/wreport.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_Message_Type = nullptr;
}

namespace {

struct GetType : Getter<GetType, dpy_Message>
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

struct GetDatetime : Getter<GetDatetime, dpy_Message>
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

struct GetCoords : Getter<GetCoords, dpy_Message>
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

struct GetIdent : Getter<GetIdent, dpy_Message>
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

struct GetReport : Getter<GetReport, dpy_Message>
{
    constexpr static const char* name = "report";
    constexpr static const char* doc = "message report";
    static PyObject* get(Impl* self, void* closure)
    {
        try {
            auto report = self->message->get_report();
            return PyUnicode_FromStringAndSize(report.data(), report.size());
        } DBALLE_CATCH_RETURN_PYO
    }
};


struct get : MethKwargs<get, dpy_Message>
{
    constexpr static const char* name = "get";
    constexpr static const char* signature = "level: dballe.Level, trange: dballe.Trange, code: str";
    constexpr static const char* returns = "Union[dballe.Var, None]";
    constexpr static const char* summary = "Get a Var given its level, timerange, and varcode; returns None if not found";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "level", "trange", "code", nullptr };
        PyObject* pylevel = nullptr;
        PyObject* pytrange = nullptr;
        PyObject* pycode = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOO", const_cast<char**>(kwlist), &pylevel, &pytrange, &pycode))
            return nullptr;

        try {
            Level level = level_from_python(pylevel);
            Trange trange = trange_from_python(pytrange);
            Varcode code = varcode_from_python(pycode);

            const Var* res = self->message->get(level, trange, code);
            if (!res)
                Py_RETURN_NONE;
            else
                return wreport_api.var_create(*res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct get_named : MethKwargs<get_named, dpy_Message>
{
    constexpr static const char* name = "get_named";
    constexpr static const char* signature = "name: str";
    constexpr static const char* returns = "Union[dballe.Var, None]";
    constexpr static const char* summary = "Get a Var given its shortcut name; returns None if not found";
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
                return wreport_api.var_create(*res);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct set : MethKwargs<set, dpy_Message>
{
    constexpr static const char* name = "set";
    constexpr static const char* signature = "level: dballe.Level, trange: dballe.Trange, var: dballe.Var";
    constexpr static const char* summary = "Set a Var given level and timerange";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "level", "trange", "var", nullptr };
        PyObject* pylevel = nullptr;
        PyObject* pytrange = nullptr;
        PyObject* var = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "OOO!", const_cast<char**>(kwlist), &pylevel, &pytrange, wreport_api.api().var_type, &var))
            return nullptr;

        try {
            Level level = level_from_python(pylevel);
            Trange trange = trange_from_python(pytrange);
            self->message->set(level, trange, wreport_api.var(var));
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

struct set_named : MethKwargs<set_named, dpy_Message>
{
    constexpr static const char* name = "set_named";
    constexpr static const char* signature = "name: str, var: dballe.Var";
    constexpr static const char* summary = "Set a Var given its shortcut name";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", "var", nullptr };
        const char* name = nullptr;
        PyObject* var = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "sO!", const_cast<char**>(kwlist), &name, wreport_api.api().var_type, &var))
            return nullptr;

        try {
            self->message->set(name, wreport_api.var(var));
        } DBALLE_CATCH_RETURN_PYO

        Py_RETURN_NONE;
    }
};

template<typename Base>
struct MethQuery : public MethKwargs<Base, dpy_Message>
{
    typedef typename MethKwargs<Base, dpy_Message>::Impl Impl;
    constexpr static const char* signature = "query: Dict[str, Any]";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", NULL };
        PyObject* pyquery = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", const_cast<char**>(kwlist), &pyquery))
            return nullptr;

        try {
            auto query = query_from_python(pyquery);
            return Base::run_query(self, *query);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct query_stations : MethQuery<query_stations>
{
    constexpr static const char* name = "query_stations";
    constexpr static const char* returns = "dballe.CursorStation";
    constexpr static const char* summary = "Query the station data in the message";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->message->query_stations(query);
        gil.lock();
        return (PyObject*)cursor_create(impl::CursorStation::downcast(std::move(res)));
    }
};

struct query_station_data : MethQuery<query_station_data>
{
    constexpr static const char* name = "query_station_data";
    constexpr static const char* returns = "dballe.CursorStationData";
    constexpr static const char* summary = "Query the station variables in the message";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->message->query_station_data(query);
        gil.lock();
        return (PyObject*)cursor_create(impl::CursorStationData::downcast(std::move(res)));
    }
};

struct query_data : MethQuery<query_data>
{
    constexpr static const char* name = "query_data";
    constexpr static const char* returns = "dballe.CursorData";
    constexpr static const char* summary = "Query the variables in the message";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = self->message->query_data(query);
        gil.lock();
        return (PyObject*)cursor_create(impl::CursorData::downcast(std::move(res)));
    }
};

#if 0
struct query_station_and_data : MethQuery<query_station_and_data>
{
    constexpr static const char* name = "query_station_and_data";
    constexpr static const char* returns = "dballe.CursorData";
    constexpr static const char* summary = "Query the station and data variables in the message";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        auto res = impl::Message::downcast(self->message)->query_station_and_data(query);
        gil.lock();
        return (PyObject*)cursor_create(impl::CursorData::downcast(std::move(res)));
    }
};
#endif


struct Definition : public Type<Definition, dpy_Message>
{
    constexpr static const char* name = "Message";
    constexpr static const char* qual_name = "dballe.Message";
    constexpr static const char* doc = R"(
The contents of a decoded BUFR or CREX message.

DB-All.e can interpret the contents of most weather messages commonly in use,
and represent them as variables identified by dballe.Level_, dballe.Trange_,
datetime, coordinates, network, and mobile station identifier.

A message contains only one reference station (coordinates, network, mobile
station identifier), only one reference datetime, and many (level, trange,
varcode, value) variables.

Variables that describe the station are accessible using None for level and
trange.

Constructor: Message(type: str)

`type` is a string identifying the message type, and it will affect how the
message will be encoded by the exporter.

Available values are:
 * generic
 * synop
 * pilot
 * temp
 * temp_ship;
 * airep
 * amdar
 * acars
 * ship
 * buoy
 * metar
 * sat

Example usage::

    importer = dballe.Importer("BUFR")
    with importer.from_file("test.bufr") as f:
        for msg in f:
            print("{m.report},{m.coords},{m.ident},{m.datetime},{m.type}".format(m=msg))
)";
    GetSetters<GetType, GetDatetime, GetCoords, GetIdent, GetReport> getsetters;
    Methods<get, get_named, set, set_named, query_stations, query_station_data, query_data/*, query_station_and_data*/> methods;

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

Definition* definition = nullptr;

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
    dpy_Message* res = throw_ifnull(PyObject_New(dpy_Message, dpy_Message_Type));
    new (&(res->message)) std::shared_ptr<Message>(Message::create(type));
    return res;
}

dpy_Message* message_create(std::shared_ptr<Message> message)
{
    dpy_Message* res = throw_ifnull(PyObject_New(dpy_Message, dpy_Message_Type));
    new (&(res->message)) std::shared_ptr<Message>(message);
    return res;
}

void register_message(PyObject* m)
{
    common_init();

    definition = new Definition;
    definition->define(dpy_Message_Type, m);
}

}
}
