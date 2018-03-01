#ifndef DBALLE_DB_V6_POSTGRESQL_DATAV6_H
#define DBALLE_DB_V6_POSTGRESQL_DATAV6_H

#include <dballe/db/v6/datav6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v6 {
namespace postgresql {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class PostgreSQLDataV6 : public v6::DataV6
{
protected:
    /** DB connection. */
    dballe::sql::PostgreSQLConnection& conn;

public:
    PostgreSQLDataV6(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLDataV6(const PostgreSQLDataV6&) = delete;
    PostgreSQLDataV6(const PostgreSQLDataV6&&) = delete;
    PostgreSQLDataV6& operator=(const PostgreSQLDataV6&) = delete;
    ~PostgreSQLDataV6();

    void insert(dballe::db::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode=UPDATE) override;
    void remove(const v6::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
