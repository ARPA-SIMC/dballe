#include "data.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {

StationData::~StationData() {}

Data::~Data() {}

namespace bulk {

void Item::format_flags(char* dest) const
{
    dest[0] = needs_update() ? 'u' : '-',
    dest[1] = updated() ? 'U' : '-',
    dest[2] = needs_insert() ? 'i' : '-',
    dest[3] = inserted() ? 'I' : '-',
    dest[4] = 0;
}

AnnotateVarsV7::AnnotateVarsV7(InsertVars& vars)
    : vars(vars)
{
    std::sort(vars.begin(), vars.end());
    iter = vars.begin();
}

bool AnnotateVarsV7::annotate(int id_data, int id_levtr, Varcode code, const char* value)
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
        if (strcmp(value, iter->var->enqc()) != 0)
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

void AnnotateVarsV7::annotate_end()
{
    // Mark all remaining variables as needing insert
    for ( ; iter != vars.end(); ++iter)
    {
        //fprintf(stderr, "LEFTOVER: id_levtr: %d  varcode: %d  value: %s\n", iter->id_levtr, iter->var->code(), iter->var->value());
        iter->set_needs_insert();
        do_insert = true;
    }
}

void AnnotateVarsV7::dump(FILE* out) const
{
    fprintf(out, "Needs insert: %d, needs update: %d\n", do_insert, do_update);
    vars.dump(out);
}

void StationVar::dump(FILE* out) const
{
    char flags[5];
    format_flags(flags);
    fprintf(out, "data:%d flags:%s %01d%02d%03d(%d): %s\n",
            id_data, flags, WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}

void Var::dump(FILE* out) const
{
    char flags[5];
    format_flags(flags);
    fprintf(out, "ltr:%d data:%d flags:%s %01d%02d%03d(%d): %s\n",
            id_levtr, id_data, flags, WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}

void InsertStationVars::dump(FILE* out) const
{
    fprintf(out, "ID station: %d\n", station.id);
    for (unsigned i = 0; i < size(); ++i)
    {
        fprintf(out, "%3u/%3zd: ", i, size());
        (*this)[i].dump(out);
    }
}

void InsertVars::dump(FILE* out) const
{
    fprintf(out, "ID station: %d, datetime: %04d-%02d-%02d %02d:%02d:%02d\n",
            station.id,
            datetime.year, datetime.month, datetime.day,
            datetime.hour, datetime.minute, datetime.second);
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

