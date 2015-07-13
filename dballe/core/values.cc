#include "values.h"
#include "record.h"

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
