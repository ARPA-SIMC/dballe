#ifndef DBALLE_DB_V6_MYSQL_LEVTRV6_H
#define DBALLE_DB_V6_MYSQL_LEVTRV6_H

#include <dballe/db/db.h>
#include <dballe/db/v6/levtr.h>
#include <dballe/sql/fwd.h>
#include <cstdio>
#include <memory>

namespace dballe {
struct Record;
struct Msg;

namespace msg {
struct Context;
}

namespace db {
namespace v6 {
namespace mysql {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct MySQLLevTrV6 : public v6::LevTr
{
protected:
    /// DB connection.
    dballe::sql::MySQLConnection& conn;

    DBRow working_row;

public:
    MySQLLevTrV6(dballe::sql::MySQLConnection& conn);
    MySQLLevTrV6(const LevTr&) = delete;
    MySQLLevTrV6(const LevTr&&) = delete;
    MySQLLevTrV6& operator=(const MySQLLevTrV6&) = delete;
    ~MySQLLevTrV6();

    /**
     * Return the ID for the given Level and Trange, adding it to the database
     * if it does not already exist
     */
    int obtain_id(const Level& lev, const Trange& tr) override;

    const DBRow* read(int id) override;
    void read_all(std::function<void(const DBRow&)> dest) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

}
}
}
}
#endif

