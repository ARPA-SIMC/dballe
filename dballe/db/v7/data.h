#ifndef DBALLE_DB_V7_DATAV7_H
#define DBALLE_DB_V7_DATAV7_H

#include <dballe/core/defs.h>
#include <dballe/sql/fwd.h>
#include <dballe/db/v7/state.h>
#include <wreport/var.h>
#include <memory>
#include <vector>
#include <list>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
struct Transaction;
struct QueryBuilder;

namespace bulk {
struct InsertStationVars;
struct InsertVars;

enum UpdateMode {
    UPDATE,
    IGNORE,
    ERROR,
};

}

/**
 * Precompiled query to manipulate the data table
 */
struct StationData
{
public:
    virtual ~StationData();

    /// Bulk variable insert
    virtual void insert(dballe::db::v7::Transaction& t, bulk::InsertStationVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void remove(const v7::QueryBuilder& qb) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;
};

/**
 * Precompiled query to manipulate the data table
 */
struct Data
{
public:
    virtual ~Data();

    /// Bulk variable insert
    virtual void insert(dballe::db::v7::Transaction& t, bulk::InsertVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void remove(const v7::QueryBuilder& qb) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;
};


namespace bulk {

struct Item
{
    static const unsigned FLAG_NEEDS_UPDATE = 1 << 0;
    static const unsigned FLAG_UPDATED      = 1 << 1;
    static const unsigned FLAG_NEEDS_INSERT = 1 << 2;
    static const unsigned FLAG_INSERTED     = 1 << 3;
    unsigned flags = 0;

    bool needs_update() const { return flags & FLAG_NEEDS_UPDATE; }
    bool updated() const { return flags & FLAG_UPDATED; }
    bool needs_insert() const { return flags & FLAG_NEEDS_INSERT; }
    bool inserted() const { return flags & FLAG_INSERTED; }
    void set_needs_update() { flags |= FLAG_NEEDS_UPDATE; }
    void set_updated() { flags = (flags & ~FLAG_NEEDS_UPDATE) | FLAG_UPDATED; }
    void set_needs_insert() { flags |= FLAG_NEEDS_INSERT; }
    void set_inserted() { flags = (flags & ~FLAG_NEEDS_INSERT) | FLAG_INSERTED; }

    /**
     * Format flags in the first 4 characters of dest.
     *
     * It adds a trailing 0, so dest should be at least 5 bytes long.
     */
    void format_flags(char* dest) const;
};

template<typename state_t>
struct VarItem : public Item
{
    typename state_t::iterator cur;
    const wreport::Var* var;

    VarItem(typename state_t::iterator cur, const wreport::Var* var)
        : cur(cur), var(var) {}
};

/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct StationVar : public VarItem<stationvalues_t>
{
    using VarItem::VarItem;

    bool is_new() const { return false; }
    bool has_cur(State& state) const { return cur != state.stationvalues.end(); }
    void fill_cur(State& state, const StationValueDesc& desc) { cur = state.stationvalues.find(desc); }
    void dump(FILE* out) const;
};


/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct Var : public VarItem<values_t>
{
    levtrs_t::iterator levtr;

    Var(values_t::iterator cur, const wreport::Var* var, levtrs_t::iterator levtr)
        : VarItem(cur, var), levtr(levtr) {}

    bool is_new() const { return levtr->second.is_new; }
    bool has_cur(State& state) const { return cur != state.values.end(); }
    void fill_cur(State& state, const ValueDesc& desc) { cur = state.values.find(desc); }
    void dump(FILE* out) const;
};

struct SharedContext
{
    stations_t::iterator station;

    SharedContext() {}
    SharedContext(stations_t::iterator station) : station(station) {}

    bool is_new() const { return station->second.is_new; }
};

struct SharedStationContext : public SharedContext
{
    using SharedContext::SharedContext;

    StationValueDesc make_desc(StationVar& v) const
    {
        return StationValueDesc(station, v.var->code());
    }
};

struct SharedDataContext : public SharedContext
{
    Datetime datetime;

    SharedDataContext() {}
    SharedDataContext(stations_t::iterator station, const Datetime& datetime) : SharedContext(station), datetime(datetime) {}

    ValueDesc make_desc(Var& v) const
    {
        return ValueDesc(station, v.levtr, datetime, v.var->code());
    }
};

template<typename var_t, typename shared_context_t>
struct InsertPlan : public std::vector<var_t>
{
    typedef typename std::vector<var_t>::iterator iterator;

    State& state;
    shared_context_t shared_context;

    bool do_insert = false;
    bool do_update = false;
    std::list<var_t*> to_query;

    template<typename... Args>
    InsertPlan(State& state, Args&&... args) : state(state), shared_context(std::forward<Args>(args)...) {}

    /**
     * Fill the cur state pointer in all variables to insert.
     *
     * When state info is not available, add the variable to to_query.
     */
    void map_known_values()
    {
        to_query.clear();
        for (auto i = this->begin(); i != this->end(); ++i)
        {
            i->fill_cur(state, shared_context.make_desc(*i));
            if (i->has_cur(state)) continue;
            if (shared_context.is_new() || i->is_new()) continue;
            to_query.push_back(&*i);
        }
    }

    void compute_plan()
    {
        do_insert = false;
        do_update = false;
        for (auto& var: *this)
        {
            if (!var.has_cur(state))
            {
                var.set_needs_insert();
                do_insert = true;
            }
            else if (var.cur->second.value != var.var->enqc())
            {
                // If the value is different, we need to update
                var.set_needs_update();
                do_update = true;
            }
        }
    }
};

/**
 * Input for a bulk insert of a lot of variables sharing the same context
 * information.
 */
struct InsertStationVars : public InsertPlan<StationVar, SharedStationContext>
{
    using InsertPlan::InsertPlan;

    StationValueDesc make_desc(iterator& i) const
    {
        return StationValueDesc(shared_context.station, i->var->code());
    }

    void add(const wreport::Var* var)
    {
        emplace_back(state.stationvalues.end(), var);
    }

    void dump(FILE* out) const;
};


/**
 * Input for a bulk insert of a lot of variables sharing the same context
 * information.
 */
struct InsertVars : public InsertPlan<Var, SharedDataContext>
{
    using InsertPlan::InsertPlan;

    void add(const wreport::Var* var, levtrs_t::iterator levtr)
    {
        emplace_back(state.values.end(), var, levtr);
    }

    void dump(FILE* out) const;
};

}
}
}
}
#endif
