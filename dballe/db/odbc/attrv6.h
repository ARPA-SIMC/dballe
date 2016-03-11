#ifndef DBALLE_DB_ODBC_ATTRV6_H
#define DBALLE_DB_ODBC_ATTRV6_H

#include <dballe/db/sql/attrv6.h>
#include <dballe/sql/fwd.h>
#include <sqltypes.h>

namespace dballe {
namespace db {
namespace odbc {

/**
 * Precompiled queries to manipulate the attr table
 */
class ODBCAttrV6 : public sql::AttrV6
{
protected:
    /** DB connection. */
    dballe::sql::ODBCConnection& conn;

    /** Precompiled select statement */
    dballe::sql::ODBCStatement* sstm;

    /** id_data SQL parameter */
    int id_data;
    /** attribute id SQL parameter */
    wreport::Varcode type;
    /** attribute value SQL parameter */
    char value[255];
    /** attribute value indicator */
    SQLLEN value_ind;

public:
    ODBCAttrV6(dballe::sql::ODBCConnection& conn);
    ~ODBCAttrV6();

    void insert(dballe::sql::Transaction& t, sql::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;

private:
    // disallow copy
    ODBCAttrV6(const ODBCAttrV6&);
    ODBCAttrV6& operator=(const ODBCAttrV6&);
};

}
}
}
#endif
