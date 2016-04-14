#ifndef DBALLE_DB_V7_MYSQL_REPINFO_H
#define DBALLE_DB_V7_MYSQL_REPINFO_H

#include <dballe/db/v7/repinfo.h>
#include <dballe/sql/fwd.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace mysql {

/**
 * Fast cached access to the repinfo table
 */
struct MySQLRepinfoV7 : public v7::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    dballe::sql::MySQLConnection& conn;

    MySQLRepinfoV7(dballe::sql::MySQLConnection& conn);
    MySQLRepinfoV7(const MySQLRepinfoV7&) = delete;
    MySQLRepinfoV7(const MySQLRepinfoV7&&) = delete;
    virtual ~MySQLRepinfoV7();
    MySQLRepinfoV7& operator=(const MySQLRepinfoV7&) = delete;

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

}
}
}
}
#endif
