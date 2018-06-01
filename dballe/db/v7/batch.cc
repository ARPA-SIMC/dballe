#include "batch.h"
#include "transaction.h"
#include "station.h"

namespace dballe {
namespace db {
namespace v7 {

batch::Station* Batch::find_existing(const std::string& report, const Coords& coords, const Ident& ident) const
{
    auto li = stations_by_lon.find(coords.lon);
    if (li == stations_by_lon.end())
        return nullptr;
    for (auto i: li->second)
        if (i->report == report && i->coords == coords && i->ident == ident)
            return i;
    return nullptr;
}

void Batch::index_existing(batch::Station* st)
{
    auto li = stations_by_lon.find(st->coords.lon);
    if (li == stations_by_lon.end())
        stations_by_lon.insert(make_pair(st->coords.lon, std::vector<batch::Station*>{st}));
    else
        li->second.push_back(st);
}


batch::Station* Batch::get_station(const std::string& report, const Coords& coords, const Ident& ident)
{
    batch::Station* res = find_existing(report, coords, ident);
    if (res) return res;

    v7::Station& st = transaction->station();

    stations.emplace_back();
    res = &stations.back(); // FIXME: in C++17, emplace_back returns a reference
    res->report = report;
    res->coords = coords;
    res->ident = ident;
    res->id = st.maybe_get_id(*transaction, *res);
    if (res->id == MISSING_INT)
    {
        res->is_new = true;
        res->station_data.loaded = true;
    }
    else
    {
        res->is_new = false;
        res->station_data.loaded = false;
    }
    index_existing(res);
    return res;
}

void Batch::commit()
{
    throw std::runtime_error("not implemented yet");
}

namespace batch {

void Data::add(const wreport::Var& var, bool overwrite, bool with_attrs)
{
    throw std::runtime_error("not implemented yet");
}

MeasuredData& Station::get_measured_data(int id_levtr, const Datetime& datetime)
{
    throw std::runtime_error("not implemented yet");
}

}

}
}
}
