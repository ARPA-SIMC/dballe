#ifndef DBALLE_DB_V6_ODBC_DATAV6_H
#define DBALLE_DB_V6_ODBC_DATAV6_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/v6/datav6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v6 {
namespace odbc {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class ODBCDataV6 : public v6::DataV6
{
protected:
    /** DB connection. */
    dballe::sql::ODBCConnection& conn;

public:
    ODBCDataV6(dballe::sql::ODBCConnection& conn);
    ODBCDataV6(const ODBCDataV6&) = delete;
    ODBCDataV6(const ODBCDataV6&&) = delete;
    ODBCDataV6& operator=(const ODBCDataV6&) = delete;
    ~ODBCDataV6();

    void insert(dballe::sql::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode=UPDATE) override;
    void remove(const v6::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
