#include "message.h"
#include "dballe/msg/msg.h"
#include "dballe/core/shortcuts.h"
#include "dballe/core/var.h"
#include <wreport/notes.h>

using namespace std;
using namespace wreport;

namespace dballe {

/*
 * Message
 */

Message::~Message() {}

std::unique_ptr<Message> Message::create(MessageType type)
{
    impl::Message* msg;
    std::unique_ptr<Message> res(msg = new impl::Message);
    msg->type = type;
    return res;
}

const wreport::Var* Message::get(const Level& lev, const Trange& tr, wreport::Varcode code) const
{
    return get_impl(lev, tr, code);
}

const wreport::Var* Message::get(const char* shortcut) const
{
    const impl::Shortcut& s = impl::Shortcut::by_name(shortcut);
    return get_impl(s.level, s.trange, s.code);
}

const wreport::Var* Message::get(const std::string& shortcut) const
{
    const impl::Shortcut& s = impl::Shortcut::by_name(shortcut);
    return get_impl(s.level, s.trange, s.code);
}

void Message::set(const Level& lev, const Trange& tr, wreport::Varcode code, const wreport::Var& var)
{
    set_impl(lev, tr, var_copy_without_unset_attrs(var, code));
}

void Message::set(const Level& lev, const Trange& tr, const wreport::Var& var)
{
    set_impl(lev, tr, var_copy_without_unset_attrs(var));
}

void Message::set(const Level& lev, const Trange& tr, std::unique_ptr<wreport::Var> var)
{
    set_impl(lev, tr, std::move(var));
}

void Message::set(const char* shortcut, std::unique_ptr<wreport::Var> var)
{
    const impl::Shortcut& s = impl::Shortcut::by_name(shortcut);
    if (s.code != var->code())
        error_consistency::throwf("Message::set(%s) called with varcode of %01d%02d%03d instead of %02d%02d%03d",
                shortcut, WR_VAR_FXY(var->code()), WR_VAR_FXY(s.code));
    return set(s.level, s.trange, std::move(var));
}

void Message::set(const char* shortcut, const wreport::Var& var)
{
    const impl::Shortcut& s = impl::Shortcut::by_name(shortcut);
    return set(s.level, s.trange, s.code, var);
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
