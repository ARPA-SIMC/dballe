#ifndef DBALLE_DB_V6_ODBC_REPINFO_H
#define DBALLE_DB_V6_ODBC_REPINFO_H

#include <dballe/db/v6/repinfo.h>
#include <dballe/sql/fwd.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
namespace v6 {
namespace odbc {

/**
 * Fast cached access to the repinfo table
 */
struct ODBCRepinfoBase : public v6::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    dballe::sql::ODBCConnection& conn;

    ODBCRepinfoBase(dballe::sql::ODBCConnection& conn);
    ODBCRepinfoBase(const ODBCRepinfoBase&) = delete;
    ODBCRepinfoBase(const ODBCRepinfoBase&&) = delete;
    virtual ~ODBCRepinfoBase();
    ODBCRepinfoBase& operator=(const ODBCRepinfoBase&) = delete;

    void dump(FILE* out) override;

protected:
    int id_use_count(unsigned id, const char* name) override;
    void delete_entry(unsigned id) override;
    void update_entry(const v6::repinfo::Cache& entry) override;
    void insert_entry(const v6::repinfo::Cache& entry) override;

    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

struct ODBCRepinfoV6 : public ODBCRepinfoBase
{
    ODBCRepinfoV6(dballe::sql::ODBCConnection& conn);

protected:
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
}
#endif
