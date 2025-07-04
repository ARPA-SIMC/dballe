#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/sqlite.h"
#include <cstring>
#include <map>
#include <sstream>

using namespace wreport;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

namespace {

Level to_level(SQLiteStatement& stm, int first_id = 0)
{
    return Level(stm.column_int(first_id), stm.column_int(first_id + 1),
                 stm.column_int(first_id + 2), stm.column_int(first_id + 3));
}

Trange to_trange(SQLiteStatement& stm, int first_id = 0)
{
    return Trange(stm.column_int(first_id), stm.column_int(first_id + 1),
                  stm.column_int(first_id + 2));
}

} // namespace

static const char* select_query = "SELECT id FROM levtr WHERE"
                                  "     ltype1=? AND l1=? AND ltype2=? AND l2=?"
                                  " AND pind=? AND p1=? AND p2=?";
static const char* select_data_query =
    "SELECT ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE id=?";
static const char* insert_query = "INSERT INTO levtr (ltype1, l1, ltype2, l2, "
                                  "pind, p1, p2) VALUES (?, ?, ?, ?, ?, ?, ?)";

SQLiteLevTr::SQLiteLevTr(v7::Transaction& tr, SQLiteConnection& conn)
    : v7::LevTr(tr), conn(conn)
{
    // Create the statement for select fixed
    sstm = conn.sqlitestatement(select_query).release();

    // Create the statement for select data
    sdstm = conn.sqlitestatement(select_data_query).release();

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();
}

SQLiteLevTr::~SQLiteLevTr()
{
    delete sstm;
    delete sdstm;
    delete istm;
}

void SQLiteLevTr::prefetch_ids(Tracer<>& trc, const std::set<int>& ids)
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
    auto stm = conn.sqlitestatement(qb);
    stm->execute([&]() {
        if (trc_sel)
            trc_sel->add_row();
        cache.insert(unique_ptr<LevTrEntry>(
            new LevTrEntry(stm->column_int(0),
                           Level(stm->column_int(1), stm->column_int(2),
                                 stm->column_int(3), stm->column_int(4)),
                           Trange(stm->column_int(5), stm->column_int(6),
                                  stm->column_int(7)))));
    });
}

const LevTrEntry* SQLiteLevTr::lookup_id(Tracer<>& trc, int id)
{
    // First look it up in the transaction cache
    const LevTrEntry* res = cache.find_entry(id);
    if (res)
        return res;

    Tracer<> trc_sel(trc ? trc->trace_select(select_data_query) : nullptr);
    sdstm->bind(id);
    sdstm->execute_one([&]() {
        if (trc_sel)
            trc_sel->add_row();
        std::unique_ptr<LevTrEntry> e(new LevTrEntry);
        e->id           = id;
        e->level.ltype1 = sdstm->column_int(0);
        e->level.l1     = sdstm->column_int(1);
        e->level.ltype2 = sdstm->column_int(2);
        e->level.l2     = sdstm->column_int(3);
        e->trange.pind  = sdstm->column_int(4);
        e->trange.p1    = sdstm->column_int(5);
        e->trange.p2    = sdstm->column_int(6);
        res             = cache.insert(move(e));
    });

    if (!res)
        error_notfound::throwf("levtr with id %d not found in the database",
                               id);

    return res;
}

int SQLiteLevTr::obtain_id(Tracer<>& trc, const LevTrEntry& desc)
{
    int id = cache.find_id(desc);
    if (id != MISSING_INT)
        return id;

    Tracer<> trc_oid(trc ? trc->trace_select(select_query) : nullptr);
    sstm->bind(desc.level.ltype1, desc.level.l1, desc.level.ltype2,
               desc.level.l2, desc.trange.pind, desc.trange.p1, desc.trange.p2);

    // If there is an existing record, use its ID and don't do an INSERT
    sstm->execute_one([&]() {
        if (trc_oid)
            trc_oid->add_row();
        id = sstm->column_int(0);
    });
    trc_oid.done();
    if (id != MISSING_INT)
    {
        cache.insert(desc, id);
        return id;
    }

    // Not found in the database, insert a new one
    trc_oid.reset(trc ? trc->trace_insert(insert_query, 1) : nullptr);
    istm->bind(desc.level.ltype1, desc.level.l1, desc.level.ltype2,
               desc.level.l2, desc.trange.pind, desc.trange.p1, desc.trange.p2);
    istm->execute();
    id = conn.get_last_insert_id();
    cache.insert(desc, id);
    return id;
}

void SQLiteLevTr::_dump(
    std::function<void(int, const Level&, const Trange&)> out)
{
    auto stm = conn.sqlitestatement("SELECT id, ltype1, l1, ltype2, l2, pind, "
                                    "p1, p2 FROM levtr ORDER BY ID");
    stm->execute([&]() {
        out(stm->column_int(0), to_level(*stm, 1), to_trange(*stm, 5));
    });
}

} // namespace sqlite
} // namespace v7
} // namespace db
} // namespace dballe
