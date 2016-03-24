#include "state.h"

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

int StationDesc::compare(const StationDesc& o) const
{
    if (int res = rep - o.rep) return res;
    if (int res = coords.compare(o.coords)) return res;
    return ident.compare(o.ident);
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
    if (int res = levtr->first.compare(o.levtr->first)) return res;
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
    return res.first;
}

values_t::iterator State::add_value(const ValueDesc& desc, const ValueState& state)
{
    auto res = values.insert(make_pair(desc, state));
    return res.first;
}

}
}
}

