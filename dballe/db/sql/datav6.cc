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
#include "datav6.h"
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace sql {

DataV6::~DataV6() {}

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

void AnnotateVarsV6::dump(FILE* out) const
{
    fprintf(out, "Needs insert: %d, needs update: %d\n", do_insert, do_update);
    vars.dump(out);
}

void VarV6::dump(FILE* out) const
{
    fprintf(out, "ltr:%d data:%d flags:%c%c%c%c %01d%02d%03d(%d): %s\n",
            id_levtr, id_data,
#if 0
            (flags & FLAG_NEEDS_UPDATE) ? 'u' : '-',
            (flags & FLAG_UPDATED) ? 'U' : '-',
            (flags & FLAG_NEEDS_INSERT) ? 'i' : '-',
            (flags & FLAG_INSERTED) ? 'I' : '-',
#endif
            needs_update() ? 'u' : '-',
            updated() ? 'U' : '-',
            needs_insert() ? 'i' : '-',
            inserted() ? 'I' : '-',
            WR_VAR_F(var->code()), WR_VAR_X(var->code()), WR_VAR_Y(var->code()),
            (int)(var->code()),
            var->value());
}

void InsertV6::dump(FILE* out) const
{
    fprintf(out, "ID station: %d, ID report: %d, datetime: %04d-%02d-%02d %02d:%02d:%02d\n",
            id_station, id_report,
            datetime.date.year, datetime.date.month, datetime.date.day,
            datetime.time.hour, datetime.time.minute, datetime.time.second);
    for (unsigned i = 0; i < size(); ++i)
    {
        fprintf(out, "%3u/%3zd: ", i, size());
        (*this)[i].dump(out);
    }
}

}

}
}
}

