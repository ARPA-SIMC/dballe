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

void SQLiteLevTr::prefetch_ids(const std::set<int>& ids, std::map<int, LevTrDesc>& data)
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
        data.insert(make_pair(
            stm->column_int(0),
            LevTrDesc(
                Level(stm->column_int(1), stm->column_int(2), stm->column_int(3), stm->column_int(4)),
                Trange(stm->column_int(5), stm->column_int(6), stm->column_int(7)))));
    });
}

levtrs_t::iterator SQLiteLevTr::lookup_id(State& st, int id)
{
    auto res = st.levtr_ids.find(id);
    if (res != st.levtr_ids.end())
        return res->second;

    sdstm->bind(id);
    bool found = false;
    levtrs_t::iterator new_res;
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

levtrs_t::iterator SQLiteLevTr::obtain_id(State& state, const LevTrDesc& desc)
{
    auto res = state.levtrs.find(desc);
    if (res != state.levtrs.end())
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
