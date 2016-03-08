#include <ostream>
#include <dballe/core/ostream.h>
#include <dballe/db/mem/cursor.h>
#include <wreport/varinfo.h>

namespace dballe {

namespace db {
namespace mem {
namespace cursor {
struct DataBestKey;

std::ostream& operator<<(std::ostream& out, const DataBestKey& k)
{
    const memdb::Value& v = k.value();

    out << v.station.coords
        << "." << v.station.ident
        << ":" << v.levtr.level
        << ":" << v.levtr.trange
        << ":" << v.datetime
        << ":" << wreport::varcode_format(v.var->code());
    return out;
}

}
}
}

}

