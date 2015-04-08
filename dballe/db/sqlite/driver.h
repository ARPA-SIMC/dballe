/*
 * db/sqlite/driver - Backend SQLite driver
 *
 * Copyright (C) 2014--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#ifndef DBALLE_DB_SQLITE_DRIVER_H
#define DBALLE_DB_SQLITE_DRIVER_H

#include <dballe/db/sql/driver.h>

namespace dballe {
namespace db {
struct SQLiteConnection;

namespace sqlite {

struct Driver : public sql::Driver
{
    SQLiteConnection& conn;

    Driver(SQLiteConnection& conn);
    virtual ~Driver();

    std::unique_ptr<sql::Repinfo> create_repinfov5() override;
    std::unique_ptr<sql::Repinfo> create_repinfov6() override;
    std::unique_ptr<sql::Station> create_stationv5() override;
    std::unique_ptr<sql::Station> create_stationv6() override;
    std::unique_ptr<sql::LevTr> create_levtrv6() override;
    std::unique_ptr<sql::DataV5> create_datav5() override;
    std::unique_ptr<sql::DataV6> create_datav6() override;
    std::unique_ptr<sql::AttrV5> create_attrv5() override;
    std::unique_ptr<sql::AttrV6> create_attrv6() override;
    void run_built_query_v6(const v6::QueryBuilder& qb, std::function<void(sql::SQLRecordV6& rec)> dest) override;
    void run_delete_query_v6(const v6::QueryBuilder& qb) override;
    void create_tables_v5() override;
    void create_tables_v6() override;
    void delete_tables_v5() override;
    void delete_tables_v6() override;
};

}
}
}
#endif
