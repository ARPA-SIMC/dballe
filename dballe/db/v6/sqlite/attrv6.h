#ifndef DBALLE_DB_V6_SQLITE_ATTRV6_H
#define DBALLE_DB_V6_SQLITE_ATTRV6_H

#include <dballe/db/v6/attrv6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v6 {
namespace sqlite {

/**
 * Precompiled queries to manipulate the attr table
 */
class SQLiteAttrV6 : public v6::AttrV6
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
    SQLiteAttrV6(dballe::sql::SQLiteConnection& conn);
    SQLiteAttrV6(const SQLiteAttrV6&) = delete;
    SQLiteAttrV6(const SQLiteAttrV6&&) = delete;
    SQLiteAttrV6& operator=(const SQLiteAttrV6&) = delete;
    ~SQLiteAttrV6();

    void insert(dballe::sql::Transaction& t, v6::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
