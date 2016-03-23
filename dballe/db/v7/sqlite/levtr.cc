#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/sqlite.h"
#include <map>
#include <sstream>
#include <cstring>
#include <sqltypes.h>
#include <sql.h>

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

SQLiteLevTrV7::SQLiteLevTrV7(SQLiteConnection& conn)
    : conn(conn)
{
    const char* select_query =
        "SELECT id FROM lev_tr WHERE"
        "     ltype1=? AND l1=? AND ltype2=? AND l2=?"
        " AND ptype=? AND p1=? AND p2=?";
    const char* select_data_query =
        "SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=?";
    const char* insert_query =
        "INSERT INTO lev_tr (ltype1, l1, ltype2, l2, ptype, p1, p2) VALUES (?, ?, ?, ?, ?, ?, ?)";

    // Create the statement for select fixed
    sstm = conn.sqlitestatement(select_query).release();

    // Create the statement for select data
    sdstm = conn.sqlitestatement(select_data_query).release();

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();
}

SQLiteLevTrV7::~SQLiteLevTrV7()
{
    delete sstm;
    delete sdstm;
    delete istm;
}

void SQLiteLevTrV7::prefetch_ids(const std::set<int>& ids, std::map<int, LevTrDesc>& data)
{
    if (ids.empty()) return;

    sql::Querybuf qb;
    if (ids.size() < 100)
    {
        qb.append("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id IN (");
        qb.start_list(",");
        for (auto id: ids)
            qb.append_listf("%d", id);
        qb.append(")");
    } else
        qb.append("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");

    auto stm = conn.sqlitestatement(qb);
    stm->execute([&]() {
        data.insert(make_pair(
            stm->column_int(0),
            LevTrDesc(
                Level(stm->column_int(1), stm->column_int(2), stm->column_int(3), stm->column_int(4)),
                Trange(stm->column_int(5), stm->column_int(6), stm->column_int(7)))));
    });
}

State::levels_t::iterator SQLiteLevTrV7::lookup_id(State& st, int id)
{
    auto res = st.level_ids.find(id);
    if (res != st.level_ids.end())
        return res->second;

    sdstm->bind(id);
    bool found = false;
    State::levels_t::iterator new_res;
    sdstm->execute_one([&]() {
        LevTrDesc desc;

        desc.level.ltype1 = sdstm->column_int(0);
        desc.level.l1 = sdstm->column_int(1);
        desc.level.ltype2 = sdstm->column_int(2);
        desc.level.l2 = sdstm->column_int(3);

        desc.trange.pind = sdstm->column_int(4);
        desc.trange.p1 = sdstm->column_int(5);
        desc.trange.p2 = sdstm->column_int(6);

        LevTrState sst;
        sst.id = id;
        sst.is_new = false;
        new_res = st.add_levtr(desc, sst);
        found = true;
    });

    if (!found)
        error_notfound::throwf("levtr with id %d not found in the database", id);

    return new_res;
}

State::levels_t::iterator SQLiteLevTrV7::obtain_id(State& state, const LevTrDesc& desc)
{
    auto res = state.levels.find(desc);
    if (res != state.levels.end())
        return res;

    sstm->bind(
            desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);

    LevTrState st;

    // If there is an existing record, use its ID and don't do an INSERT
    bool found = false;
    sstm->execute_one([&]() {
        st.id = sstm->column_int(0);
        st.is_new = false;
        found = true;
    });
    if (found)
        return state.add_levtr(desc, st);

    // Not found in the database, insert a new one
    istm->bind(
            desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);
    istm->execute();
    st.id = conn.get_last_insert_id();
    st.is_new = true;
    return state.add_levtr(desc, st);
}

#if 0
const v7::LevTr::DBRow* SQLiteLevTrV7::read(int id)
{
    sdstm->bind(id);
    bool found = false;
    sdstm->execute([&]() {
        working_row.id = id;
        working_row.ltype1 = sdstm->column_int(0);
        working_row.l1 = sdstm->column_int(1);
        working_row.ltype2 = sdstm->column_int(2);
        working_row.l2 = sdstm->column_int(3);
        working_row.pind = sdstm->column_int(4);
        working_row.p1 = sdstm->column_int(5);
        working_row.p2 = sdstm->column_int(6);
        found = true;
    });

    if (!found) return nullptr;
    return &working_row;
}

void SQLiteLevTrV7::read_all(std::function<void(const LevTr::DBRow&)> dest)
{
    auto stm = conn.sqlitestatement("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");
    stm->execute([&]() {
        working_row.id = stm->column_int(0);
        working_row.ltype1 = stm->column_int(1);
        working_row.l1 = stm->column_int(2);
        working_row.ltype2 = stm->column_int(3);
        working_row.l2 = stm->column_int(4);
        working_row.pind = stm->column_int(5);
        working_row.p1 = stm->column_int(6);
        working_row.p2 = stm->column_int(7);
        dest(working_row);
    });
}
#endif

void SQLiteLevTrV7::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    auto stm = conn.sqlitestatement("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY ID");
    stm->execute([&]() {
        fprintf(out, " %4d ", stm->column_int(0));
        int written = to_level(*stm, 1).print(out);
        while (written++ < 21) putc(' ', out);
        written = to_trange(*stm, 5).print(out);
        while (written++ < 11) putc(' ', out);
        ++count;
    });
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}

}
}
}
}
