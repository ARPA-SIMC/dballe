#ifndef DBALLE_DB_V7_MYSQL_DATA_H
#define DBALLE_DB_V7_MYSQL_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/db/v7/cache.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace mysql {
struct DB;

template<typename Traits>
class MySQLDataCommon : public DataCommon<Traits>
{
protected:
    /// DB connection
    dballe::sql::MySQLConnection& conn;

#if 0
    /// Precompiled read attributes statement
    dballe::sql::MySQLStatement* read_attrs_stm = nullptr;
    /// Precompiled write attributes statement
    dballe::sql::MySQLStatement* write_attrs_stm = nullptr;
    /// Precompiled remove attributes statement
    dballe::sql::MySQLStatement* remove_attrs_stm = nullptr;
    /// Precompiled select statement
    dballe::sql::MySQLStatement* sstm = nullptr;
    /// Precompiled insert statement
    dballe::sql::MySQLStatement* istm = nullptr;
    /// Precompiled update statement
    dballe::sql::MySQLStatement* ustm = nullptr;
#endif

public:
    MySQLDataCommon(dballe::sql::MySQLConnection& conn);
    MySQLDataCommon(const MySQLDataCommon&) = delete;
    MySQLDataCommon(const MySQLDataCommon&&) = delete;
    MySQLDataCommon& operator=(const MySQLDataCommon&) = delete;
    ~MySQLDataCommon();

    void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(int id_data, const Values& values) override;
    void remove_all_attrs(int id_data) override;
    void remove(const v7::IdQueryBuilder& qb) override;
};

extern template class MySQLDataCommon<StationDataTraits>;
extern template class MySQLDataCommon<DataTraits>;

/**
 * Precompiled query to manipulate the station data table
 */
class MySQLStationData : public MySQLDataCommon<StationDataTraits>
{
public:
    using MySQLDataCommon::MySQLDataCommon;
    StationValueCache cache;

    MySQLStationData(dballe::sql::MySQLConnection& conn);
    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override { return cache.clear(); }
};

/**
 * Precompiled query to manipulate the data table
 */
class MySQLData : public MySQLDataCommon<DataTraits>
{
public:
    using MySQLDataCommon::MySQLDataCommon;
    ValueCache cache;

    MySQLData(dballe::sql::MySQLConnection& conn);
    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override { return cache.clear(); }
};

}
}
}
}
#endif
