/*
 * db/mysql/repinfo - repinfo table management
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_MYSQL_REPINFO_H
#define DBALLE_DB_MYSQL_REPINFO_H

#include <dballe/db/sql/repinfo.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
struct MySQLConnection;

namespace mysql {

/**
 * Fast cached access to the repinfo table
 */
struct MySQLRepinfoV5 : public sql::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    MySQLConnection& conn;

    MySQLRepinfoV5(MySQLConnection& conn);
    MySQLRepinfoV5(const MySQLRepinfoV5&) = delete;
    MySQLRepinfoV5(const MySQLRepinfoV5&&) = delete;
    virtual ~MySQLRepinfoV5();
    MySQLRepinfoV5& operator=(const MySQLRepinfoV5&) = delete;

    void dump(FILE* out) override;

protected:
    void delete_entry(unsigned id) override;
    void update_entry(const sql::repinfo::Cache& entry) override;
    void insert_entry(const sql::repinfo::Cache& entry) override;
    int id_use_count(unsigned id, const char* name) override;
    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

struct MySQLRepinfoV6 : public MySQLRepinfoV5
{
    MySQLRepinfoV6(MySQLConnection& conn);

protected:
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
#endif

