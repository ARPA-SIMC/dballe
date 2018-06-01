#ifndef DBALLE_DB_V7_BATCH_H
#define DBALLE_DB_V7_BATCH_H

#include <dballe/core/values.h>
#include <dballe/db/v7/fwd.h>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
struct Transaction;

class Batch
{
protected:
    std::shared_ptr<Transaction> transaction;
    std::vector<batch::Station> stations;
    std::unordered_map<int, std::vector<size_t>> stations_by_lon;

    batch::Station* find_existing(const std::string& report, const Coords& coords, const Ident& ident);
    void index_existing(batch::Station* st, size_t pos);

public:
    Batch(std::shared_ptr<Transaction> transaction) : transaction(transaction) {}

    batch::Station* get_station(const std::string& report, const Coords& coords, const Ident& ident);

    void commit();
};


namespace batch {

struct StationDatumInsert
{
    wreport::Var var;
    std::vector<uint8_t> attrs;

    StationDatumInsert(const wreport::Var& var)
        : var(var) {}
};

struct StationDatumUpdate
{
    int id;
    wreport::Var var;
    std::vector<uint8_t> attrs;

    StationDatumUpdate(int id, const wreport::Var& var)
        : id(id), var(var) {}
};

struct StationData
{
    std::unordered_map<wreport::Varcode, int> ids_by_code;
    std::vector<StationDatumInsert> to_insert;
    std::vector<StationDatumUpdate> to_update;
    bool loaded = false;

    void add(const wreport::Var& var, bool overwrite=false, bool with_attrs=false);
    void commit(Transaction& tr, int station_id);
};


struct MeasuredDatumInsert
{
    int id_levtr;
    wreport::Var var;
    std::vector<uint8_t> attrs;

    MeasuredDatumInsert(int id_levtr, const wreport::Var& var)
        : id_levtr(id_levtr), var(var) {}
};

struct MeasuredDatumUpdate
{
    int id;
    int id_levtr;
    wreport::Var var;
    std::vector<uint8_t> attrs;

    MeasuredDatumUpdate(int id, int id_levtr, const wreport::Var& var)
        : id(id), id_levtr(id_levtr), var(var) {}
};

struct IdVarcode
{
    int id;
    wreport::Varcode varcode;

    IdVarcode(int id, wreport::Varcode varcode)
        : id(id), varcode(varcode)
    {
    }

    bool operator==(const IdVarcode& o) const { return o.id == id && o.varcode == varcode; }
};

} } } }

namespace std
{
    template<> struct hash<dballe::db::v7::batch::IdVarcode>
    {
        typedef dballe::db::v7::batch::IdVarcode argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const noexcept
        {
            result_type const h1 ( std::hash<int>{}(s.id) );
            result_type const h2 ( std::hash<wreport::Varcode>{}(s.varcode) );
            return h1 ^ (h2 << 1);
        }
    };
}

namespace dballe { namespace db { namespace v7 { namespace batch {

struct MeasuredData
{
    Datetime datetime;
    std::unordered_map<IdVarcode, int> ids_on_db;
    std::vector<MeasuredDatumInsert> to_insert;
    std::vector<MeasuredDatumUpdate> to_update;

    MeasuredData(Datetime datetime)
        : datetime(datetime)
    {
    }

    void add(int id_levtr, const wreport::Var& var, bool overwrite=false, bool with_attrs=false);
    void commit(Transaction& tr, int station_id);
};

struct Station : public dballe::Station
{
    std::shared_ptr<Transaction> transaction;
    bool is_new = true;
    StationData station_data;
    std::map<Datetime, MeasuredData> measured_data;

    using dballe::Station::Station;

    StationData& get_station_data();
    MeasuredData& get_measured_data(const Datetime& datetime);

    void commit();
};

}
}
}
}

#endif
