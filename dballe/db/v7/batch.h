#ifndef DBALLE_DB_V7_BATCH_H
#define DBALLE_DB_V7_BATCH_H

#include <dballe/core/values.h>
#include <dballe/db/v7/fwd.h>
#include <vector>
#include <unordered_map>
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

struct Datum
{
    int data_id = MISSING_INT;
    wreport::Var var;
    std::vector<uint8_t> attrs;
    bool to_update = false;

    Datum(const wreport::Var& var, int data_id)
        : data_id(data_id), var(var) {}
    Datum(const wreport::Var& var)
        : var(var) {}
};

struct StationData
{
    std::unordered_map<wreport::Varcode, int> ids_by_code;
    std::vector<Datum> data;
    bool loaded = false;

    void add(const wreport::Var& var, bool overwrite=false, bool with_attrs=false);
};

struct MeasuredData
{
    Datetime datetime;
    std::vector<Datum> data;
    bool loaded = false;

    void add(int id_levtr, const wreport::Var& var, bool overwrite=false, bool with_attrs=false);
};

struct Station : public dballe::Station
{
    std::shared_ptr<Transaction> transaction;
    bool is_new = true;
    StationData station_data;

    using dballe::Station::Station;

    StationData& get_station_data();
    MeasuredData& get_measured_data(const Datetime& datetime);
};

}
}
}
}

#endif
