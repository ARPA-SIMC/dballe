#ifndef DBALLE_DB_V7_LEVTR_H
#define DBALLE_DB_V7_LEVTR_H

#include <dballe/core/defs.h>
#include <dballe/db/v7/state.h>
#include <memory>
#include <set>
#include <cstdio>
#include <functional>

namespace dballe {
struct Record;
struct Msg;

namespace msg {
struct Context;
}

namespace db {
namespace v7 {

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct LevTr
{
protected:
    virtual void _dump(std::function<void(int, const Level&, const Trange&)> out) = 0;

public:
    virtual ~LevTr();

    /**
     * Given a set of IDs, load LevTr information for them and add it to data.
     */
    virtual void prefetch_ids(const std::set<int>& ids, std::function<void(int, const LevTrDesc&)> dest) = 0;

    /**
     * Given an ID, load LevTr information for all the entries with the same level type
     */
    virtual void prefetch_same_level(int id, std::function<void(int, const LevTrDesc&)> dest) = 0;

    /**
     * Get/create a Context in the Msg for this level/timerange.
     *
     * @returns the context, or 0 if the id is not valid.
     */
    msg::Context* to_msg(State& st, int id, Msg& msg);

    /// Look up a LevTr from the database given its ID.
    virtual levtrs_t::iterator lookup_id(State& st, int id) = 0;

    /**
     * Look up a LevTr from the database given its description. Insert a new
     * one if not found.
     */
    virtual levtrs_t::iterator obtain_id(State& state, const LevTrDesc& desc) = 0;

    /// Dump the entire contents of the table to an output stream
    void dump(FILE* out);
};

}
}
}
#endif
