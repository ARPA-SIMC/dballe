#ifndef DBALLE_DB_V6_MYSQL_REPINFO_H
#define DBALLE_DB_V6_MYSQL_REPINFO_H

#include <dballe/db/v6/repinfo.h>
#include <dballe/sql/fwd.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
namespace v6 {
namespace mysql {

/**
 * Fast cached access to the repinfo table
 */
struct MySQLRepinfoBase : public v6::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    dballe::sql::MySQLConnection& conn;

    MySQLRepinfoBase(dballe::sql::MySQLConnection& conn);
    MySQLRepinfoBase(const MySQLRepinfoBase&) = delete;
    MySQLRepinfoBase(const MySQLRepinfoBase&&) = delete;
    virtual ~MySQLRepinfoBase();
    MySQLRepinfoBase& operator=(const MySQLRepinfoBase&) = delete;

    void dump(FILE* out) override;

protected:
    void delete_entry(unsigned id) override;
    void update_entry(const v6::repinfo::Cache& entry) override;
    void insert_entry(const v6::repinfo::Cache& entry) override;
    int id_use_count(unsigned id, const char* name) override;
    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

struct MySQLRepinfoV6 : public MySQLRepinfoBase
{
    MySQLRepinfoV6(dballe::sql::MySQLConnection& conn);

protected:
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
}
#endif

