#include "batch.h"
#include "transaction.h"
#include "station.h"
#include <algorithm>

namespace dballe {
namespace db {
namespace v7 {

Batch::~Batch()
{
    // Do not try to flush it, pending data may be lost unless write_pending is
    // called, and it's ok
    delete last_station;
}

void Batch::set_write_attrs(bool write_attrs)
{
    if (this->write_attrs != write_attrs)
    {
        // Throw away pending data. It should not cause damage, since all
        // insert operations have their own write_pending at the end
        // write_pending();
        clear();
    }
    this->write_attrs = write_attrs;
}

bool Batch::have_station(const std::string& report, const Coords& coords, const Ident& ident)
{
    return last_station && last_station->report == report && last_station->coords == coords && last_station->ident == ident;

}

void Batch::new_station(Tracer<>& trc, const std::string& report, const Coords& coords, const Ident& ident)
{
    if (last_station)
    {
        last_station->write_pending(trc, write_attrs);
        delete last_station;
        last_station = nullptr;
    }
    last_station = new batch::Station(*this);
    last_station->report = report;
    last_station->coords = coords;
    last_station->ident = ident;
}

batch::Station* Batch::get_station(Tracer<>& trc, const dballe::Station& station, bool station_can_add)
{
    if (have_station(station.report, station.coords, station.ident))
    {
        if (station.id != MISSING_INT && last_station->id != station.id)
            throw std::runtime_error("batch station has different id than the station we are using to look it up");
        return last_station;
    }
    v7::Station& st = transaction.station();

    new_station(trc, station.report, station.coords, station.ident);

    if (station.id != MISSING_INT)
        last_station->id = station.id;
    else
    {
        ++count_select_stations;
        last_station->id = st.maybe_get_id(trc, *last_station);
    }
    if (last_station->id == MISSING_INT)
    {
        if (!station_can_add)
            throw wreport::error_notfound("station not found in the database");
        last_station->is_new = true;
        last_station->station_data.loaded = true;
    }
    else
    {
        last_station->is_new = false;
        last_station->station_data.loaded = false;
    }
    return last_station;
}

batch::Station* Batch::get_station(Tracer<>& trc, const std::string& report, const Coords& coords, const Ident& ident)
{
    if (have_station(report, coords, ident))
        return last_station;

    v7::Station& st = transaction.station();

    new_station(trc, report, coords, ident);

    last_station->id = st.maybe_get_id(trc, *last_station);
    ++count_select_stations;
    if (last_station->id == MISSING_INT)
    {
        last_station->is_new = true;
        last_station->station_data.loaded = true;
    }
    else
    {
        last_station->is_new = false;
        last_station->station_data.loaded = false;
    }
    return last_station;
}

void Batch::write_pending(Tracer<>& trc)
{
    if (!last_station)
        return;
    last_station->write_pending(trc, write_attrs);
}

void Batch::clear()
{
    delete last_station;
    last_station = nullptr;
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

void StationData::write_pending(Tracer<>& trc, Transaction& tr, int station_id, bool with_attrs)
{
    if (!to_insert.empty())
    {
        auto& st = tr.station_data();
        st.insert(trc, station_id, to_insert, with_attrs);
        for (const auto& v: to_insert)
            ids_by_code[v.var->code()] = v.id;
    }
    if (!to_update.empty())
    {
        auto& st = tr.station_data();
        st.update(trc, to_update, with_attrs);
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

void MeasuredData::write_pending(Tracer<>& trc, Transaction& tr, int station_id, bool with_attrs)
{
    if (!to_insert.empty())
    {
        auto& st = tr.data();
        st.insert(trc, station_id, datetime, to_insert, with_attrs);
        for (const auto& v: to_insert)
            ids_on_db[IdVarcode(v.id_levtr, v.var->code())] = v.id;

    }
    if (!to_update.empty())
    {
        auto& st = tr.data();
        st.update(trc, to_update, with_attrs);
    }
    to_insert.clear();
    to_update.clear();
}


MeasuredDataVector::~MeasuredDataVector()
{
    for (auto md: items)
        delete md;
}


StationData& Station::get_station_data(Tracer<>& trc)
{
    if (!station_data.loaded)
    {
        v7::StationData& sd = batch.transaction.station_data();
        sd.query(trc, id, [&](int data_id, wreport::Varcode code) {
            station_data.ids_by_code.insert(std::make_pair(code, data_id));
        });
        station_data.loaded = true;
        ++batch.count_select_station_data;
    }
    return station_data;
}

MeasuredData& Station::get_measured_data(Tracer<>& trc, const Datetime& datetime)
{
    auto mdi = measured_data.find(datetime);
    if (mdi != measured_data.end())
        return **mdi;

    MeasuredData* md = measured_data.add(new MeasuredData(datetime));

    if (!is_new)
    {
        v7::Data& d = batch.transaction.data();
        d.query(trc, id, datetime, [&](int data_id, int id_levtr, wreport::Varcode code) {
            md->ids_on_db.insert(std::make_pair(IdVarcode(id_levtr, code), data_id));
        });
        ++batch.count_select_data;
    }

    return *md;
}

void Station::write_pending(Tracer<>& trc, bool with_attrs)
{
    if (id == MISSING_INT)
        id = batch.transaction.station().insert_new(trc, *this);

    station_data.write_pending(trc, batch.transaction, id, with_attrs);
    for (auto md: measured_data)
        md->write_pending(trc, batch.transaction, id, with_attrs);
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
