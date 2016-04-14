#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/mysql.h"
#include <map>
#include <sstream>
#include <cstring>
#include <sqltypes.h>
#include <sql.h>

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

MySQLLevTr::MySQLLevTr(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLLevTr::~MySQLLevTr()
{
}

void MySQLLevTr::prefetch_ids(const std::set<int>& ids, std::function<void(int, const LevTrDesc&)> dest)
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
    while (auto row = res.fetch())
    {
        dest(
            row.as_int(0),
            LevTrDesc(
                Level(row.as_int(1), row.as_int(2), row.as_int(3), row.as_int(4)),
                Trange(row.as_int(5), row.as_int(6), row.as_int(7))));
    }
}

void MySQLLevTr::prefetch_same_level(int id, std::function<void(int, const LevTrDesc&)> dest)
{
    char query[128];
    snprintf(query, 128, "SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE ltype1=(SELECT ltype1 FROM levtr WHERE id=%d)", id);
    auto res = conn.exec_store(query);
    while (auto row = res.fetch())
    {
        dest(
            row.as_int(0),
            LevTrDesc(
                Level(row.as_int(1), row.as_int(2), row.as_int(3), row.as_int(4)),
                Trange(row.as_int(5), row.as_int(6), row.as_int(7))));
    }
}

levtrs_t::iterator MySQLLevTr::lookup_id(State& st, int id)
{
    auto res = st.levtr_ids.find(id);
    if (res != st.levtr_ids.end())
        return res->second;

    char query[128];
    snprintf(query, 128, "SELECT ltype1, l1, ltype2, l2, pind, p1, p2 FROM levtr WHERE id=%d", id);

    bool found = false;
    levtrs_t::iterator new_res;
    auto qres = conn.exec_store(query);
    while (auto row = qres.fetch())
    {
        LevTrDesc desc;

        desc.level.ltype1 = row.as_int(0);
        desc.level.l1 = row.as_int(1);
        desc.level.ltype2 = row.as_int(2);
        desc.level.l2 = row.as_int(3);

        desc.trange.pind = row.as_int(4);
        desc.trange.p1 = row.as_int(5);
        desc.trange.p2 = row.as_int(6);

        LevTrState sst;
        sst.id = id;
        sst.is_new = false;
        new_res = st.add_levtr(desc, sst);
        found = true;
    }

    if (!found)
        error_notfound::throwf("levtr with id %d not found in the database", id);

    return new_res;
}

levtrs_t::iterator MySQLLevTr::obtain_id(State& state, const LevTrDesc& desc)
{
    auto res = state.levtrs.find(desc);
    if (res != state.levtrs.end())
        return res;

    char query[512];
    snprintf(query, 512, R"(
        SELECT id FROM levtr WHERE
             ltype1=%d AND l1=%d AND ltype2=%d AND l2=%d
         AND pind=%d AND p1=%d AND p2=%d
    )", desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);

    LevTrState st;

    // If there is an existing record, use its ID and don't do an INSERT
    bool found = false;
    auto qres = conn.exec_store(query);
    while (auto row = qres.fetch())
    {
        st.id = row.as_int(0);
        st.is_new = false;
        found = true;
    }
    if (found)
        return state.add_levtr(desc, st);

    // Not found in the database, insert a new one
    snprintf(query, 512, "INSERT INTO levtr (ltype1, l1, ltype2, l2, pind, p1, p2) VALUES (%d, %d, %d, %d, %d, %d, %d)",
            desc.level.ltype1, desc.level.l1, desc.level.ltype2, desc.level.l2,
            desc.trange.pind, desc.trange.p1, desc.trange.p2);
    conn.exec_no_data(query);
    st.id = conn.get_last_insert_id();
    st.is_new = true;
    return state.add_levtr(desc, st);
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
