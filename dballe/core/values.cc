#include "values.h"
#include "record.h"

using namespace std;
using namespace wreport;

namespace dballe {

void Station::from_record(const Record& rec)
{
    if (const Var* var = rec.get("ana_id"))
    {
        // If we have ana_id, the rest is optional
        ana_id = var->enqi();
        coords.lat = rec.enq("lat", MISSING_INT);
        coords.lat = rec.enq("lon", MISSING_INT);
        ident.clear();
        if (const Var* var = rec.get("ident"))
            ident = var->value();
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
            ident = var->value();

        report.clear();
        if (const Var* var = rec.get("rep_memo"))
            report = var->value();
        if (report.empty())
            throw error_notfound("record has no 'rep_memo' set");
    }
}

void Sampling::from_record(const Record& rec)
{
    Station::from_record(rec);
    const auto& r = core::Record::downcast(rec);
    datetime = r.get_datetime();
    if (datetime.is_missing()) throw error_notfound("record has no date and time information set");
    level = r.get_level();
    if (level.is_missing()) throw error_notfound("record has no level information set");
    trange = r.get_trange();
    if (trange.is_missing()) throw error_notfound("record has no time range information set");
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

void Values::from_record(const Record& rec)
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

void StationValues::from_record(const Record& rec)
{
    info.from_record(rec);
    values.from_record(rec);
}

void DataValues::from_record(const Record& rec)
{
    info.from_record(rec);
    values.from_record(rec);
}

}
