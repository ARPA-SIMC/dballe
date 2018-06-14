#include "batch.h"
#include "transaction.h"
#include "station.h"
#include <algorithm>

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
        if (station.id != MISSING_INT && res->id != station.id)
            throw std::runtime_error("batch station has different id than the station we are using to look it up");
        return res;
    }
    v7::Station& st = transaction.station();

    stations.emplace_back(*this);
    res = &stations.back(); // FIXME: in C++17, emplace_back already returns a reference
    res->report = station.report;
    res->coords = station.coords;
    res->ident = station.ident;
    if (station.id != MISSING_INT)
        res->id = station.id;
    else
    {
        ++count_select_stations;
        res->id = st.maybe_get_id(transaction, *res);
    }
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

    v7::Station& st = transaction.station();

    stations.emplace_back(*this);
    res = &stations.back(); // FIXME: in C++17, emplace_back already returns a reference
    res->report = report;
    res->coords = coords;
    res->ident = ident;
    res->id = st.maybe_get_id(transaction, *res);
    ++count_select_stations;
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

void Batch::write_pending(bool with_attrs)
{
    for (auto& st: stations)
        st.write_pending(with_attrs);
}

void Batch::clear()
{
    stations_by_lon.clear();
    stations.clear();
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

void StationData::write_pending(Transaction& tr, int station_id, bool with_attrs)
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
    to_insert.clear();
    to_update.clear();
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

void MeasuredData::write_pending(Transaction& tr, int station_id, bool with_attrs)
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
    to_insert.clear();
    to_update.clear();
}


MeasuredDataVector::~MeasuredDataVector()
{
    for (auto md: measured_data)
        delete md;
}

MeasuredData* MeasuredDataVector::find(const Datetime& datetime)
{
    if (measured_data.empty()) return nullptr;

    // Stick to linear search if the vector size is small
    if (measured_data.size() < 6)
    {
        for (auto md: measured_data)
            if (md->datetime == datetime)
                return md;
        return nullptr;
    }

    // Use binary search for larger vectors
    if (dirty)
    {
        std::sort(measured_data.begin(), measured_data.end(), [](const MeasuredData* a, const MeasuredData* b) {
            return a->datetime < b->datetime;
        });
        dirty = false;
    }

    // Binary search
    int begin, end;
    begin = -1, end = measured_data.size();
    while (end - begin > 1)
    {
        int cur = (end + begin) / 2;
        if (measured_data[cur]->datetime > datetime)
            end = cur;
        else
            begin = cur;
    }
    if (begin == -1 || measured_data[begin]->datetime != datetime)
        return nullptr;
    else
        return measured_data[begin];
}

MeasuredData& MeasuredDataVector::add(const Datetime& datetime)
{
    dirty = true;
    measured_data.push_back(new MeasuredData(datetime));
    return *measured_data.back();
}


StationData& Station::get_station_data()
{
    if (!station_data.loaded)
    {
        v7::StationData& sd = batch.transaction.station_data();
        sd.query(id, [&](int data_id, wreport::Varcode code) {
            station_data.ids_by_code.insert(std::make_pair(code, data_id));
        });
        station_data.loaded = true;
        ++batch.count_select_station_data;
    }
    return station_data;
}

MeasuredData& Station::get_measured_data(const Datetime& datetime)
{
    if (MeasuredData* md = measured_data.find(datetime))
        return *md;

    MeasuredData& md = measured_data.add(datetime);

    if (!is_new)
    {
        v7::Data& d = batch.transaction.data();
        d.query(id, datetime, [&](int data_id, int id_levtr, wreport::Varcode code) {
            md.ids_on_db.insert(std::make_pair(IdVarcode(id_levtr, code), data_id));
        });
        ++batch.count_select_data;
    }

    return md;
}

void Station::write_pending(bool with_attrs)
{
    if (id == MISSING_INT)
        id = batch.transaction.station().insert_new(batch.transaction, *this);

    station_data.write_pending(batch.transaction, id, with_attrs);
    for (auto md: measured_data.measured_data)
        md->write_pending(batch.transaction, id, with_attrs);
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
