#ifndef DBALLE_DB_V7_SQLITE_LEVTRV7_H
#define DBALLE_DB_V7_SQLITE_LEVTRV7_H

#include <dballe/db/db.h>
#include <dballe/db/v7/levtr.h>
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
namespace v7 {
namespace sqlite {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct SQLiteLevTr : public v7::LevTr
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

    void _dump(std::function<void(int, const Level&, const Trange&)> out) override;

public:
    SQLiteLevTr(dballe::sql::SQLiteConnection& conn);
    SQLiteLevTr(const LevTr&) = delete;
    SQLiteLevTr(const LevTr&&) = delete;
    SQLiteLevTr& operator=(const SQLiteLevTr&) = delete;
    ~SQLiteLevTr();

    void prefetch_ids(const std::set<int>& ids, std::function<void(int, const LevTrDesc&)> dest) override;
    void prefetch_same_level(int id, std::function<void(int, const LevTrDesc&)> dest) override;
    levtrs_t::iterator lookup_id(State& st, int id) override;
    levtrs_t::iterator obtain_id(State& state, const LevTrDesc& desc) override;
};


}
}
}
}
#endif
