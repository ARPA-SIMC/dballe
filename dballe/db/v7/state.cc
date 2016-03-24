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

void State::clear()
{
    stations.clear();
    levels.clear();
    level_ids.clear();
}

stations_t::iterator State::add_station(const StationDesc& desc, const StationState& state)
{
    auto res = stations.insert(make_pair(desc, state));
    return res.first;
}

levels_t::iterator State::add_levtr(const LevTrDesc& desc, const LevTrState& state)
{
    auto res = levels.insert(make_pair(desc, state));
    level_ids.insert(make_pair(state.id, res.first));
    return res.first;
}

}
}
}

