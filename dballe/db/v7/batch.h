#ifndef DBALLE_DB_V7_BATCH_H
#define DBALLE_DB_V7_BATCH_H

#include <dballe/core/values.h>
#include <dballe/core/smallset.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/db/v7/utils.h>
#include <vector>
#include <tuple>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
struct Transaction;

class Batch
{
protected:
    bool write_attrs = true;
    batch::Station* last_station = nullptr;

    bool have_station(const std::string& report, const Coords& coords, const Ident& ident);
    void new_station(Tracer<>& trc, const std::string& report, const Coords& coords, const Ident& ident);

public:
    Transaction& transaction;
    unsigned count_select_stations = 0;
    unsigned count_select_station_data = 0;
    unsigned count_select_data = 0;

    Batch(Transaction& transaction) : transaction(transaction) {}
    ~Batch();

    void set_write_attrs(bool write_attrs);

    batch::Station* get_station(Tracer<>& trc, const dballe::Station& station, bool station_can_add);
    batch::Station* get_station(Tracer<>& trc, const std::string& report, const Coords& coords, const Ident& ident);

    void write_pending(Tracer<>& trc);
    void clear();
};


namespace batch {

enum UpdateMode {
    UPDATE,
    IGNORE,
    ERROR,
};

struct StationDatum
{
    int id = MISSING_INT;
    const wreport::Var* var;

    StationDatum(const wreport::Var* var)
        : var(var) {}
    StationDatum(int id, const wreport::Var* var)
        : id(id), var(var) {}

    void dump(FILE* out) const;
    bool operator<(const StationDatum& o) const { return var->code() < o.var->code(); }
    bool operator==(const StationDatum& o) const { return var->code() == o.var->code(); }
};

struct StationDataIDs : public core::SmallSet<StationDataIDs, IdVarcode, wreport::Varcode>
{
    static const wreport::Varcode& _smallset_get_value(const IdVarcode& item)
    {
        return item.varcode;
    }
};

struct StationData
{
    StationDataIDs ids_by_code;
    std::vector<StationDatum> to_insert;
    std::vector<StationDatum> to_update;
    bool loaded = false;

    void add(const wreport::Var* var, UpdateMode on_conflict);
    void write_pending(Tracer<>& trc, Transaction& tr, int station_id, bool with_attrs);
};

struct MeasuredDatum
{
    int id = MISSING_INT;
    int id_levtr;
    const wreport::Var* var;

    MeasuredDatum(int id_levtr, const wreport::Var* var)
        : id_levtr(id_levtr), var(var) {}
    MeasuredDatum(int id, int id_levtr, const wreport::Var* var)
        : id(id), id_levtr(id_levtr), var(var) {}

    void dump(FILE* out) const;
    bool operator<(const MeasuredDatum& o) const { return id_levtr < o.id_levtr || (id_levtr == o.id_levtr && var->code() < o.var->code()); }
    bool operator==(const MeasuredDatum& o) const { return id_levtr == o.id_levtr && var->code() == o.var->code(); }
};

struct MeasuredDataID
{
    IdVarcode id_varcode;
    int id;

    MeasuredDataID(IdVarcode id_varcode, int id)
        : id_varcode(id_varcode), id(id)
    {
    }
};

struct MeasuredDataIDs : public core::SmallSet<MeasuredDataIDs, MeasuredDataID, IdVarcode>
{
    static const IdVarcode& _smallset_get_value(const MeasuredDataID& item)
    {
        return item.id_varcode;
    }
};

struct MeasuredData
{
    Datetime datetime;
    MeasuredDataIDs ids_on_db;
    std::vector<MeasuredDatum> to_insert;
    std::vector<MeasuredDatum> to_update;

    MeasuredData(Datetime datetime)
        : datetime(datetime)
    {
    }

    void add(int id_levtr, const wreport::Var* var, UpdateMode on_conflict);
    void write_pending(Tracer<>& trc, Transaction& tr, int station_id, bool with_attrs);
};

struct MeasuredDataVector : public core::SmallSet<MeasuredDataVector, MeasuredData*, Datetime>
{
    MeasuredDataVector() {}
    MeasuredDataVector(const MeasuredDataVector&) = delete;
    MeasuredDataVector(MeasuredDataVector&&) = default;
    ~MeasuredDataVector();
    MeasuredDataVector& operator=(const MeasuredDataVector&) = delete;
    MeasuredDataVector& operator=(MeasuredDataVector&&) = default;

    static const Datetime& _smallset_get_value(const MeasuredData* md) { return md->datetime; }
};

struct Station : public dballe::Station
{
    Batch& batch;
    bool is_new = true;
    StationData station_data;
    MeasuredDataVector measured_data;

    Station(Batch& batch)
        : batch(batch) {}

    StationData& get_station_data(Tracer<>& trc);
    MeasuredData& get_measured_data(Tracer<>& trc, const Datetime& datetime);

    void write_pending(Tracer<>& trc, bool with_attrs);
};

}
}
}
}

#endif
