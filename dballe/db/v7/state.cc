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

void Stations::clear()
{
    for (auto& i: by_id)
        delete i.second;
    by_id.clear();
    by_lon.clear();
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

const dballe::Station* Stations::insert(const dballe::Station& st)
{
    return insert(std::unique_ptr<dballe::Station>(new dballe::Station(st)));
}

const dballe::Station* Stations::insert(const dballe::Station& st, int id)
{
    std::unique_ptr<dballe::Station> nst(new dballe::Station(st));
    nst->ana_id = id;
    return insert(move(nst));
}

const dballe::Station* Stations::insert(std::unique_ptr<dballe::Station> st)
{
    if (st->ana_id == MISSING_INT)
        throw std::runtime_error("station to cache in transaction state must have a database ID");

    auto i = by_id.find(st->ana_id);
    if (i != by_id.end())
    {
        // Stations do not move: if we have a match on the ID, we just need to
        // enforce that there is no mismatch on the station data
        if (*i->second != *st)
            throw std::runtime_error("cannot replace a station with one with the same ID and different data");
        return i->second;
    }

    const dballe::Station* res;
    res = st.get();
    by_id.insert(make_pair(res->ana_id, st.release()));
    by_lon_add(res);
    return res;
}

void Stations::by_lon_add(const dballe::Station* st)
{
    auto li = by_lon.find(st->coords.lon);
    if (li == by_lon.end())
        by_lon.insert(make_pair(st->coords.lon, std::vector<const dballe::Station*>{st}));
    else
        li->second.push_back(st);
}


int LevTrDesc::compare(const LevTrDesc& o) const
{
    if (int res = level.compare(o.level)) return res;
    return trange.compare(o.trange);
}

int StationValueDesc::compare(const StationValueDesc& o) const
{
    if (int res = station - o.station) return res;
    return varcode - o.varcode;
}

int ValueDesc::compare(const ValueDesc& o) const
{
    if (int res = station - o.station) return res;
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

