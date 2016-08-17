#include "levtr.h"
#include "dballe/sql/postgresql.h"
#include "dballe/core/defs.h"
#include "dballe/msg/msg.h"
#include <map>
#include <sstream>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::PostgreSQLConnection;

namespace dballe {
namespace db {
namespace v6 {
namespace postgresql {

namespace {

Level to_level(const dballe::sql::postgresql::Result& res, unsigned row, int first_id)
{
    return Level(
            res.get_int4(row, first_id),
            res.get_int4(row, first_id + 1),
            res.get_int4(row, first_id + 2),
            res.get_int4(row, first_id + 3));
}

Trange to_trange(const dballe::sql::postgresql::Result& res, unsigned row, int first_id)
{
    return Trange(
            res.get_int4(row, first_id),
            res.get_int4(row, first_id + 1),
            res.get_int4(row, first_id + 2));
}

}

PostgreSQLLevTrV6::PostgreSQLLevTrV6(PostgreSQLConnection& conn)
    : conn(conn)
{
    conn.prepare("v6_levtr_select_id", R"(
        SELECT id FROM lev_tr WHERE
             ltype1=$1::int4 AND l1=$2::int4 AND ltype2=$3::int4 AND l2=$4::int4
         AND ptype=$5::int4 AND p1=$6::int4 AND p2=$7::int4
    )");
    conn.prepare("v6_levtr_select_data", "SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=$1::int4");
    conn.prepare("v6_levtr_insert", R"(
        INSERT INTO lev_tr (id, ltype1, l1, ltype2, l2, ptype, p1, p2)
             VALUES (DEFAULT, $1::int4, $2::int4, $3::int4, $4::int4, $5::int4, $6::int4, $7::int4)
          RETURNING id
    )");
}

PostgreSQLLevTrV6::~PostgreSQLLevTrV6()
{
}

int PostgreSQLLevTrV6::obtain_id(const Level& lev, const Trange& tr)
{
    using namespace dballe::sql::postgresql;
    Result res = conn.exec_prepared("v6_levtr_select_id",
            lev.ltype1, lev.l1, lev.ltype2, lev.l2, tr.pind, tr.p1, tr.p2);
    switch (res.rowcount())
    {
        case 0: break;
        case 1:
            // If there is an existing record, use its ID and don't do an INSERT
            return res.get_int4(0, 0);
        default: error_consistency::throwf("select levtr ID query returned %u results", res.rowcount());

    }

    return conn.exec_prepared_one_row("v6_levtr_insert",
            lev.ltype1, lev.l1, lev.ltype2, lev.l2, tr.pind, tr.p1, tr.p2).get_int4(0, 0);
}

const v6::LevTr::DBRow* PostgreSQLLevTrV6::read(int id)
{
    auto res = conn.exec_prepared("v6_levtr_select_data", id);
    switch (res.rowcount())
    {
        case 0: return nullptr;
        case 1:
            working_row.id = id;
            working_row.ltype1 = res.get_int4(0, 0);
            working_row.l1 = res.get_int4(0, 1);
            working_row.ltype2 = res.get_int4(0, 2);
            working_row.l2 = res.get_int4(0, 3);
            working_row.pind = res.get_int4(0, 4);
            working_row.p1 = res.get_int4(0, 5);
            working_row.p2 = res.get_int4(0, 6);
            return &working_row;
        default: error_consistency::throwf("select levtr data query returned %u results", res.rowcount());
    }
}

void PostgreSQLLevTrV6::read_all(std::function<void(const LevTr::DBRow&)> dest)
{
    auto res = conn.exec("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        working_row.id = res.get_int4(row, 0);
        working_row.ltype1 = res.get_int4(row, 1);
        working_row.l1 = res.get_int4(row, 2);
        working_row.ltype2 = res.get_int4(row, 3);
        working_row.l2 = res.get_int4(row, 4);
        working_row.pind = res.get_int4(row, 5);
        working_row.p1 = res.get_int4(row, 6);
        working_row.p2 = res.get_int4(row, 7);
        dest(working_row);
    }
}

void PostgreSQLLevTrV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    auto res = conn.exec("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY ID");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        fprintf(out, " %4d ", res.get_int4(row, 0));
        int written = to_level(res, row, 1).print(out);
        while (written++ < 21) putc(' ', out);
        written = to_trange(res, row, 5).print(out);
        while (written++ < 11) putc(' ', out);
        ++count;
    }
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}

}
}
}
}
