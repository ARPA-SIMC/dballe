/*
 * db/sql/runqueryv6 - db-independent support for specific v6 queries
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

#include "driver.h"
#include "config.h"
#include "dballe/db/sqlite/driver.h"
#include "dballe/db/sqlite/internals.h"
#ifdef HAVE_LIBPQ
#include "dballe/db/postgresql/driver.h"
#include "dballe/db/postgresql/internals.h"
#endif
#ifdef HAVE_ODBC
#include "dballe/db/odbc/driver.h"
#include "dballe/db/odbc/internals.h"
#endif
#include <cstring>
#include <sstream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sql {

bool SQLRecordV6::querybest_fields_are_the_same(const SQLRecordV6& r)
{
    if (out_ana_id != r.out_ana_id) return false;
    if (out_id_ltr != r.out_id_ltr) return false;
    if (out_datetime != r.out_datetime) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}

void SQLRecordV6::dump(FILE* out)
{
    fprintf(out, "st: %d %6d %6d ", out_ana_id, out_lat, out_lon);
    if (out_ident_size == -1)
        fputs("fixed, ", out);
    else
        fprintf(out, "%.*s, ", out_ident_size, out_ident);
    fprintf(out, "rc: %d %d, ltr %d, did %d, ", out_rep_cod, priority, out_id_ltr, out_id_data);
    stringstream s;
    s << "dt: " << out_datetime << " " << out_datetimemax << ", ";
    fputs(s.str().c_str(), out);
    fprintf(out, "%d%02d%03d %s\n", WR_VAR_F(out_varcode), WR_VAR_X(out_varcode), WR_VAR_Y(out_varcode), out_value);
}

namespace bulk {

bool AnnotateVarsV6::annotate(int id_data, int id_levtr, Varcode code, const char* value)
{
    //fprintf(stderr, "ANNOTATE ");
    while (iter != vars.end())
    {
        //fprintf(stderr, "id_data: %d/%d  id_levtr: %d/%d  varcode: %d/%d  value: %s/%s: ", id_data, iter->id_data, id_levtr, iter->id_levtr, code, iter->var->code(), value, iter->var->value());

        // This variable is not on our list: stop here and wait for a new one
        if (id_levtr < iter->id_levtr)
        {
            //fprintf(stderr, "levtr lower than ours, wait for next\n");
            return true;
        }

        // iter points to a variable that is not currently in the DB
        if (id_levtr > iter->id_levtr)
        {
            //fprintf(stderr, "levtr higher than ours, insert this\n");
            do_insert = true;
            iter->set_needs_insert();
            ++iter;
            continue;
        }

        // id_levtr is the same

        // This variable is not on our list: stop here and wait for a new one
        if (code < iter->var->code())
        {
            //fprintf(stderr, "varcode lower than ours, wait for next\n");
            return true;
        }

        // iter points to a variable that is not currently in the DB
        if (code > iter->var->code())
        {
            //fprintf(stderr, "varcode higher than ours, insert this\n");
            do_insert = true;
            iter->set_needs_insert();
            ++iter;
            continue;
        }

        // iter points to a variable that is also in the DB

        // Annotate with the ID
        //fprintf(stderr, "id_data=%d ", id_data);
        iter->id_data = id_data;

        // If the value is different, we need to update
        if (strcmp(value, iter->var->value()) != 0)
        {
            //fprintf(stderr, "needs_update ");
            iter->set_needs_update();
            do_update = true;
        }

        // We processed this variable: stop here and wait for a new one
        ++iter;
        //fprintf(stderr, "wait for next\n");
        return true;
    }

    // We have no more variables to consider: signal the caller that they can
    // stop iterating if they wish.
    //fprintf(stderr, "done.\n");
    return false;
}

void AnnotateVarsV6::annotate_end()
{
    // Mark all remaining variables as needing insert
    for ( ; iter != vars.end(); ++iter)
    {
        //fprintf(stderr, "LEFTOVER: id_levtr: %d  varcode: %d  value: %s\n", iter->id_levtr, iter->var->code(), iter->var->value());
        iter->set_needs_insert();
        do_insert = true;
    }
}

}

Driver::~Driver()
{
}

void Driver::bulk_insert_v6(bulk::InsertV6& vars, bool update_existing)
{
    throw error_unimplemented("bulk insert v6 not implemented on this database connector");
}

std::unique_ptr<Driver> Driver::create(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<Driver>(new sqlite::Driver(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<Driver>(new odbc::Driver(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<Driver>(new postgresql::Driver(*c));
#endif
    else
        throw error_unimplemented("DB drivers only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

}
}
}
