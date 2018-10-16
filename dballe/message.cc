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

std::ostream& operator<<(std::ostream& o, const dballe::MessageType& v)
{
    return o << msg_type_name(v);
}

}
