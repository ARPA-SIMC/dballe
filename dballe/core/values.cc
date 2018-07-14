#include "values.h"
#include "record.h"
#include "json.h"
#include <arpa/inet.h>
#include <ostream>

using namespace std;
using namespace wreport;

namespace dballe {

void Station::set_from_record(const Record& rec)
{
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

void Station::to_record(Record& rec) const
{
    rec.set("rep_memo", report);

    rec.set_coords(coords);
    if (ident.is_missing())
    {
        rec.unset("ident");
        rec.seti("mobile", 0);
    } else {
        rec.setc("ident", ident);
        rec.seti("mobile", 1);
    }
}

void Station::print(FILE* out, const char* end) const
{
    if (coords.is_missing())
        fputs("(-,-) ", out);
    else
        coords.print(out, " ");

    if (ident.is_missing())
        fputs("-", out);
    else
        fputs(ident.get(), out);

    fprintf(out, " %s%s", report.c_str(), end);
}

void Station::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("r", report);
    writer.add("c", coords);
    if (ident) writer.add("i", ident);
    writer.end_mapping();
}

Station Station::from_json(core::json::Stream& in)
{
    Station res;
    in.parse_object([&](const std::string& key) {
        if (key == "r")
            res.report = in.parse_string();
        else if (key == "c")
            res.coords = in.parse_coords();
        else if (key == "i")
            res.ident = in.parse_ident();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for Station");
    });
    return res;
}

std::ostream& operator<<(std::ostream& out, const Station& st)
{
    return out << st.coords << "," << st.ident << "," << st.report;
}


void DBStation::set_from_record(const Record& rec)
{
    if (const Var* var = rec.get("ana_id"))
    {
        // If we have ana_id, the rest is optional
        id = var->enqi();
        coords.lat = rec.enq("lat", MISSING_INT);
        coords.lon = rec.enq("lon", MISSING_INT);
        ident.clear();
        if (const Var* var = rec.get("ident"))
            ident = var->isset() ? var->enqc() : 0;
        report = rec.enq("rep_memo", "");
    } else {
        // If we do not have ana_id, we require at least lat, lon and rep_memo
        id = MISSING_INT;
        Station::set_from_record(rec);
    }
}

void DBStation::to_record(Record& rec) const
{
    if (id != MISSING_INT)
        rec.set("ana_id", id);
    else
        rec.unset("ana_id");

    Station::to_record(rec);
}

void DBStation::print(FILE* out, const char* end) const
{
    if (id == MISSING_INT)
        fputs("- ", out);
    else
        fprintf(out, "%d,", id);

    Station::print(out, end);
}

void DBStation::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    if (id != MISSING_INT) writer.add("id", id);
    writer.add("r", report);
    writer.add("c", coords);
    if (ident) writer.add("i", ident);
    writer.end_mapping();
}

DBStation DBStation::from_json(core::json::Stream& in)
{
    DBStation res;
    in.parse_object([&](const std::string& key) {
        if (key == "id")
            res.id = in.parse_unsigned<int>();
        else if (key == "r")
            res.report = in.parse_string();
        else if (key == "c")
            res.coords = in.parse_coords();
        else if (key == "i")
            res.ident = in.parse_ident();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for Station");
    });
    return res;
}

std::ostream& operator<<(std::ostream& out, const DBStation& st)
{
    if (st.id == MISSING_INT)
        out << "-,";
    else
        out << st.id << ",";

    return out << (const Station&)st;
}


void Sampling::set_from_record(const Record& rec)
{
    DBStation::set_from_record(rec);
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
    DBStation::print(out, " ");

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
