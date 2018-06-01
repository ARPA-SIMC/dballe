#ifndef DBALLE_DB_V7_BATCH_H
#define DBALLE_DB_V7_BATCH_H

#include <dballe/core/values.h>
#include <dballe/db/v7/fwd.h>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
struct Transaction;

struct Batch
{
    std::shared_ptr<Transaction> transaction;

    Batch(std::shared_ptr<Transaction> transaction) : transaction(transaction) {}

    std::shared_ptr<batch::Station> get_station(const std::string& report, const Coords& coords);
    std::shared_ptr<batch::Station> get_station(const std::string& report, const Coords& coords, const std::string& ident);

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

struct Data
{
    std::vector<Datum> data;

    void add(const wreport::Var& var, bool overwrite=false, bool with_attrs=false);

};

struct MeasuredData : public Data
{
    int id_levtr;
    Datetime datetime;
};

struct Station : public dballe::Station
{
    bool is_new = true;
    Data station_data;

    using dballe::Station::Station;
    MeasuredData& get_measured_data(int id_levtr, const Datetime& datetime);
};

}
}
}
}

#endif
