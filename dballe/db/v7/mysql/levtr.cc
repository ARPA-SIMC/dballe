#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/mysql.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/trace.h"
#include <map>
#include <sstream>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::mysql::Row;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

namespace {

Level to_level(Row& row, int first_id=0)
{
    return Level(
            row.as_int(first_id),
            row.as_int(first_id + 1),
            row.as_int(first_id + 2),
            row.as_int(first_id + 3));
}

Trange to_trange(Row& row, int first_id=0)
{
    return Trange(
            row.as_int(first_id),
            row.as_int(first_id + 1),
            row.as_int(first_id + 2));
}

}

MySQLLevTr::MySQLLevTr(v7::Transaction& tr, MySQLConnection& conn)
    : v7::LevTr(tr), conn(conn)
{
}

MySQLLevTr::~MySQLLevTr()
{
}

void MySQLLevTr::prefetch_ids(const std::set<int>& ids)
{
    if (ids.empty()) return;

    sql::Querybuf qb;
    if (ids.size() < 100)
    {
        qb.append("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE id IN (");
        qb.start_list(",");
        for (auto id: ids)
            qb.append_listf("%d", id);
        qb.append(")");
    } else
        qb.append("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr");

    auto res = conn.exec_store(qb);
    if (tr.trace) tr.trace->trace_select(qb);
    while (auto row = res.fetch())
    {
        if (tr.trace) tr.trace->trace_select_row();
        cache.insert(unique_ptr<LevTrEntry>(new LevTrEntry(
            row.as_int(0),
            Level(row.as_int(1), row.as_int(2), row.as_int(3), row.as_int(4)),
            Trange(row.as_int(5), row.as_int(6), row.as_int(7)))));
    }
}

const LevTrEntry* MySQLLevTr::lookup_id(int id)
{
    const LevTrEntry* res = cache.find_entry(id);
    if (res) return res;

    char query[128];
    snprintf(query, 128, "SELECT ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE id=%d", id);

    auto qres = conn.exec_store(query);
    if (tr.trace) tr.trace->trace_select(query);
    while (auto row = qres.fetch())
    {
        if (tr.trace) tr.trace->trace_select_row();
        std::unique_ptr<LevTrEntry> e(new LevTrEntry);
        e->id = id;
        e->level.ltype1 = row.as_int(0);
        e->level.l1 = row.as_int(1);
        e->level.ltype2 = row.as_int(2);
        e->level.l2 = row.as_int(3);
        e->trange.pind = row.as_int(4);
        e->trange.p1 = row.as_int(5);
        e->trange.p2 = row.as_int(6);
        res = cache.insert(move(e));
    }

    if (!res)
        error_notfound::throwf("levtr with id %d not found in the database", id);

    return res;
}

int MySQLLevTr::obtain_id(const LevTrEntry& desc)
{
    int id = cache.find_id(desc);
    if (id != MISSING_INT) return id;

    char query[512];
    snprintf(query, 512, R"(
        SELECT id FROM levtr WHERE
             ltype1=%d AND l1=%d AND ltype2=%d AND l2=%d
         AND pind=%d AND p1=%d AND p2=%d
    )", desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);

    // If there is an existing record, use its ID and don't do an INSERT
    if (tr.trace) tr.trace->trace_select(query);
    auto qres = conn.exec_store(query);
    while (auto row = qres.fetch())
    {
        if (tr.trace) tr.trace->trace_select_row();
        id = row.as_int(0);
    }
    if (id != MISSING_INT)
    {
        cache.insert(desc, id);
        return id;
    }

    // Not found in the database, insert a new one
    snprintf(query, 512, "INSERT INTO levtr (ltype1, l1, ltype2, l2, pind, p1, p2) VALUES (%d, %d, %d, %d, %d, %d, %d)",
            desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);
    if (tr.trace) tr.trace->trace_insert(query, 1);
    conn.exec_no_data(query);
    id = conn.get_last_insert_id();
    cache.insert(desc, id);
    return id;
}

void MySQLLevTr::_dump(std::function<void(int, const Level&, const Trange&)> out)
{
    auto res = conn.exec_store("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr ORDER BY ID");
    while (auto row = res.fetch())
        out(row.as_int(0), to_level(row, 1), to_trange(row, 5));
}

}
}
}
}
