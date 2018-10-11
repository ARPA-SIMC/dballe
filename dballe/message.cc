#include "message.h"
#include <wreport/notes.h>

using namespace std;
using namespace wreport;

namespace dballe {

/*
 * Message
 */

Message::~Message() {}

#if 0

/*
 * Messages
 */

Messages::Messages()
{
}

Messages::Messages(const Messages& o)
{
    msgs.reserve(o.msgs.size());
    for (const auto& i: o.msgs)
        msgs.push_back(i->clone().release());
}

Messages::Messages(Messages&& o)
    : msgs(move(o.msgs))
{
}

Messages::~Messages()
{
    for (auto& i: msgs)
        delete i;
}

Messages& Messages::operator=(const Messages& o)
{
    if (this == &o) return *this;
    clear();
    msgs.reserve(o.msgs.size());
    for (const auto& i: o.msgs)
        msgs.push_back(i->clone().release());
    return *this;
}

Messages& Messages::operator=(Messages&& o)
{
    if (this == &o) return *this;
    clear();
    msgs = move(o.msgs);
    return *this;
}

bool Messages::empty() const
{
    return msgs.empty();
}

size_t Messages::size() const
{
    return msgs.size();
}

void Messages::clear()
{
    for (auto& i: msgs)
        delete i;
    msgs.clear();
}

void Messages::append(const Message& msg)
{
    msgs.push_back(msg.clone().release());
}

void Messages::append(unique_ptr<Message>&& msg)
{
    msgs.push_back(msg.release());
}

void Messages::print(FILE* out) const
{
    for (unsigned i = 0; i < msgs.size(); ++i)
    {
        fprintf(out, "Subset %d:\n", i);
        msgs[i]->print(out);
    }
}

unsigned Messages::diff(const Messages& o) const
{
    unsigned diffs = 0;
    if (msgs.size() != o.msgs.size())
    {
        notes::logf("the message groups contain a different number of messages (first is %zd, second is %zd)\n",
                msgs.size(), o.msgs.size());
        ++diffs;
    }
    size_t count = min(msgs.size(), o.msgs.size());
    for (size_t i = 0; i < count; ++i)
        diffs += msgs[i]->diff(*o.msgs[i]);
    return diffs;
}

#endif

}
