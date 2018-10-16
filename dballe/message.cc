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
