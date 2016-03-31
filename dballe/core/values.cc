#include "values.h"
#include "record.h"
#include <arpa/inet.h>

using namespace std;
using namespace wreport;

namespace dballe {

void Station::set_from_record(const Record& rec)
{
    if (const Var* var = rec.get("ana_id"))
    {
        // If we have ana_id, the rest is optional
        ana_id = var->enqi();
        coords.lat = rec.enq("lat", MISSING_INT);
        coords.lat = rec.enq("lon", MISSING_INT);
        ident.clear();
        if (const Var* var = rec.get("ident"))
            ident = var->isset() ? var->enqc() : 0;
        report = rec.enq("rep_memo", "");
    } else {
        // If we do not have ana_id, we require at least lat, lon and rep_memo
        ana_id = MISSING_INT;

        if (const Var* var = rec.get("lat"))
            coords.lat = var->enqi();
        else
            throw error_notfound("record has no 'lat' set");

        if (const Var* var = rec.get("lon"))
            coords.lon = var->enqi();
        else
            throw error_notfound("record has no 'lon' set");

        ident.clear();
        if (const Var* var = rec.get("ident"))
            ident = var->isset() ? var->enqc() : 0;

        report.clear();
        if (const Var* var = rec.get("rep_memo"))
        {
            if (var->isset())
                report = var->enqs();
            else
                throw error_notfound("record has no 'rep_memo' set");
        }
    }
}

void Station::print(FILE* out, const char* end) const
{
    if (ana_id == MISSING_INT)
        fputs("- ", out);
    else
        fprintf(out, "%d,", ana_id);

    if (coords.is_missing())
        fputs("(-,-) ", out);
    else
        coords.print(out, " ");

    if (ident.is_missing())
        fputs("-", out);
    else
        fputs(ident.get(), out);

    fputs(end, out);
}

void Sampling::set_from_record(const Record& rec)
{
    Station::set_from_record(rec);
    const auto& r = core::Record::downcast(rec);
    datetime = r.get_datetime();
    if (datetime.is_missing()) throw error_notfound("record has no date and time information set");
    level = r.get_level();
    if (level.is_missing()) throw error_notfound("record has no level information set");
    trange = r.get_trange();
    if (trange.is_missing()) throw error_notfound("record has no time range information set");
}

void Sampling::print(FILE* out, const char* end) const
{
    Station::print(out, " ");

    if (datetime.is_missing())
        fputs("xxxx-xx-xx xx:xx:xx ", out);
    else
        datetime.print_iso8601(out, ' ', " ");

    level.print(out, "-", " ");
    trange.print(out, "-", end);
}

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

namespace {

struct Encoder
{
    std::vector<uint8_t> buf;

    Encoder()
    {
        buf.reserve(64);
    }

    void append_uint16(uint16_t val)
    {
        uint16_t encoded = htons(val);
        buf.insert(buf.end(), (uint8_t*)&encoded, (uint8_t*)&encoded + 2);
    }

    void append_uint32(uint32_t val)
    {
        uint32_t encoded = htonl(val);
        buf.insert(buf.end(), (uint8_t*)&encoded, (uint8_t*)&encoded + 4);
    }

    void append_cstring(const char* val)
    {
        for ( ; *val; ++val)
            buf.push_back(*val);
        buf.push_back(0);
    }

    void append(const wreport::Var& var)
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
};

struct Decoder
{
    const uint8_t* buf;
    unsigned size;

    Decoder(const std::vector<uint8_t>& buf)
        : buf(buf.data()), size(buf.size())
    {
    }

    uint16_t decode_uint16()
    {
        if (size < 2) error_toolong::throwf("cannot decode a 16 bit integer: only %u bytes are left to read", size);
        uint16_t res = ntohs(*(uint16_t*)buf);
        buf += 2;
        size -= 2;
        return res;
    }

    uint32_t decode_uint32()
    {
        if (size < 4) error_toolong::throwf("cannot decode a 32 bit integer: only %u bytes are left to read", size);
        uint32_t res = ntohl(*(uint32_t*)buf);
        buf += 4;
        size -= 4;
        return res;
    }

    const char* decode_cstring()
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

    unique_ptr<wreport::Var> decode_var()
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
};

}

std::vector<uint8_t> Values::encode() const
{
    Encoder enc;
    for (const auto& i: *this)
        enc.append(*i.second.var);
    return enc.buf;
}

std::vector<uint8_t> Values::encode_attrs(const wreport::Var& var)
{
    Encoder enc;
    for (const Var* a = var.next_attr(); a != NULL; a = a->next_attr())
        enc.append(*a);
    return enc.buf;
}

void Values::decode(const std::vector<uint8_t>& buf, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    Decoder dec(buf);
    while (dec.size)
        dest(move(dec.decode_var()));
}


void StationValues::set_from_record(const Record& rec)
{
    info.set_from_record(rec);
    values.set_from_record(rec);
}

void StationValues::print(FILE* out) const
{
    info.print(out);
    values.print(out);
}

void DataValues::set_from_record(const Record& rec)
{
    info.set_from_record(rec);
    values.set_from_record(rec);
}

void DataValues::print(FILE* out) const
{
    info.print(out);
    values.print(out);
}

}
