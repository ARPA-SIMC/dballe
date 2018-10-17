#include "message.h"
#include "dballe/msg/msg.h"
#include <wreport/notes.h>

using namespace std;
using namespace wreport;

namespace dballe {

/*
 * Message
 */

Message::~Message() {}

const wreport::Var* Message::get(const Level& lev, const Trange& tr, wreport::Varcode code) const
{
    return get_full(lev, tr, code);
}

const wreport::Var* Message::get(const char* shortcut) const
{
    return get_shortcut(shortcut);
}

void Message::set(const Level& lev, const Trange& tr, wreport::Varcode code, const wreport::Var& var)
{
    set_copy(lev, tr, code, var);
}

void Message::set(const Level& lev, const Trange& tr, const wreport::Var& var)
{
    set_copy(lev, tr, var.code(), var);
}

void Message::set(const Level& lev, const Trange& tr, std::unique_ptr<wreport::Var> var)
{
    set_move(lev, tr, std::move(var));
}

const char* format_message_type(MessageType type)
{
    switch (type)
    {
        case MessageType::GENERIC: return "generic";
        case MessageType::SYNOP: return "synop";
        case MessageType::PILOT: return "pilot";
        case MessageType::TEMP: return "temp";
        case MessageType::TEMP_SHIP: return "temp_ship";
        case MessageType::AIREP: return "airep";
        case MessageType::AMDAR: return "amdar";
        case MessageType::ACARS: return "acars";
        case MessageType::SHIP: return "ship";
        case MessageType::BUOY: return "buoy";
        case MessageType::METAR: return "metar";
        case MessageType::SAT: return "sat";
        case MessageType::POLLUTION: return "pollution";
    }
    return "(unknown)";
}


std::ostream& operator<<(std::ostream& o, const dballe::MessageType& v)
{
    return o << format_message_type(v);
}

}
