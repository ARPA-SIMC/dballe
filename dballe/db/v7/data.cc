#include "data.h"
#include "dballe/core/values.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {

const char* StationDataTraits::table_name = "station_data";
const char* DataTraits::table_name = "data";

template<typename Traits>
const char* DataCommon<Traits>::table_name = Traits::table_name;

template<typename Traits>
void DataCommon<Traits>::read_attrs_into_values(int id_data, Values& values)
{
    read_attrs(id_data, [&](unique_ptr<wreport::Var> var) { values.set(move(var)); });
}

template<typename Traits>
void DataCommon<Traits>::merge_attrs(int id_data, const Values& attrs)
{
    // Read existing attributes
    Values merged;
    read_attrs_into_values(id_data, merged);

    // Merge attributes from attrs
    merged.set(attrs);

    // Write them back
    write_attrs(id_data, merged);
}

template<typename Traits>
void DataCommon<Traits>::remove_attrs(int id_data, const db::AttrList& attrs)
{
    if (attrs.empty())
        remove_all_attrs(id_data);
    else {
        // Read existing attributes
        Values remaining;
        read_attrs_into_values(id_data, remaining);

        for (const auto& i: attrs)
            remaining.erase(i);

        if (remaining.empty())
            remove_all_attrs(id_data);
        else
            write_attrs(id_data, remaining);
    }
}

template class DataCommon<StationDataTraits>;
template class DataCommon<DataTraits>;


StationDataDumper::StationDataDumper(FILE* out)
    : out(out)
{
}

void StationDataDumper::print_head()
{
    fprintf(out, "dump of table station_data:\n");
    fprintf(out, " id   st   var\n");
}

void StationDataDumper::print_row(int id, int id_station, wreport::Varcode code, const char* val, const std::vector<uint8_t>& attrs)
{
    fprintf(out, " %4d %4d %01d%02d%03d", id, id_station, WR_VAR_FXY(code));
    if (!val)
        fprintf(out, "\n");
    else
        fprintf(out, " %s\n", val);

    Values::decode(attrs, [&](std::unique_ptr<wreport::Var> var) {
        fprintf(out, "     ");
        var->print(out);
    });

    ++count;
}

void StationDataDumper::print_tail()
{
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

DataDumper::DataDumper(FILE* out)
    : out(out)
{
}

void DataDumper::print_head()
{
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   ltr  datetime              var\n");
}

void DataDumper::print_row(int id, int id_station, int id_levtr, const Datetime& dt, wreport::Varcode code, const char* val, const std::vector<uint8_t>& attrs)
{
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

    Values::decode(attrs, [&](std::unique_ptr<wreport::Var> var) {
        fprintf(out, "     ");
        var->print(out);
    });

    ++count;
}

void DataDumper::print_tail()
{
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
