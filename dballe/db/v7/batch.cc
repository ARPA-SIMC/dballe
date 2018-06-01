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

batch::Station* Batch::get_station(const dballe::Station& station, bool station_can_add)
{
    batch::Station* res = find_existing(station.report, station.coords, station.ident);
    if (res)
    {
        if (res->id != station.id)
            throw std::runtime_error("batch station has different id than the station we are using to look it up");
        return res;
    }
    v7::Station& st = transaction->station();

    stations.emplace_back();
    res = &stations.back(); // FIXME: in C++17, emplace_back already returns a reference
    res->transaction = transaction;
    res->report = station.report;
    res->coords = station.coords;
    res->ident = station.ident;
    if (station.id != MISSING_INT)
        res->id = station.id;
    else
        res->id = st.maybe_get_id(*transaction, *res);
    if (res->id == MISSING_INT)
    {
        if (!station_can_add)
            throw wreport::error_notfound("station not found in the database");
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

void Batch::commit(bool with_attrs)
{
    for (auto& st: stations)
        st.commit(with_attrs);
}

namespace batch {

void StationDatum::dump(FILE* out) const
{
    fprintf(out, "%01d%02d%03d(%d): %s\n",
            WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}

void MeasuredDatum::dump(FILE* out) const
{
    fprintf(out, "ltr:%d %01d%02d%03d(%d): %s\n",
            id_levtr,
            WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}


void StationData::add(const wreport::Var* var, UpdateMode on_conflict)
{
    if (!loaded)
        throw std::runtime_error("StationData::add called without loading status from DB first");
    auto in_db = ids_by_code.find(var->code());
    if (in_db != ids_by_code.end())
    {
        // Exists in the database
        switch (on_conflict)
        {
            case UPDATE: to_update.emplace_back(in_db->second, var); break;
            case IGNORE: break;
            case ERROR: throw wreport::error_consistency("refusing to overwrite existing data");
        }
    } else {
        // Does not exist in the database
        to_insert.emplace_back(var);
    }
}

void StationData::commit(Transaction& tr, int station_id, bool with_attrs)
{
    if (!to_insert.empty())
    {
        auto& st = tr.station_data();
        st.insert(tr, station_id, to_insert, with_attrs);
        for (const auto& v: to_insert)
            ids_by_code[v.var->code()] = v.id;
    }
    if (!to_update.empty())
    {
        auto& st = tr.station_data();
        st.update(tr, to_update, with_attrs);
    }
}

void MeasuredData::add(int id_levtr, const wreport::Var* var, UpdateMode on_conflict)
{
    auto in_db = ids_on_db.find(IdVarcode(id_levtr, var->code()));
    if (in_db != ids_on_db.end())
    {
        // Exists in the database
        switch (on_conflict)
        {
            case UPDATE: to_update.emplace_back(in_db->second, id_levtr, var); break;
            case IGNORE: break;
            case ERROR: throw wreport::error_consistency("refusing to overwrite existing data");
        }
    } else {
        // Does not exist in the database
        to_insert.emplace_back(id_levtr, var);
    }
}

void MeasuredData::commit(Transaction& tr, int station_id, bool with_attrs)
{
    if (!to_insert.empty())
    {
        auto& st = tr.data();
        st.insert(tr, station_id, datetime, to_insert, with_attrs);
        for (const auto& v: to_insert)
            ids_on_db[IdVarcode(v.id_levtr, v.var->code())] = v.id;
    }
    if (!to_update.empty())
    {
        auto& st = tr.data();
        st.update(tr, to_update, with_attrs);
    }
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
    auto i = measured_data.find(datetime);
    if (i != measured_data.end())
        return i->second;

    auto inserted = measured_data.emplace(datetime, datetime);
    MeasuredData& md = inserted.first->second;

    if (!is_new)
    {
        v7::Data& d = transaction->data();
        d.query(id, datetime, [&](int data_id, int id_levtr, wreport::Varcode code) {
            md.ids_on_db.insert(std::make_pair(IdVarcode(id_levtr, code), data_id));
        });
    }

    return md;
}

void Station::commit(bool with_attrs)
{
    if (id == MISSING_INT)
        id = transaction->station().obtain_id(*transaction, *this);

    station_data.commit(*transaction, id, with_attrs);
    for (auto& md: measured_data)
        md.second.commit(*transaction, id, with_attrs);
}

#if 0
void InsertStationVars::dump(FILE* out) const
{
    fprintf(out, "ID station: %d\n", shared_context.station);
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
            shared_context.station,
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    for (unsigned i = 0; i < size(); ++i)
    {
        fprintf(out, "%3u/%3zd: ", i, size());
        (*this)[i].dump(out);
    }
}
#endif

}

}
}
}
