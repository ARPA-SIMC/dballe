#include "batch.h"
#include "transaction.h"
#include "station.h"

namespace dballe {
namespace db {
namespace v7 {

batch::Station* Batch::find_existing(const std::string& report, const Coords& coords, const Ident& ident)
{
    auto li = stations_by_lon.find(coords.lon);
    if (li == stations_by_lon.end())
        return nullptr;
    for (auto i: li->second)
    {
        batch::Station* st = &stations[i];
        if (st->report == report && st->coords == coords && st->ident == ident)
            return st;
    }
    return nullptr;
}

void Batch::index_existing(batch::Station* st, size_t pos)
{
    auto li = stations_by_lon.find(st->coords.lon);
    if (li == stations_by_lon.end())
        stations_by_lon.insert(std::make_pair(st->coords.lon, std::vector<size_t>{pos}));
    else
        li->second.push_back(pos);
}


batch::Station* Batch::get_station(const std::string& report, const Coords& coords, const Ident& ident)
{
    batch::Station* res = find_existing(report, coords, ident);
    if (res) return res;

    v7::Station& st = transaction->station();

    stations.emplace_back();
    res = &stations.back(); // FIXME: in C++17, emplace_back already returns a reference
    res->transaction = transaction;
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
    index_existing(res, stations.size() - 1);
    return res;
}

void Batch::commit()
{
    throw std::runtime_error("not implemented yet");
}

namespace batch {

void StationData::add(const wreport::Var& var, bool overwrite, bool with_attrs)
{
    if (with_attrs)
        throw std::runtime_error("StationData::add with_attrs not yet implemented"); // TODO
    if (!loaded)
        throw std::runtime_error("StationData::add called without loading status from DB first");
    auto in_db = ids_by_code.find(var.code());
    if (in_db != ids_by_code.end())
    {
        // Exists in the database
        if (!overwrite)
            return;
        to_update.emplace_back(in_db->second, var);
    } else {
        // Does not exist in the database
        to_insert.emplace_back(var);
    }
}

void MeasuredData::add(int id_levtr, const wreport::Var& var, bool overwrite, bool with_attrs)
{
    throw std::runtime_error("not implemented yet");
}


StationData& Station::get_station_data()
{
    if (!station_data.loaded)
    {
        v7::StationData& sd = transaction->station_data();
        sd.query(id, [&](int data_id, wreport::Varcode code) {
            station_data.ids_by_code.insert(std::make_pair(code, data_id));
        });
        station_data.loaded = true;
    }
    return station_data;
}

MeasuredData& Station::get_measured_data(const Datetime& datetime)
{
    throw std::runtime_error("not implemented yet");
}

}

}
}
}
