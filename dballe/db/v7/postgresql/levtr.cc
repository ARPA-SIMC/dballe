#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/postgresql.h"
#include "dballe/sql/querybuf.h"
#include <cstring>
#include <map>
#include <sstream>

using namespace wreport;
using namespace std;
using dballe::sql::PostgreSQLConnection;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

namespace {

Level to_level(const dballe::sql::postgresql::Result& res, unsigned row,
               int first_id)
{
    return Level(res.get_int4(row, first_id), res.get_int4(row, first_id + 1),
                 res.get_int4(row, first_id + 2),
                 res.get_int4(row, first_id + 3));
}

Trange to_trange(const dballe::sql::postgresql::Result& res, unsigned row,
                 int first_id)
{
    return Trange(res.get_int4(row, first_id), res.get_int4(row, first_id + 1),
                  res.get_int4(row, first_id + 2));
}

} // namespace

PostgreSQLLevTr::PostgreSQLLevTr(v7::Transaction& tr,
                                 PostgreSQLConnection& conn)
    : v7::LevTr(tr), conn(conn)
{
    conn.prepare("v7_levtr_select_id", R"(
        SELECT id FROM levtr WHERE ltype1=$1::int4 AND l1=$2::int4 AND ltype2=$3::int4 AND l2=$4::int4
                               AND pind=$5::int4 AND p1=$6::int4 AND p2=$7::int4
    )");
    conn.prepare("v7_levtr_select_data", "SELECT ltype1, l1, ltype2, l2, pind, "
                                         "p1, p2 FROM levtr WHERE id=$1::int4");
    conn.prepare("v7_levtr_insert", R"(
        INSERT INTO levtr (id, ltype1, l1, ltype2, l2, pind, p1, p2)
             VALUES (DEFAULT, $1::int4, $2::int4, $3::int4, $4::int4, $5::int4, $6::int4, $7::int4)
          RETURNING id
    )");
}

PostgreSQLLevTr::~PostgreSQLLevTr() {}

void PostgreSQLLevTr::prefetch_ids(Tracer<>& trc, const std::set<int>& ids)
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
    auto res = conn.exec(qb);
    if (trc_sel)
        trc_sel->add_row(res.rowcount());
    for (unsigned row = 0; row < res.rowcount(); ++row)
        cache.insert(unique_ptr<LevTrEntry>(
            new LevTrEntry(res.get_int4(row, 0), to_level(res, row, 1),
                           to_trange(res, row, 5))));
}

const LevTrEntry* PostgreSQLLevTr::lookup_id(Tracer<>& trc, int id)
{
    using namespace dballe::sql::postgresql;
    const LevTrEntry* e = cache.find_entry(id);
    if (e)
        return e;

    Tracer<> trc_sel(trc ? trc->trace_select("v7_levtr_select_data") : nullptr);
    auto res = conn.exec_prepared("v7_levtr_select_data", id);
    if (trc_sel)
        trc_sel->add_row(res.rowcount());
    switch (res.rowcount())
    {
        case 0:
            error_notfound::throwf("levtr with id %d not found in the database",
                                   id);
        case 1:
            return cache.insert(unique_ptr<LevTrEntry>(
                new LevTrEntry(id, to_level(res, 0, 0), to_trange(res, 0, 4))));
        default:
            error_consistency::throwf(
                "select levtr data query returned %u results", res.rowcount());
    }
}

int PostgreSQLLevTr::obtain_id(Tracer<>& trc, const LevTrEntry& desc)
{
    using namespace dballe::sql::postgresql;
    int id = cache.find_id(desc);
    if (id != MISSING_INT)
        return id;

    Tracer<> trc_oid(trc ? trc->trace_select("v7_levtr_select_id") : nullptr);
    Result res =
        conn.exec_prepared("v7_levtr_select_id", desc.level.ltype1,
                           desc.level.l1, desc.level.ltype2, desc.level.l2,
                           desc.trange.pind, desc.trange.p1, desc.trange.p2);
    if (trc_oid)
        trc_oid->add_row(res.rowcount());
    switch (res.rowcount())
    {
        case 0: {
            trc_oid.done();
            trc_oid.reset(trc ? trc->trace_insert("v7_levtr_insert", 1)
                              : nullptr);
            auto res = conn.exec_prepared_one_row(
                "v7_levtr_insert", desc.level.ltype1, desc.level.l1,
                desc.level.ltype2, desc.level.l2, desc.trange.pind,
                desc.trange.p1, desc.trange.p2);
            id = res.get_int4(0, 0);
            cache.insert(desc, id);
            return id;
        }
        case 1: {
            id = res.get_int4(0, 0);
            cache.insert(desc, id);
            return id;
        }
        default:
            error_consistency::throwf(
                "select levtr ID query returned %u results", res.rowcount());
    }
}

void PostgreSQLLevTr::_dump(
    std::function<void(int, const Level&, const Trange&)> out)
{
    auto res = conn.exec("SELECT id, ltype1, l1, ltype2, l2, pind, p1, p2 FROM "
                         "levtr ORDER BY ID");
    for (unsigned row = 0; row < res.rowcount(); ++row)
        out(res.get_int4(row, 0), to_level(res, row, 1),
            to_trange(res, row, 5));
}

} // namespace postgresql
} // namespace v7
} // namespace db
} // namespace dballe
