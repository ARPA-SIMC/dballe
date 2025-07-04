#ifndef DBALLE_DB_V7_LEVTR_H
#define DBALLE_DB_V7_LEVTR_H

#include <cstdio>
#include <dballe/core/defs.h>
#include <dballe/db/v7/cache.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/msg/fwd.h>
#include <functional>
#include <memory>
#include <set>

namespace dballe {
namespace db {
namespace v7 {

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct LevTr
{
protected:
    v7::Transaction& tr;
    LevTrCache cache;
    virtual void
    _dump(std::function<void(int, const Level&, const Trange&)> out) = 0;

public:
    LevTr(v7::Transaction& tr);
    virtual ~LevTr();

    /**
     * Invalidate the LevTrEntry cache.
     *
     * Further accesses will be done via the database, and slowly repopulate
     * the cache from scratch.
     */
    void clear_cache();

    /**
     * Given a set of IDs, load LevTr information for them and add it to the
     * cache.
     */
    virtual void prefetch_ids(Tracer<>& trc, const std::set<int>& ids) = 0;

    /**
     * Get/create a Context in the Msg for this level/timerange.
     *
     * @returns the context, or 0 if the id is not valid.
     */
    impl::msg::Context* to_msg(Tracer<>& trc, int id, impl::Message& msg);

    /**
     * Lookup a LevTr entry from the cache, throwing an exception if it is not
     * found
     */
    const LevTrEntry& lookup_cache(int id);

    /// Look up a LevTr from the database given its ID.
    virtual const LevTrEntry* lookup_id(Tracer<>& trc, int id) = 0;

    /**
     * Look up a LevTr from the database given its description. Insert a new
     * one if not found.
     */
    virtual int obtain_id(Tracer<>& trc, const LevTrEntry& desc) = 0;

    /// Dump the entire contents of the table to an output stream
    void dump(FILE* out);
};

} // namespace v7
} // namespace db
} // namespace dballe
#endif
