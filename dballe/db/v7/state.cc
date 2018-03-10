#include "state.h"
#include "dballe/record.h"
#include <algorithm>
#include <ostream>

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

bool LevTrEntry::operator==(const LevTrEntry& o) const
{
    if (id != MISSING_INT && o.id != MISSING_INT)
        return id == o.id;
    if (level != o.level) return false;
    return trange == o.trange;
}

bool LevTrEntry::operator!=(const LevTrEntry& o) const
{
    if (id != MISSING_INT && o.id != MISSING_INT)
        return id != o.id;
    if (level == o.level) return false;
    return trange != o.trange;
}

std::ostream& operator<<(std::ostream& out, const LevTrEntry& l)
{
    out << "(";
    if (l.id == MISSING_INT)
        out << "-";
    else
        out << l.id;
    return out << ":" << l.level << ":" << l.trange;
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

