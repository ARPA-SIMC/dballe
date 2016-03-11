/*
 * db/postgresql/repinfo - repinfo table management
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

#ifndef DBALLE_DB_POSTGRESQL_REPINFO_H
#define DBALLE_DB_POSTGRESQL_REPINFO_H

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

#include <dballe/db/sql/repinfo.h>
#include <dballe/sql/fwd.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
namespace postgresql {

/**
 * Fast cached access to the repinfo table
 */
struct PostgreSQLRepinfoBase : public sql::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    dballe::sql::PostgreSQLConnection& conn;

    PostgreSQLRepinfoBase(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLRepinfoBase(const PostgreSQLRepinfoBase&) = delete;
    PostgreSQLRepinfoBase(const PostgreSQLRepinfoBase&&) = delete;
    virtual ~PostgreSQLRepinfoBase();
    PostgreSQLRepinfoBase& operator=(const PostgreSQLRepinfoBase&) = delete;

    void dump(FILE* out) override;

protected:
    int id_use_count(unsigned id, const char* name) override;
    void delete_entry(unsigned id) override;
    void update_entry(const sql::repinfo::Cache& entry) override;
    void insert_entry(const sql::repinfo::Cache& entry) override;

    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

struct PostgreSQLRepinfoV6 : public PostgreSQLRepinfoBase
{
    PostgreSQLRepinfoV6(dballe::sql::PostgreSQLConnection& conn);

protected:
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
#endif

