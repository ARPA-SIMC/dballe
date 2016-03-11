#ifndef DBALLE_DB_POSTGRESQL_ATTRV6_H
#define DBALLE_DB_POSTGRESQL_ATTRV6_H

#include <dballe/db/sql/attrv6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace postgresql {

/**
 * Precompiled queries to manipulate the attr table
 */
class PostgreSQLAttrV6 : public sql::AttrV6
{
protected:
    /** DB connection. */
    dballe::sql::PostgreSQLConnection& conn;

public:
    PostgreSQLAttrV6(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLAttrV6(const PostgreSQLAttrV6&) = delete;
    PostgreSQLAttrV6(const PostgreSQLAttrV6&&) = delete;
    PostgreSQLAttrV6& operator=(const PostgreSQLAttrV6&) = delete;
    ~PostgreSQLAttrV6();

    void insert(dballe::sql::Transaction& t, sql::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
#endif
