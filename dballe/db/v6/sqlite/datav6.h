#ifndef DBALLE_DB_V6_SQLITE_DATA_H
#define DBALLE_DB_V6_SQLITE_DATA_H

#include <dballe/db/v6/datav6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v6 {
namespace sqlite {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class SQLiteDataV6 : public v6::DataV6
{
protected:
    /// DB connection
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled select statement to prepare bulk insert
    dballe::sql::SQLiteStatement* sstm = nullptr;

public:
    SQLiteDataV6(dballe::sql::SQLiteConnection& conn);
    SQLiteDataV6(const SQLiteDataV6&) = delete;
    SQLiteDataV6(const SQLiteDataV6&&) = delete;
    SQLiteDataV6& operator=(const SQLiteDataV6&) = delete;
    ~SQLiteDataV6();

    void insert(dballe::sql::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode=UPDATE) override;
    void remove(const v6::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
