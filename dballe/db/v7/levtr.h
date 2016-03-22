#ifndef DBALLE_DB_V7_LEVTR_H
#define DBALLE_DB_V7_LEVTR_H

#include <dballe/core/defs.h>
#include <memory>
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
    struct DBRow
    {
        /// lev_tr ID SQL parameter
        int id;
        /// First level type SQL parameter
        int ltype1;
        /// Level L1 SQL parameter
        int l1;
        /// Second level type SQL parameter
        int ltype2;
        /// Level L2 SQL parameter
        int l2;
        /// Time range type SQL parameter
        int pind;
        /// Time range P1 SQL parameter
        int p1;
        /// Time range P2 SQL parameter
        int p2;
    };

    virtual ~LevTr();

    /**
     * Return the ID for the given Level and Trange, adding it to the database
     * if it does not already exist
     */
    virtual int obtain_id(const Level& lev, const Trange& tr) = 0;

    /// Read the LevTr data for an id, returns nullptr if not found
    virtual const DBRow* read(int id) = 0;

    /// Read the contents of the LevTr table
    virtual void read_all(std::function<void(const DBRow&)> dest) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;
};

struct LevTrCache
{
    virtual ~LevTrCache();

    /**
     * Fill a record with level/timerange info with this id.
     *
     * @return true if found, else false
     */
    virtual bool to_rec(int id, Record& rec) = 0;

    /// Return a Level for this ID
    virtual Level to_level(int id) const = 0;

    /// Return a Trange for this ID
    virtual Trange to_trange(int id) const = 0;

    /**
     * Get/create a Context in the Msg for this level/timerange.
     *
     * @returns the context, or 0 if the id is not valid.
     */
    virtual msg::Context* to_msg(int id, Msg& msg) = 0;

    /// Invalidate the cache
    virtual void invalidate() = 0;

    /// Dump cache contents to an output stream
    virtual void dump(FILE* out) const = 0;

    /// Create a new LevTrCache for this LevTr
    static std::unique_ptr<LevTrCache> create(LevTr& levtr);
};

}
}
}

#endif

