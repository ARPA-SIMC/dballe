#ifndef DBALLE_DB_V7_MYSQL_LEVTRV7_H
#define DBALLE_DB_V7_MYSQL_LEVTRV7_H

#include <dballe/db/db.h>
#include <dballe/db/v7/levtr.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/sql/fwd.h>
#include <cstdio>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct MySQLLevTr : public v7::LevTr
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::MySQLConnection& conn;

    void _dump(std::function<void(int, const Level&, const Trange&)> out) override;

public:
    MySQLLevTr(v7::Transaction& tr, dballe::sql::MySQLConnection& conn);
    MySQLLevTr(const LevTr&) = delete;
    MySQLLevTr(const LevTr&&) = delete;
    MySQLLevTr& operator=(const MySQLLevTr&) = delete;
    ~MySQLLevTr();

    void prefetch_ids(Tracer<>& trc, const std::set<int>& ids) override;
    const LevTrEntry* lookup_id(Tracer<>& trc, int id) override;
    int obtain_id(Tracer<>& trc, const LevTrEntry& desc) override;
};


}
}
}
}
#endif
