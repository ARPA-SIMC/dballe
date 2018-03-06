#include "station.h"
#include "dballe/core/values.h"
#include "transaction.h"

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Station::~Station()
{
}

int Station::get_id(v7::Transaction& tr, const dballe::Station& desc)
{
    auto id = tr.state.stations.find_id(desc);
    if (id != MISSING_INT)
        return id;

    if (maybe_get_id(tr, desc, &id))
    {
        tr.state.stations.insert(desc, id);
        return id;
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
