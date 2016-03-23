#ifndef DBALLE_DB_V7_SQLITE_DATA_H
#define DBALLE_DB_V7_SQLITE_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace sqlite {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class SQLiteDataV7 : public v7::DataV7
{
protected:
    /// DB connection
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled select statement to prepare bulk insert
    dballe::sql::SQLiteStatement* sstm = nullptr;

public:
    SQLiteDataV7(dballe::sql::SQLiteConnection& conn);
    SQLiteDataV7(const SQLiteDataV7&) = delete;
    SQLiteDataV7(const SQLiteDataV7&&) = delete;
    SQLiteDataV7& operator=(const SQLiteDataV7&) = delete;
    ~SQLiteDataV7();

    void insert(dballe::Transaction& t, v7::bulk::InsertV7& vars, UpdateMode update_mode=UPDATE) override;
    void remove(const v7::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
