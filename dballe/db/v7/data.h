#ifndef DBALLE_DB_V7_DATAV7_H
#define DBALLE_DB_V7_DATAV7_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/core/defs.h>
#include <dballe/sql/fwd.h>
#include <wreport/var.h>
#include <memory>
#include <vector>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
struct QueryBuilder;

namespace bulk {
struct InsertV7;
}

/**
 * Precompiled query to manipulate the data table
 */
struct DataV7
{
public:
    enum UpdateMode {
        UPDATE,
        IGNORE,
        ERROR,
    };

    virtual ~DataV7();

    /// Bulk variable insert
    virtual void insert(dballe::Transaction& t, bulk::InsertV7& vars, UpdateMode update_mode=UPDATE) = 0;

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

/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct VarV7 : public Item
{
    int id_levtr;
    int id_data;
    const wreport::Var* var;

    VarV7(const wreport::Var* var, int id_levtr=-1, int id_data=-1)
        : id_levtr(id_levtr), id_data(id_data), var(var)
    {
    }
    bool operator<(const VarV7& v) const
    {
        if (int d = id_levtr - v.id_levtr) return d < 0;
        return var->code() < v.var->code();
    }

    void dump(FILE* out) const;
};


/**
 * Input for a bulk insert of a lot of variables sharing the same context
 * information.
 */
struct InsertV7 : public std::vector<VarV7>
{
    int id_station;
    int id_report;
    Datetime datetime;

    void add(const wreport::Var* var, int id_levtr)
    {
        emplace_back(var, id_levtr);
    }

    void dump(FILE* out) const;
};

/**
 * Helper class for annotating InsertV7 variables with the current status of
 * the database.
 */
struct AnnotateVarsV7
{
    InsertV7& vars;
    InsertV7::iterator iter;
    bool do_insert = false;
    bool do_update = false;

    AnnotateVarsV7(InsertV7& vars);

    bool annotate(int id_data, int id_levtr, wreport::Varcode code, const char* value);
    void annotate_end();

    void dump(FILE* out) const;
};

}



}
}
}

#endif

