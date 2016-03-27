#include "data.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {

StationData::~StationData() {}

void StationData::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station_data:\n");
    fprintf(out, " id   st   var\n");
    _dump([&](int id, int id_station, wreport::Varcode code, const char* val) {
        fprintf(out, " %4d %4d %01d%02d%03d", id, id_station, WR_VAR_FXY(code));
        if (!val)
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", val);

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}


Data::~Data() {}

void Data::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   ltr  datetime              var\n");
    _dump([&](int id, int id_station, int id_levtr, const Datetime& dt, wreport::Varcode code, const char* val) {
        fprintf(out, " %4d %4d %04d %04d-%02d-%02d %02d:%02d:%02d %01d%02d%03d",
                id,
                id_station,
                id_levtr,
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                WR_VAR_FXY(code));
        if (!val)
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", val);

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}


namespace bulk {

void Item::format_flags(char* dest) const
{
    dest[0] = needs_update() ? 'u' : '-',
    dest[1] = updated() ? 'U' : '-',
    dest[2] = needs_insert() ? 'i' : '-',
    dest[3] = inserted() ? 'I' : '-',
    dest[4] = 0;
}

void StationVar::dump(FILE* out) const
{
    char flags[5];
    format_flags(flags);
    fprintf(out, "flags:%s %01d%02d%03d(%d): %s\n",
            flags, WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}

void Var::dump(FILE* out) const
{
    char flags[5];
    format_flags(flags);
    fprintf(out, "flags:%s %01d%02d%03d(%d): %s\n",
            flags, WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}

void InsertStationVars::dump(FILE* out) const
{
    fprintf(out, "ID station: %d\n", shared_context.station->second.id);
    for (unsigned i = 0; i < size(); ++i)
    {
        fprintf(out, "%3u/%3zd: ", i, size());
        (*this)[i].dump(out);
    }
}

void InsertVars::dump(FILE* out) const
{
    const auto& dt = shared_context.datetime;

    fprintf(out, "ID station: %d, datetime: %04d-%02d-%02d %02d:%02d:%02d\n",
            shared_context.station->second.id,
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
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
