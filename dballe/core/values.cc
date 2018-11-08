#include "values.h"
#include "record.h"
#include "json.h"
#include <arpa/inet.h>
#include <ostream>

using namespace std;
using namespace wreport;

namespace dballe {
namespace values {

void Value::print(FILE* out) const
{
    if (data_id == MISSING_INT)
        fputs("-------- ", out);
    else
        fprintf(out, "%8d ", data_id);
    if (!var)
        fputs("-\n", out);
    else
        var->print(out);
}

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

bool Values::operator==(const Values& o) const
{
    auto a = begin();
    auto b = o.begin();
    while (a != end() && b != o.end())
    {
        if (*a != *b) return false;
        ++a;
        ++b;
    }
    return a == end() && b == o.end();
}

void Values::add_data_id(wreport::Varcode code, int data_id)
{
    auto i = find(code);
    if (i == end()) return;
    i->second.data_id = data_id;
}

void Values::set_from_record(const Record& rec)
{
    const auto& r = core::Record::downcast(rec);
    for (const auto& i: r.vars())
        set(*i);
}

void Values::set(const wreport::Var& v)
{
    auto i = find(v.code());
    if (i == end())
        insert(make_pair(v.code(), values::Value(v)));
    else
        i->second.set(v);
}

void Values::set(std::unique_ptr<wreport::Var>&& v)
{
    auto code = v->code();
    auto i = find(code);
    if (i == end())
        insert(make_pair(code, values::Value(move(v))));
    else
        i->second.set(move(v));
}

void Values::set(const Values& vals)
{
    for (const auto& vi: vals)
        set(*vi.second.var);
}

const values::Value& Values::operator[](wreport::Varcode code) const
{
    auto i = find(code);
    if (i == end())
        error_notfound::throwf("variable %01d%02d%03d not found",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
    return i->second;
}

const values::Value* Values::get(wreport::Varcode code) const
{
    auto i = find(code);
    if (i == end())
        return nullptr;
    return &i->second;
}

void Values::print(FILE* out) const
{
    for (const auto& i: *this)
        i.second.print(out);
}

std::vector<uint8_t> Values::encode() const
{
    values::Encoder enc;
    for (const auto& i: *this)
        enc.append(*i.second.var);
    return enc.buf;
}

std::vector<uint8_t> Values::encode_attrs(const wreport::Var& var)
{
    values::Encoder enc;
    for (const Var* a = var.next_attr(); a != NULL; a = a->next_attr())
        enc.append(*a);
    return enc.buf;
}

void Values::decode(const std::vector<uint8_t>& buf, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    values::Decoder dec(buf);
    while (dec.size)
        dest(move(dec.decode_var()));
}


DataValues::DataValues(const dballe::Record& rec)
    : station(rec.get_dbstation()), datetime(rec.get_datetime()), level(rec.get_level()), trange(rec.get_trange()), values(rec) {}

void DataValues::set_from_record(const Record& rec)
{
    station = rec.get_dbstation();
    datetime = rec.get_datetime();
    if (datetime.is_missing()) throw error_notfound("record has no date and time information set");
    level = rec.get_level();
    if (level.is_missing()) throw error_notfound("record has no level information set");
    trange = rec.get_trange();
    if (trange.is_missing()) throw error_notfound("record has no time range information set");
    values.set_from_record(rec);
}

void DataValues::print(FILE* out, const char* end) const
{
    station.print(out, " ");

    if (datetime.is_missing())
        fputs("xxxx-xx-xx xx:xx:xx ", out);
    else
        datetime.print_iso8601(out, ' ', " ");

    level.print(out, "-", " ");
    trange.print(out, "-", end);
    values.print(out);
}

}
