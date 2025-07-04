#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"
#include <cstring>
#include <map>
#include <sstream>

using namespace wreport;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::Querybuf;
using dballe::sql::mysql::Row;

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

namespace {

Level to_level(Row& row, int first_id = 0)
{
    return Level(row.as_int(first_id), row.as_int(first_id + 1),
                 row.as_int(first_id + 2), row.as_int(first_id + 3));
}

Trange to_trange(Row& row, int first_id = 0)
{
    return Trange(row.as_int(first_id), row.as_int(first_id + 1),
                  row.as_int(first_id + 2));
}

} // namespace

MySQLLevTr::MySQLLevTr(v7::Transaction& tr, MySQLConnection& conn)
    : v7::LevTr(tr), conn(conn)
{
}

MySQLLevTr::~MySQLLevTr() {}

void MySQLLevTr::prefetch_ids(Tracer<>& trc, const std::set<int>& ids)
{
    if (ids.empty())
        return;

    sql::Querybuf qb;
    if (ids.size() < 100)
    {
        qb.append("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr "
                  "WHERE id IN (");
        qb.start_list(",");
        for (auto id : ids)
            qb.append_listf("%d", id);
        qb.append(")");
    }
    else
        qb.append("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr");

    Tracer<> trc_sel(trc ? trc->trace_select(qb) : nullptr);
    auto res = conn.exec_store(qb);
    while (auto row = res.fetch())
    {
        if (trc_sel)
            trc_sel->add_row();
        cache.insert(unique_ptr<LevTrEntry>(new LevTrEntry(
            row.as_int(0),
            Level(row.as_int(1), row.as_int(2), row.as_int(3), row.as_int(4)),
            Trange(row.as_int(5), row.as_int(6), row.as_int(7)))));
    }
}

const LevTrEntry* MySQLLevTr::lookup_id(Tracer<>& trc, int id)
{
    const LevTrEntry* res = cache.find_entry(id);
    if (res)
        return res;

    char query[128];
    snprintf(
        query, 128,
        "SELECT ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE id=%d",
        id);

    Tracer<> trc_sel(trc ? trc->trace_select(query) : nullptr);
    auto qres = conn.exec_store(query);
    while (auto row = qres.fetch())
    {
        if (trc_sel)
            trc_sel->add_row();
        std::unique_ptr<LevTrEntry> e(new LevTrEntry);
        e->id           = id;
        e->level.ltype1 = row.as_int(0);
        e->level.l1     = row.as_int(1);
        e->level.ltype2 = row.as_int(2);
        e->level.l2     = row.as_int(3);
        e->trange.pind  = row.as_int(4);
        e->trange.p1    = row.as_int(5);
        e->trange.p2    = row.as_int(6);
        res             = cache.insert(move(e));
    }

    if (!res)
        error_notfound::throwf("levtr with id %d not found in the database",
                               id);

    return res;
}

int MySQLLevTr::obtain_id(Tracer<>& trc, const LevTrEntry& desc)
{
    int id = cache.find_id(desc);
    if (id != MISSING_INT)
        return id;

    char query[512];
    snprintf(query, 512, R"(
        SELECT id FROM levtr WHERE
             ltype1=%d AND l1=%d AND ltype2=%d AND l2=%d
         AND pind=%d AND p1=%d AND p2=%d
    )",
             desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
             desc.trange.pind, desc.trange.p1, desc.trange.p2);

    // If there is an existing record, use its ID and don't do an INSERT
    Tracer<> trc_oid(trc ? trc->trace_select(query) : nullptr);
    auto qres = conn.exec_store(query);
    while (auto row = qres.fetch())
    {
        if (trc_oid)
            trc_oid->add_row();
        id = row.as_int(0);
    }
    if (id != MISSING_INT)
    {
        cache.insert(desc, id);
        return id;
    }

    // Not found in the database, insert a new one
    snprintf(query, 512,
             "INSERT INTO levtr (ltype1, l1, ltype2, l2, pind, p1, p2) VALUES "
             "(%d, %d, %d, %d, %d, %d, %d)",
             desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
             desc.trange.pind, desc.trange.p1, desc.trange.p2);
    trc_oid.reset(trc ? trc->trace_insert(query, 1) : nullptr);
    conn.exec_no_data(query);
    id = conn.get_last_insert_id();
    cache.insert(desc, id);
    return id;
}

void MySQLLevTr::_dump(
    std::function<void(int, const Level&, const Trange&)> out)
{
    auto res = conn.exec_store("SELECT id, ltype1, l1, ltype2, l2, pind, p1, "
                               "p2 FROM levtr ORDER BY ID");
    while (auto row = res.fetch())
        out(row.as_int(0), to_level(row, 1), to_trange(row, 5));
}

} // namespace mysql
} // namespace v7
} // namespace db
} // namespace dballe
