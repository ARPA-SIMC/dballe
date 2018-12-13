#ifndef DBALLE_DB_V7_POSTGRESQL_DATA_H
#define DBALLE_DB_V7_POSTGRESQL_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/db/v7/cache.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {
struct DB;

template<typename Parent>
class PostgreSQLDataCommon : public Parent
{
protected:
    /// DB connection
    dballe::sql::PostgreSQLConnection& conn;
    std::string select_attrs_query_name;
    std::string write_attrs_query_name;
    std::string remove_attrs_query_name;
    std::string remove_data_query_name;

public:
    PostgreSQLDataCommon(v7::Transaction& tr, dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLDataCommon(const PostgreSQLDataCommon&) = delete;
    PostgreSQLDataCommon(const PostgreSQLDataCommon&&) = delete;
    PostgreSQLDataCommon& operator=(const PostgreSQLDataCommon&) = delete;

    void update(Tracer<>& trc, std::vector<typename Parent::BatchValue>& vars, bool with_attrs) override;
    void read_attrs(Tracer<>& trc, int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(Tracer<>& trc, int id_data, const Values& values) override;
    void remove_all_attrs(Tracer<>& trc, int id_data) override;
    void remove(Tracer<>& trc, const v7::IdQueryBuilder& qb) override;
    void remove_by_id(Tracer<>& trc, int id) override;
};

extern template class PostgreSQLDataCommon<StationData>;
extern template class PostgreSQLDataCommon<Data>;


class PostgreSQLStationData : public PostgreSQLDataCommon<StationData>
{
public:
    using PostgreSQLDataCommon::PostgreSQLDataCommon;

    PostgreSQLStationData(v7::Transaction& tr, dballe::sql::PostgreSQLConnection& conn);

    void query(Tracer<>& trc, int id_station, std::function<void(int id, wreport::Varcode code)> dest) override;
    void insert(Tracer<>& trc, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs) override;
    void run_station_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

class PostgreSQLData : public PostgreSQLDataCommon<Data>
{
public:
    using PostgreSQLDataCommon::PostgreSQLDataCommon;

    PostgreSQLData(v7::Transaction& tr, dballe::sql::PostgreSQLConnection& conn);

    void query(Tracer<>& trc, int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) override;
    void insert(Tracer<>& trc, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs) override;
    void run_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void run_summary_query(Tracer<>& trc, const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)>) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

}
}
}
}
#endif
