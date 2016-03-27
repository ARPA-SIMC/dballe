#ifndef DBALLE_DB_V7_POSTGRESQL_ATTRV7_H
#define DBALLE_DB_V7_POSTGRESQL_ATTRV7_H

#include <dballe/db/v7/attr.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

/**
 * Precompiled queries to manipulate the attr table
 */
class PostgreSQLAttr : public v7::Attr
{
protected:
    /** DB connection. */
    dballe::sql::PostgreSQLConnection& conn;

    void _dump(std::function<void(int, wreport::Varcode, const char*)> out) override;

public:
    PostgreSQLAttr(dballe::sql::PostgreSQLConnection& conn, const std::string& table_name, std::unordered_set<int> State::* new_ids);
    PostgreSQLAttr(const PostgreSQLAttr&) = delete;
    PostgreSQLAttr(const PostgreSQLAttr&&) = delete;
    PostgreSQLAttr& operator=(const PostgreSQLAttr&) = delete;
    ~PostgreSQLAttr();

    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertAttrsV7& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
};

}
}
}
}
#endif
