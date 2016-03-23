#ifndef DBALLE_DB_V7_SQLITE_ATTRV7_H
#define DBALLE_DB_V7_SQLITE_ATTRV7_H

#include <dballe/db/v7/attr.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

/**
 * Precompiled queries to manipulate the attr table
 */
class SQLiteAttrV7 : public v7::AttrV7
{
protected:
    /** DB connection. */
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled select statement
    dballe::sql::SQLiteStatement* sstm = nullptr;
    /// Precompiled insert statement
    dballe::sql::SQLiteStatement* istm = nullptr;
    /// Precompiled update statement
    dballe::sql::SQLiteStatement* ustm = nullptr;

public:
    SQLiteAttrV7(dballe::sql::SQLiteConnection& conn);
    SQLiteAttrV7(const SQLiteAttrV7&) = delete;
    SQLiteAttrV7(const SQLiteAttrV7&&) = delete;
    SQLiteAttrV7& operator=(const SQLiteAttrV7&) = delete;
    ~SQLiteAttrV7();

    void insert(dballe::Transaction& t, v7::bulk::InsertAttrsV7& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
