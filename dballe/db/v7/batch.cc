#include "batch.h"
#include "station.h"
#include "transaction.h"
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

bool Batch::have_station(const std::string& report, const Coords& coords,
                         const Ident& ident)
{
    return last_station && last_station->report == report &&
           last_station->coords == coords && last_station->ident == ident;
}

void Batch::new_station(Tracer<>& trc, const std::string& report,
                        const Coords& coords, const Ident& ident)
{
    if (last_station)
    {
        last_station->write_pending(trc, write_attrs);
        delete last_station;
        last_station = nullptr;
    }
    last_station         = new batch::Station(*this);
    last_station->report = report;
    last_station->coords = coords;
    last_station->ident  = ident;
}

batch::Station* Batch::get_station(Tracer<>& trc,
                                   const dballe::DBStation& station,
                                   bool station_can_add)
{
    v7::Station& st = transaction.station();

    if (station.coords.is_missing())
    {
        if (station.id == MISSING_INT)
            throw std::runtime_error("cannot use station information without "
                                     "both coordinates and ana_id");
        if (last_station && last_station->id == station.id)
            return last_station;
        DBStation from_db = st.lookup(trc, station.id);
        new_station(trc, from_db.report, from_db.coords, from_db.ident);
        ++count_select_stations;
        last_station->id = station.id;
    }
    else
    {
        if (have_station(station.report, station.coords, station.ident))
            return last_station;
        new_station(trc, station.report, station.coords, station.ident);
        ++count_select_stations;
        last_station->id = st.maybe_get_id(trc, *last_station);
    }

    if (last_station->id == MISSING_INT)
    {
        if (!station_can_add)
            throw wreport::error_notfound("station not found in the database");
        last_station->is_new              = true;
        last_station->station_data.loaded = true;
    }
    else
    {
        last_station->is_new              = false;
        last_station->station_data.loaded = false;
    }
    return last_station;
}

batch::Station* Batch::get_station(Tracer<>& trc, const std::string& report,
                                   const Coords& coords, const Ident& ident)
{
    if (have_station(report, coords, ident))
        return last_station;

    v7::Station& st = transaction.station();

    new_station(trc, report, coords, ident);

    last_station->id = st.maybe_get_id(trc, *last_station);
    ++count_select_stations;
    if (last_station->id == MISSING_INT)
    {
        last_station->is_new              = true;
        last_station->station_data.loaded = true;
    }
    else
    {
        last_station->is_new              = false;
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

void Batch::dump(FILE* out) const
{
    fprintf(out, " * Batch wa:%d csst:%u cssd: %u, csd: %u\n", (int)write_attrs,
            count_select_stations, count_select_station_data,
            count_select_data);
    if (last_station)
    {
        fprintf(out, "Cached station:\n");
        last_station->dump(out);
    }
    else
    {
        fprintf(out, "No cached station.\n");
    }
}

namespace batch {

void StationDatum::dump(FILE* out) const
{
    fprintf(out, "%01d%02d%03d(%d): %s\n", WR_VAR_FXY(var->code()),
            (int)(var->code()), var->isset() ? var->enqc() : "(null)");
}

void MeasuredDatum::dump(FILE* out) const
{
    fprintf(out, "ltr:%d %01d%02d%03d(%d): %s\n", id_levtr,
            WR_VAR_FXY(var->code()), (int)(var->code()),
            var->isset() ? var->enqc() : "(null)");
}

void StationData::add(const wreport::Var* var, UpdateMode on_conflict)
{
    if (!loaded)
        throw std::runtime_error(
            "StationData::add called without loading status from DB first");
    auto in_db = ids_by_code.find(var->code());
    if (in_db != ids_by_code.end())
    {
        // Exists in the database
        switch (on_conflict)
        {
            case UPDATE: to_update.emplace_back(in_db->id, var); break;
            case IGNORE: break;
            case ERROR:
                throw wreport::error_consistency(
                    "refusing to overwrite existing data");
        }
    }
    else
    {
        // Does not exist in the database
        to_insert.emplace_back(var);
    }
}

void StationData::write_pending(Tracer<>& trc, Transaction& tr, int station_id,
                                bool with_attrs)
{
    if (!to_insert.empty())
    {
        auto& st = tr.station_data();
        st.insert(trc, station_id, to_insert, with_attrs);
        for (const auto& v : to_insert)
        {
            auto cur = ids_by_code.find(v.var->code());
            if (cur == ids_by_code.end())
                ids_by_code.add(IdVarcode(v.id, v.var->code()));
            else
                cur->id = v.id;
        }
    }
    if (!to_update.empty())
    {
        auto& st = tr.station_data();
        st.update(trc, to_update, with_attrs);
    }
    to_insert.clear();
    to_update.clear();
}

void MeasuredData::add(int id_levtr, const wreport::Var* var,
                       UpdateMode on_conflict)
{
    auto in_db = ids_on_db.find(IdVarcode(id_levtr, var->code()));
    if (in_db != ids_on_db.end())
    {
        // Exists in the database
        switch (on_conflict)
        {
            case UPDATE:
                to_update.emplace_back(in_db->id, id_levtr, var);
                break;
            case IGNORE: break;
            case ERROR:
                throw wreport::error_consistency(
                    "refusing to overwrite existing data");
        }
    }
    else
    {
        // Does not exist in the database
        to_insert.emplace_back(id_levtr, var);
    }
}

void MeasuredData::write_pending(Tracer<>& trc, Transaction& tr, int station_id,
                                 bool with_attrs)
{
    if (!to_insert.empty())
    {
        auto& st = tr.data();
        st.insert(trc, station_id, datetime, to_insert, with_attrs);
        for (const auto& v : to_insert)
        {
            auto cur = ids_on_db.find(IdVarcode(v.id_levtr, v.var->code()));
            if (cur == ids_on_db.end())
                ids_on_db.add(
                    MeasuredDataID(IdVarcode(v.id_levtr, v.var->code()), v.id));
            else
                cur->id = v.id;
        }
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
    for (auto md : items)
        delete md;
}

StationData& Station::get_station_data(Tracer<>& trc)
{
    if (!station_data.loaded)
    {
        v7::StationData& sd = batch.transaction.station_data();
        sd.query(trc, id, [&](int data_id, wreport::Varcode code) {
            station_data.ids_by_code.add(IdVarcode(data_id, code));
        });
        station_data.loaded = true;
        ++batch.count_select_station_data;
    }
    return station_data;
}

MeasuredData& Station::get_measured_data(Tracer<>& trc,
                                         const Datetime& datetime)
{
    if (datetime.is_missing())
        throw std::runtime_error(
            "cannot access measured data with undefined datetime");
    auto mdi = measured_data.find(datetime);
    if (mdi != measured_data.end())
        return **mdi;

    MeasuredData* md = measured_data.add(new MeasuredData(datetime));

    if (!is_new)
    {
        v7::Data& d = batch.transaction.data();
        d.query(trc, id, datetime,
                [&](int data_id, int id_levtr, wreport::Varcode code) {
                    md->ids_on_db.add(
                        MeasuredDataID(IdVarcode(id_levtr, code), data_id));
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
    for (auto md : measured_data)
        md->write_pending(trc, batch.transaction, id, with_attrs);
}

void Station::dump(FILE* out) const
{
    fprintf(out, "Station%s: ", is_new ? " (new)" : "");
    print(out);
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

} // namespace batch

} // namespace v7
} // namespace db
} // namespace dballe
