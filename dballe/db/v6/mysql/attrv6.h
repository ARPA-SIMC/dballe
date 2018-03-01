#ifndef DBALLE_DB_V6_MYSQL_ATTRV6_H
#define DBALLE_DB_V6_MYSQL_ATTRV6_H

#include <dballe/db/v6/attrv6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

/**
 * Precompiled queries to manipulate the attr table
 */
class MySQLAttrV6 : public v6::AttrV6
{
protected:
    /** DB connection. */
    dballe::sql::MySQLConnection& conn;

public:
    MySQLAttrV6(dballe::sql::MySQLConnection& conn);
    MySQLAttrV6(const MySQLAttrV6&) = delete;
    MySQLAttrV6(const MySQLAttrV6&&) = delete;
    MySQLAttrV6& operator=(const MySQLAttrV6&) = delete;
    ~MySQLAttrV6();

    void insert(dballe::db::Transaction& t, v6::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
}
#endif
