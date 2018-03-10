#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/sqlite.h"
#include <map>
#include <sstream>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

namespace {

Level to_level(SQLiteStatement& stm, int first_id=0)
{
    return Level(
            stm.column_int(first_id),
            stm.column_int(first_id + 1),
            stm.column_int(first_id + 2),
            stm.column_int(first_id + 3));
}

Trange to_trange(SQLiteStatement& stm, int first_id=0)
{
    return Trange(
            stm.column_int(first_id),
            stm.column_int(first_id + 1),
            stm.column_int(first_id + 2));
}

}

SQLiteLevTr::SQLiteLevTr(SQLiteConnection& conn)
    : conn(conn)
{
    const char* select_query =
        "SELECT id FROM levtr WHERE"
        "     ltype1=? AND l1=? AND ltype2=? AND l2=?"
        " AND pind=? AND p1=? AND p2=?";
    const char* select_data_query =
        "SELECT ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE id=?";
    const char* insert_query =
        "INSERT INTO levtr (ltype1, l1, ltype2, l2, pind, p1, p2) VALUES (?, ?, ?, ?, ?, ?, ?)";

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

void SQLiteLevTr::prefetch_ids(const std::set<int>& ids)
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

    auto stm = conn.sqlitestatement(qb);
    stm->execute([&]() {
        cache.insert(unique_ptr<LevTrEntry>(new LevTrEntry(
                stm->column_int(0),
                Level(stm->column_int(1), stm->column_int(2), stm->column_int(3), stm->column_int(4)),
                Trange(stm->column_int(5), stm->column_int(6), stm->column_int(7)))));
    });
}

const LevTrEntry* SQLiteLevTr::lookup_id(int id)
{
    // First look it up in the transaction cache
    const LevTrEntry* res = cache.find_entry(id);
    if (res) return res;

    sdstm->bind(id);
    sdstm->execute_one([&]() {
        std::unique_ptr<LevTrEntry> e(new LevTrEntry);
        e->id = id;
        e->level.ltype1 = sdstm->column_int(0);
        e->level.l1 = sdstm->column_int(1);
        e->level.ltype2 = sdstm->column_int(2);
        e->level.l2 = sdstm->column_int(3);
        e->trange.pind = sdstm->column_int(4);
        e->trange.p1 = sdstm->column_int(5);
        e->trange.p2 = sdstm->column_int(6);
        // TODO: mark that this has been inserted in this transaction
        // sst.is_new = false;
        res = cache.insert(move(e));
    });

    if (!res)
        error_notfound::throwf("levtr with id %d not found in the database", id);

    return res;
}

int SQLiteLevTr::obtain_id(const LevTrEntry& desc)
{
    int id = cache.find_id(desc);
    if (id != MISSING_INT) return id;

    sstm->bind(
            desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);

    // If there is an existing record, use its ID and don't do an INSERT
    sstm->execute_one([&]() {
        id = sstm->column_int(0);
    });
    if (id != MISSING_INT)
    {
        cache.insert(desc, id);
        return id;
    }

    // Not found in the database, insert a new one
    istm->bind(
            desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);
    istm->execute();
    id = conn.get_last_insert_id();
    // TODO: mark entry as newly inserted in this transaction
    // st.is_new = true;
    cache.insert(desc, id);
    return id;
}

void SQLiteLevTr::_dump(std::function<void(int, const Level&, const Trange&)> out)
{
    auto stm = conn.sqlitestatement("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr ORDER BY ID");
    stm->execute([&]() {
        out(stm->column_int(0), to_level(*stm, 1), to_trange(*stm, 5));
    });
}

}
}
}
}
