#include "state.h"
#include "dballe/record.h"
#include <algorithm>

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Stations::~Stations()
{
    for (auto& i: by_id)
        delete i.second;
}

const dballe::Station* Stations::find_station(int id) const
{
    auto i = by_id.find(id);
    if (i == by_id.end())
        return nullptr;
    return i->second;
}

int Stations::find_id(const dballe::Station& st) const
{
    if (st.ana_id != MISSING_INT)
        return st.ana_id;
    auto li = by_lon.find(st.coords.lon);
    if (li == by_lon.end())
        return MISSING_INT;
    for (auto i: li->second)
        if (i->report == st.report && i->coords == st.coords && i->ident == st.ident)
            return i->ana_id;
    return MISSING_INT;
}

void Stations::insert(const dballe::Station& st)
{
    insert(std::unique_ptr<dballe::Station>(new dballe::Station(st)));
}

void Stations::insert(std::unique_ptr<dballe::Station> st)
{
    if (st->ana_id == MISSING_INT)
        throw std::runtime_error("station to cache in transaction state must have a database ID");

    auto i = by_id.find(st->ana_id);
    if (i == by_id.end())
    {
        dballe::Station* ns = st.release();
        by_id.insert(make_pair(ns->ana_id, ns));
        by_lon_add(ns);
    } else {
        std::unique_ptr<dballe::Station> old(i->second);
        dballe::Station* ns;
        i->second = ns = st.release();
        by_lon_remove(old.get());
        by_lon_add(ns);
    }
}

void Stations::by_lon_add(const dballe::Station* st)
{
    auto li = by_lon.find(st->coords.lon);
    if (li == by_lon.end())
        by_lon.insert(make_pair(st->coords.lon, std::vector<const dballe::Station*>{st}));
    else
        li->second.push_back(st);
}

void Stations::by_lon_remove(const dballe::Station* st)
{
    auto li = by_lon.find(st->coords.lon);
    if (li == by_lon.end())
        return;
    auto i = std::find(li->second.begin(), li->second.end(), st);
    if (i == li->second.end())
        return;
    li->second.erase(i);
}


int StationDesc::compare(const StationDesc& o) const
{
    if (int res = rep - o.rep) return res;
    if (int res = coords.compare(o.coords)) return res;
    return ident.compare(o.ident);
}

void StationDesc::to_record(Record& rec) const
{
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

int LevTrDesc::compare(const LevTrDesc& o) const
{
    if (int res = level.compare(o.level)) return res;
    return trange.compare(o.trange);
}

int StationValueDesc::compare(const StationValueDesc& o) const
{
    if (int res = station->first.compare(o.station->first)) return res;
    return varcode - o.varcode;
}

int ValueDesc::compare(const ValueDesc& o) const
{
    if (int res = station->first.compare(o.station->first)) return res;
    if (int res = levtr - o.levtr) return res;
    if (int res = datetime.compare(o.datetime)) return res;
    return varcode - o.varcode;
}

void State::clear()
{
    stations.clear();
    levtrs.clear();
    levtr_ids.clear();
    stationvalues.clear();
    values.clear();
}

stations_t::iterator State::add_station(const StationDesc& desc, const StationState& state)
{
    auto res = stations.insert(make_pair(desc, state));
    return res.first;
}

levtrs_t::iterator State::add_levtr(const LevTrDesc& desc, const LevTrState& state)
{
    auto res = levtrs.insert(make_pair(desc, state));
    levtr_ids.insert(make_pair(state.id, res.first));
    return res.first;
}

stationvalues_t::iterator State::add_stationvalue(const StationValueDesc& desc, const StationValueState& state)
{
    auto res = stationvalues.insert(make_pair(desc, state));
    if (state.is_new) stationvalues_new.insert(state.id);
    return res.first;
}

values_t::iterator State::add_value(const ValueDesc& desc, const ValueState& state)
{
    auto res = values.insert(make_pair(desc, state));
    if (state.is_new) values_new.insert(state.id);
    return res.first;
}

}
}
}

