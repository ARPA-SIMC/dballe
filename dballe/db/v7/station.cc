#include "station.h"

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Station::~Station()
{
}

stations_t::iterator Station::get_id(State& st, const StationDesc& desc)
{
    auto res = st.stations.find(desc);
    if (res != st.stations.end())
        return res;

    StationState state;
    if (maybe_get_id(desc, &state.id))
    {
        state.is_new = false;
        return st.add_station(desc, state);
    }
    throw error_notfound("station not found in the database");
}

void Station::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");

    _dump([&](int id, int rep, const Coords& coords, const char* ident) {
        fprintf(out, " %d, %d, %.5f, %.5f", id, rep, coords.dlat(), coords.dlon());
        if (!ident)
            putc('\n', out);
        else
            fprintf(out, ", %s\n", ident);
        ++count;
    });
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}

}
}
}
