#include "state.h"

namespace dballe {
namespace db {
namespace v7 {

int StationDesc::compare(const StationDesc& o) const
{
    if (int res = rep - o.rep) return res;
    if (int res = coords.compare(o.coords)) return res;
    return ident.compare(o.ident);
}

void State::clear()
{
    stations.clear();
}

}
}
}

