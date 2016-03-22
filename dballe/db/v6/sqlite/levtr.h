#ifndef DBALLE_DB_V6_SQLITE_LEVTRV6_H
#define DBALLE_DB_V6_SQLITE_LEVTRV6_H

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
namespace sqlite {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct SQLiteLevTrV6 : public v6::LevTr
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::SQLiteConnection& conn;

    /** Precompiled select statement */
    dballe::sql::SQLiteStatement* sstm = nullptr;
    /** Precompiled select data statement */
    dballe::sql::SQLiteStatement* sdstm = nullptr;
    /** Precompiled insert statement */
    dballe::sql::SQLiteStatement* istm = nullptr;
    /** Precompiled delete statement */
    dballe::sql::SQLiteStatement* dstm = nullptr;

    DBRow working_row;

public:
    SQLiteLevTrV6(dballe::sql::SQLiteConnection& conn);
    SQLiteLevTrV6(const LevTr&) = delete;
    SQLiteLevTrV6(const LevTr&&) = delete;
    SQLiteLevTrV6& operator=(const SQLiteLevTrV6&) = delete;
    ~SQLiteLevTrV6();

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
