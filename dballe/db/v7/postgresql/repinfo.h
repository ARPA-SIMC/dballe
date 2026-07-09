#ifndef DBALLE_DB_V7_POSTGRESQL_REPINFO_H
#define DBALLE_DB_V7_POSTGRESQL_REPINFO_H

#include <dballe/db/v7/repinfo.h>
#include <dballe/sql/fwd.h>
#include <map>
#include <string>
#include <vector>

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

/**
 * Fast cached access to the repinfo table
 */
struct PostgreSQLRepinfo : public v7::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    dballe::sql::PostgreSQLConnection& conn;

    PostgreSQLRepinfo(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLRepinfo(const PostgreSQLRepinfo&)  = delete;
    PostgreSQLRepinfo(const PostgreSQLRepinfo&&) = delete;
    virtual ~PostgreSQLRepinfo();
    PostgreSQLRepinfo& operator=(const PostgreSQLRepinfo&) = delete;

    void dump(FILE* out) override;

protected:
    /// Return how many time this ID is used in the database
    int id_use_count(unsigned id, const char* name) override;
    void delete_entry(unsigned id) override;
    void update_entry(const v7::repinfo::Cache& entry) override;
    void insert_entry(const v7::repinfo::Cache& entry) override;
    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

} // namespace postgresql
} // namespace v7
} // namespace db
} // namespace dballe
#endif
