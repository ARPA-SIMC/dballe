/*
 * db/sql/datav6 - interface to the V6 data table
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBALLE_DB_SQL_DATAV6_H
#define DBALLE_DB_SQL_DATAV6_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/core/defs.h>
#include <wreport/var.h>
#include <memory>
#include <vector>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
struct Connection;
struct Transaction;

namespace v6 {
struct QueryBuilder;
}

namespace sql {

namespace bulk {
struct InsertV6;
}

/**
 * Precompiled query to manipulate the data table
 */
struct DataV6
{
public:
    enum UpdateMode {
        UPDATE,
        IGNORE,
        ERROR,
    };

    virtual ~DataV6();

    /// Bulk variable insert
    virtual void insert(Transaction& t, bulk::InsertV6& vars, UpdateMode update_mode=UPDATE) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void remove(const v6::QueryBuilder& qb) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;
};


namespace bulk {

/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct VarV6
{
    static const unsigned FLAG_NEEDS_UPDATE = 1 << 0;
    static const unsigned FLAG_UPDATED      = 1 << 1;
    static const unsigned FLAG_NEEDS_INSERT = 1 << 2;
    static const unsigned FLAG_INSERTED     = 1 << 3;
    int id_levtr;
    int id_data;
    const wreport::Var* var;
    unsigned flags = 0;

    VarV6(const wreport::Var* var, int id_levtr=-1, int id_data=-1)
        : id_levtr(id_levtr), id_data(id_data), var(var)
    {
    }
    bool operator<(const VarV6& v) const
    {
        if (int d = id_levtr - v.id_levtr) return d < 0;
        return var->code() < v.var->code();
    }

    bool needs_update() const { return flags & FLAG_NEEDS_UPDATE; }
    bool updated() const { return flags & FLAG_UPDATED; }
    bool needs_insert() const { return flags & FLAG_NEEDS_INSERT; }
    bool inserted() const { return flags & FLAG_INSERTED; }
    void set_needs_update() { flags |= FLAG_NEEDS_UPDATE; }
    void set_updated() { flags = (flags & ~FLAG_NEEDS_UPDATE) | FLAG_UPDATED; }
    void set_needs_insert() { flags |= FLAG_NEEDS_INSERT; }
    void set_inserted() { flags = (flags & ~FLAG_NEEDS_INSERT) | FLAG_INSERTED; }

    void dump(FILE* out) const;
};


/**
 * Input for a bulk insert of a lot of variables sharing the same context
 * information.
 */
struct InsertV6 : public std::vector<VarV6>
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
 * Helper class for annotating InsertV6 variables with the current status of
 * the database.
 */
struct AnnotateVarsV6
{
    InsertV6& vars;
    InsertV6::iterator iter;
    bool do_insert = false;
    bool do_update = false;

    AnnotateVarsV6(InsertV6& vars) : vars(vars), iter(vars.begin()) {}

    bool annotate(int id_data, int id_levtr, wreport::Varcode code, const char* value);
    void annotate_end();

    void dump(FILE* out) const;
};

}



}
}
}

#endif

