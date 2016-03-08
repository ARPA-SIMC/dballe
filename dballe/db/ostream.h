#ifndef DBA_DB_OSTREAM_H
#define DBA_DB_OSTREAM_H

#include <iosfwd>

namespace dballe {

namespace db {
namespace mem {
namespace cursor {
struct DataBestKey;

std::ostream& operator<<(std::ostream& out, const DataBestKey& k);

}
}
}

}
#endif
