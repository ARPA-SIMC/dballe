#include "values.h"
#include "dballe/core/var.h"
#include <arpa/inet.h>
#include <ostream>

using namespace std;
using namespace wreport;

namespace dballe {
namespace core {
namespace value {

Encoder::Encoder()
{
    buf.reserve(64);
}

void Encoder::append_uint16(uint16_t val)
{
    uint16_t encoded = htons(val);
    buf.insert(buf.end(), (uint8_t*)&encoded, (uint8_t*)&encoded + 2);
}

void Encoder::append_uint32(uint32_t val)
{
    uint32_t encoded = htonl(val);
    buf.insert(buf.end(), (uint8_t*)&encoded, (uint8_t*)&encoded + 4);
}

void Encoder::append_cstring(const char* val)
{
    for ( ; *val; ++val)
        buf.push_back(*val);
    buf.push_back(0);
}

void Encoder::append(const wreport::Var& var)
{
    // Encode code
    append_uint16(var.code());
    switch (var.info()->type)
    {
        case Vartype::Binary:
        case Vartype::String:
            // Encode value, including terminating zero
            append_cstring(var.enqc());
            break;
        case Vartype::Integer:
        case Vartype::Decimal:
            // Just encode the integer value
            append_uint32(var.enqi());
            break;
    }
}

void Encoder::append_attributes(const wreport::Var& var)
{
    for (const Var* a = var.next_attr(); a != NULL; a = a->next_attr())
        append(*a);
}

Decoder::Decoder(const std::vector<uint8_t>& buf) : buf(buf.data()), size(buf.size()) {}

uint16_t Decoder::decode_uint16()
{
    if (size < 2) error_toolong::throwf("cannot decode a 16 bit integer: only %u bytes are left to read", size);
    uint16_t res = ntohs(*(uint16_t*)buf);
    buf += 2;
    size -= 2;
    return res;
}

uint32_t Decoder::decode_uint32()
{
    if (size < 4) error_toolong::throwf("cannot decode a 32 bit integer: only %u bytes are left to read", size);
    uint32_t res = ntohl(*(uint32_t*)buf);
    buf += 4;
    size -= 4;
    return res;
}

const char* Decoder::decode_cstring()
{
    if (!size) error_toolong::throwf("cannot decode a C string: the buffer is empty");
    const char* res = (const char*)buf;
    while (true)
    {
        if (!size) error_toolong::throwf("cannot decode a C string: reached the end of buffer before finding the string terminator");
        if (*buf == 0) break;
        ++buf;
        --size;
    }
    ++buf;
    --size;
    return res;
}

unique_ptr<wreport::Var> Decoder::decode_var()
{
    wreport::Varinfo info = varinfo(decode_uint16());
    switch (info->type)
    {
        case Vartype::Binary:
        case Vartype::String:
            return unique_ptr<wreport::Var>(new wreport::Var(info, decode_cstring()));
        case Vartype::Integer:
        case Vartype::Decimal:
            return unique_ptr<wreport::Var>(new wreport::Var(info, (int)decode_uint32()));
        default:
            error_consistency::throwf("unsupported variable type %d", (int)info->type);
    }
}

void Decoder::decode_attrs(const std::vector<uint8_t>& buf, wreport::Var& var)
{
    Decoder dec(buf);
    while (dec.size)
        var.seta(move(dec.decode_var()));
}

}
}
}
