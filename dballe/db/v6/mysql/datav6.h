#ifndef DBALLE_DB_V6_MYSQL_V6_DATA_H
#define DBALLE_DB_V6_MYSQL_V6_DATA_H

#include <dballe/db/v6/datav6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v6 {
namespace mysql {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class MySQLDataV6 : public v6::DataV6
{
protected:
    /** DB connection. */
    dballe::sql::MySQLConnection& conn;

public:
    MySQLDataV6(dballe::sql::MySQLConnection& conn);
    MySQLDataV6(const MySQLDataV6&) = delete;
    MySQLDataV6(const MySQLDataV6&&) = delete;
    MySQLDataV6& operator=(const MySQLDataV6&) = delete;
    ~MySQLDataV6();

    void insert(dballe::sql::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode=UPDATE) override;
    void remove(const v6::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
