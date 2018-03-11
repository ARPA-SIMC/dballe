#ifndef DBALLE_DB_V7_DATAV7_H
#define DBALLE_DB_V7_DATAV7_H

#include <dballe/core/defs.h>
#include <dballe/sql/fwd.h>
#include <dballe/db/defs.h>
#include <wreport/var.h>
#include <memory>
#include <vector>
#include <list>
#include <cstdio>
#include <functional>

namespace dballe {
struct Record;
struct Values;

namespace db {
namespace v7 {
struct Transaction;
struct IdQueryBuilder;

namespace bulk {
struct InsertStationVars;
struct InsertVars;

enum UpdateMode {
    UPDATE,
    IGNORE,
    ERROR,
};

}

template<typename Traits>
class DataCommon
{
protected:
    /**
     * Load attributes from the database into a Values
     */
    void read_attrs_into_values(int id_data, Values& values);

    /**
     * Replace the attributes of a variable with those in Values
     */
    virtual void write_attrs(int id_data, const Values& values) = 0;

    /**
     * Remove all attributes from a variable
     */
    virtual void remove_all_attrs(int id_data) = 0;

public:
    virtual ~DataCommon() {}

    /**
     * Load from the database all the attributes for var
     *
     * @param id_data
     *   ID of the data row for the value of which we will read attributes
     * @param dest
     *   Function that will be called to consume the attrbutes as they are
     *   loaded.
     */
    virtual void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Merge the given attributes with the existing attributes of the given
     * variable:
     *
     * * Existing attributes not in attrs are preserved.
     * * Existing attributes in attrs are overwritten.
     * * New attributes in attrs are inesrted.
     */
    void merge_attrs(int id_data, const Values& attrs);

    /**
     * Remove the given attributes from the given variable, if they exist.
     */
    void remove_attrs(int data_id, const db::AttrList& attrs);

    /// Bulk variable insert
    virtual void insert(dballe::db::v7::Transaction& t, typename Traits::BulkVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void remove(const v7::IdQueryBuilder& qb) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;

    virtual void clear_cache() = 0;
};


struct StationDataDumper
{
    unsigned count = 0;
    FILE* out;

    StationDataDumper(FILE* out);

    void print_head();
    void print_row(int id, int id_station, wreport::Varcode code, const char* val, const std::vector<uint8_t>& attrs);
    void print_tail();
};

struct DataDumper
{
    unsigned count = 0;
    FILE* out;

    DataDumper(FILE* out);

    void print_head();
    void print_row(int id, int id_station, int id_levtr, const Datetime& dt, wreport::Varcode code, const char* val, const std::vector<uint8_t>& attrs);
    void print_tail();
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


/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct StationVar : public Item
{
    int id = MISSING_INT;
    const wreport::Var* var;

    StationVar(const wreport::Var* var) : var(var) {}
    StationVar(int id, const wreport::Var* var)
        : id(id), var(var) {}

    void dump(FILE* out) const;
};


/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct Var : public Item
{
    int id = MISSING_INT;
    int id_levtr;
    const wreport::Var* var;

    Var(const wreport::Var* var, int id_levtr) : id_levtr(id_levtr), var(var) {}
    Var(int id, const wreport::Var* var, int id_levtr)
        : id(id), id_levtr(id_levtr), var(var) {}

    void dump(FILE* out) const;
};

struct SharedStationContext
{
    int station = MISSING_INT;

    SharedStationContext() = default;
    SharedStationContext(int station) : station(station) {}
};

struct SharedDataContext
{
    int station = MISSING_INT;
    Datetime datetime;

    SharedDataContext() = default;
    SharedDataContext(int station) : station(station) {}
    SharedDataContext(int station, const Datetime& datetime) : station(station), datetime(datetime) {}
};

template<typename var_t, typename shared_context_t>
struct InsertPlan : public std::vector<var_t>
{
    typedef typename std::vector<var_t>::iterator iterator;

    shared_context_t shared_context;

    bool do_insert = false;
    bool do_update = false;
    std::list<var_t*> to_query;

    template<typename... Args>
    InsertPlan(Args&&... args) : shared_context(std::forward<Args>(args)...) {}

    void look_for_missing_ids()
    {
        // Scan vars adding known IDs to the current state
        to_query.clear();
        for (auto& i: *this)
        {
            if (i.id != MISSING_INT) continue;
            to_query.push_back(&i);
        }
    }

    void compute_plan()
    {
        do_insert = false;
        do_update = false;
        for (auto& var: *this)
        {
            if (var.id == MISSING_INT)
            {
                var.set_needs_insert();
                do_insert = true;
            }
            else
            {
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

    void add(const wreport::Var* var)
    {
        emplace_back(var);
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

    bool has_datetime() const
    {
        return not shared_context.datetime.is_missing();
    }

    void set_datetime(const Datetime& dt)
    {
        shared_context.datetime = dt;
    }

    void add(const wreport::Var* var, int id_levtr)
    {
        emplace_back(var, id_levtr);
    }

    void dump(FILE* out) const;
};

}

struct StationDataTraits
{
    typedef bulk::InsertStationVars BulkVars;
    static const char* table_name;
};

struct DataTraits
{
    typedef bulk::InsertVars BulkVars;
    static const char* table_name;
};

extern template class DataCommon<StationDataTraits>;
extern template class DataCommon<DataTraits>;

typedef DataCommon<StationDataTraits> StationData;
typedef DataCommon<DataTraits> Data;

}
}
}
#endif
