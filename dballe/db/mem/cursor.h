#ifndef DBA_DB_MEM_CURSOR_H
#define DBA_DB_MEM_CURSOR_H

#include <dballe/memdb/memdb.h>
#include <dballe/memdb/results.h>
#include <dballe/db/db.h>
#include <iosfwd>

namespace dballe {
struct DB;
struct Record;

namespace memdb {
template<typename T> class ValueStorage;
}

namespace db {

namespace mem {
struct DB;

/**
 * Simple typedef to make typing easier, and also to help some versions of swig
 * match this complex type
 */
typedef std::vector<wreport::Varcode> AttrList;


namespace cursor {

std::unique_ptr<db::CursorStation> createStations(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Station>& res);
std::unique_ptr<db::CursorStationData> createStationData(mem::DB& db, unsigned modifiers, memdb::Results<memdb::StationValue>& res);
std::unique_ptr<db::CursorData> createData(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Value>& res);
std::unique_ptr<db::CursorData> createDataBest(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Value>& res);
std::unique_ptr<db::CursorSummary> createSummary(mem::DB& db, unsigned modifiers, memdb::Results<memdb::Value>& res);

/**
 * Wrapper around a Value index that compares so that all values from which the
 * best report should be selected appear to be the same.
 *
 * This is exported only so that it can be unit tested.
 */
struct DataBestKey
{
    const memdb::ValueStorage<memdb::Value>& values;
    size_t idx;

    DataBestKey(const memdb::ValueStorage<memdb::Value>& values, size_t idx);
    const memdb::Value& value() const { return *values[idx]; }
    bool operator<(const DataBestKey& o) const;
};
std::ostream& operator<<(std::ostream& out, const DataBestKey& k);

}

}
}
}
#endif
