#ifndef DBALLE_DB_V7_LEVTR_H
#define DBALLE_DB_V7_LEVTR_H

#include <dballe/core/defs.h>
#include <dballe/db/v7/state.h>
#include <memory>
#include <set>
#include <cstdio>

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
    virtual ~LevTr();

    /**
     * Given a set of IDs, load LevTr information for them and add it to data.
     */
    virtual void prefetch_ids(const std::set<int>& ids, std::map<int, LevTrDesc>& data) = 0;

    /**
     * Get/create a Context in the Msg for this level/timerange.
     *
     * @returns the context, or 0 if the id is not valid.
     */
    msg::Context* to_msg(State& st, int id, Msg& msg);

    /// Look up a LevTr from the database given its ID.
    virtual levels_t::iterator lookup_id(State& st, int id) = 0;

    /**
     * Look up a LevTr from the database given its description. Insert a new
     * one if not found.
     */
    virtual levels_t::iterator obtain_id(State& state, const LevTrDesc& desc) = 0;

#if 0
    /// Read the LevTr data for an id, returns nullptr if not found
    virtual const DBRow* read(int id) = 0;

    /// Read the contents of the LevTr table
    virtual void read_all(std::function<void(const DBRow&)> dest) = 0;
#endif

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;
};

}
}
}
#endif
