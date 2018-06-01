#ifndef DBALLE_DB_V7_SQLITE_DATA_H
#define DBALLE_DB_V7_SQLITE_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/db/v7/cache.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace sqlite {
struct DB;

template<typename Traits>
class SQLiteDataCommon : public DataCommon<Traits>
{
protected:
    /// DB connection
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled read attributes statement
    dballe::sql::SQLiteStatement* read_attrs_stm = nullptr;
    /// Precompiled write attributes statement
    dballe::sql::SQLiteStatement* write_attrs_stm = nullptr;
    /// Precompiled remove attributes statement
    dballe::sql::SQLiteStatement* remove_attrs_stm = nullptr;
    /// Precompiled select statement
    dballe::sql::SQLiteStatement* sstm = nullptr;
    /// Precompiled insert statement
    dballe::sql::SQLiteStatement* istm = nullptr;
    /// Precompiled update statement
    dballe::sql::SQLiteStatement* ustm = nullptr;

public:
    SQLiteDataCommon(dballe::sql::SQLiteConnection& conn);
    SQLiteDataCommon(const SQLiteDataCommon&) = delete;
    SQLiteDataCommon(const SQLiteDataCommon&&) = delete;
    SQLiteDataCommon& operator=(const SQLiteDataCommon&) = delete;
    ~SQLiteDataCommon();

    void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(int id_data, const Values& values) override;
    void remove_all_attrs(int id_data) override;
    void remove(const v7::IdQueryBuilder& qb) override;
};

extern template class SQLiteDataCommon<StationDataTraits>;
extern template class SQLiteDataCommon<DataTraits>;

/**
 * Precompiled query to manipulate the station data table
 */
class SQLiteStationData : public SQLiteDataCommon<StationDataTraits>
{
public:
    using SQLiteDataCommon::SQLiteDataCommon;

    SQLiteStationData(dballe::sql::SQLiteConnection& conn);

    void query(const int& query, std::function<void(int id, wreport::Varcode code)> dest) override;
    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

/**
 * Precompiled query to manipulate the data table
 */
class SQLiteData : public SQLiteDataCommon<DataTraits>
{
public:
    using SQLiteDataCommon::SQLiteDataCommon;

    SQLiteData(dballe::sql::SQLiteConnection& conn);

    void query(const std::pair<int, Datetime>& query, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) override;
    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

}
}
}
}
#endif
