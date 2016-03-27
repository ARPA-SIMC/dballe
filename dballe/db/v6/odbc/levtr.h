#ifndef DBALLE_DB_V6_ODBC_LEV_TR_H
#define DBALLE_DB_V6_ODBC_LEV_TR_H

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
namespace odbc {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct ODBCLevTrV6 : public v6::LevTr
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::ODBCConnection& conn;

    /// lev_tr ID sequence, for databases that need it
    dballe::sql::Sequence* seq_lev_tr = nullptr;

    /** Precompiled select statement */
    dballe::sql::ODBCStatement* sstm = nullptr;
    /** Precompiled select data statement */
    dballe::sql::ODBCStatement* sdstm = nullptr;
    /** Precompiled insert statement */
    dballe::sql::ODBCStatement* istm = nullptr;
    /** Precompiled delete statement */
    dballe::sql::ODBCStatement* dstm = nullptr;

    DBRow working_row;

    /**
     * Insert a new lev_tr in the database
     *
     * @return
     *   The ID of the newly inserted lev_tr
     */
    int insert();

    /**
     * Get the lev_tr id for the current lev_tr data.
     *
     * @return
     *   The database ID, or -1 if no existing lev_tr entry matches the given values
     */
    int get_id();

    /**
     * Remove a lev_tr record
     */
    void remove();

public:
    ODBCLevTrV6(dballe::sql::ODBCConnection& conn);
    ODBCLevTrV6(const LevTr&) = delete;
    ODBCLevTrV6(const LevTr&&) = delete;
    ODBCLevTrV6& operator=(const ODBCLevTrV6&) = delete;
    ~ODBCLevTrV6();

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
