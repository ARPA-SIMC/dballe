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
class SQLiteAttr : public v7::Attr
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
    SQLiteAttr(dballe::sql::SQLiteConnection& conn);
    SQLiteAttr(const SQLiteAttr&) = delete;
    SQLiteAttr(const SQLiteAttr&&) = delete;
    SQLiteAttr& operator=(const SQLiteAttr&) = delete;
    ~SQLiteAttr();

    void insert(dballe::Transaction& t, v7::bulk::InsertAttrsV7& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
