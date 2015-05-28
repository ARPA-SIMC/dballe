#ifndef DBA_MEMDB_LTR_H
#define DBA_MEMDB_LTR_H

#include <dballe/memdb/valuestorage.h>
#include <dballe/memdb/index.h>
#include <dballe/core/defs.h>

namespace dballe {
struct Record;

namespace core {
struct Query;
}

namespace memdb {
template<typename T> struct Results;

/// Aggregated level and time range information
struct LevTr
{
    Level level;
    Trange trange;

    LevTr(const Level& level, const Trange& trange)
        : level(level), trange(trange) {}

    bool operator<(const LevTr& o) const { return compare(o) < 0; }
    bool operator>(const LevTr& o) const { return compare(o) > 0; }
    bool operator==(const LevTr& o) const { return level == o.level && trange == o.trange; }
    bool operator!=(const LevTr& o) const { return level != o.level || trange != o.trange; }

    /**
     * Compare two LevTr strutures, for use in sorting.
     *
     * @return
     *   -1 if *this < l, 0 if *this == l, 1 if *this > l
     */
    int compare(const LevTr& o) const
    {
        if (int res = level.compare(o.level)) return res;
        return trange.compare(o.trange);
    }
};

/// Storage and index for level and time range aggregate sets
class LevTrs : public ValueStorage<LevTr>
{
protected:
    Index<Level> by_level;
    Index<Trange> by_trange;

public:
    LevTrs();

    void clear();

    /// Get a LevTr record
    size_t obtain(const Level& level, const Trange& trange);

    /// Get a LevTr record
    size_t obtain(const Record& rec);

    /// Query levtrs returning the IDs
    void query(const core::Query& q, Results<LevTr>& res) const;

    void dump(FILE* out) const;
};

}
}

#endif

