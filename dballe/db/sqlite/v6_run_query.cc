/*
 * db/odbc/v6_run_query - run a prebuilt query on the SQLite connector
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "v6_run_query.h"
#include "dballe/db/v6/qbuilder.h"
#include <algorithm>
#include <cstring>

using namespace std;

namespace dballe {
namespace db {
namespace v6 {

void sqlite_run_built_query(SQLiteConnection& conn, const QueryBuilder& qb,
        std::function<void(SQLRecord& rec)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    SQLRecord rec;
    stm->execute([&]() {
        int output_seq = 0;
        SQLLEN out_ident_ind;

        if (qb.select_station)
        {
            rec.out_ana_id = stm->column_int(output_seq++);
            rec.out_lat = stm->column_int(output_seq++);
            rec.out_lon = stm->column_int(output_seq++);
            if (stm->column_isnull(output_seq))
            {
                rec.out_ident_size == -1;
                rec.out_ident[0] = 0;
            } else {
                string ident = stm->column_string(output_seq);
                rec.out_ident_size = max(ident.size(), (string::size_type)63);
                memcpy(rec.out_ident, ident.data(), rec.out_ident_size);
                rec.out_ident[rec.out_ident_size] = 0;
            }
            ++output_seq;
        }

        if (qb.select_varinfo)
        {
            rec.out_rep_cod = stm->column_int(output_seq++);
            rec.out_id_ltr = stm->column_int(output_seq++);
            rec.out_varcode = stm->column_int(output_seq++);
        }

        if (qb.select_data_id)
            rec.out_id_data = stm->column_int(output_seq++);

        if (qb.select_data)
        {
            string dt = stm->column_string(output_seq++);
            sscanf(dt.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
                    &rec.out_datetime.date.year,
                    &rec.out_datetime.date.month,
                    &rec.out_datetime.date.day,
                    &rec.out_datetime.time.hour,
                    &rec.out_datetime.time.minute,
                    &rec.out_datetime.time.second);

            string value = stm->column_string(output_seq++);
            unsigned val_size = max(value.size(), (string::size_type)255);
            memcpy(rec.out_value, value.data(), val_size);
            rec.out_value[val_size] = 0;
        }

        dest(rec);
    });
}

void sqlite_run_delete_query(SQLiteConnection& conn, const QueryBuilder& qb)
{
    auto stmd = conn.sqlitestatement("DELETE FROM data WHERE id=?");
    auto stma = conn.sqlitestatement("DELETE FROM attr WHERE id_data=?");
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        // Get the list of data to delete
        int out_id_data = stm->column_int(0);

        // Compile the DELETE query for the data
        stmd->bind_val(1, out_id_data);
        stmd->execute();

        // Compile the DELETE query for the attributes
        stma->bind_val(1, out_id_data);
        stma->execute();
    });
}

}
}
}
