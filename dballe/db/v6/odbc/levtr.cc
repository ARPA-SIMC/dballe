#include "levtr.h"
#include "dballe/sql/odbc.h"
#include "dballe/record.h"
#include "dballe/core/defs.h"
#include "dballe/msg/msg.h"
#include <map>
#include <sstream>
#include <cstring>
#include <sqltypes.h>
#include <sql.h>

using namespace wreport;
using namespace std;
using dballe::sql::ODBCConnection;
using dballe::sql::ODBCStatement;
using dballe::sql::ServerType;

namespace dballe {
namespace db {
namespace v6 {
namespace odbc {

ODBCLevTrV6::ODBCLevTrV6(ODBCConnection& conn)
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
    const char* remove_query =
        "DELETE FROM lev_tr WHERE id=?";

    /* Override queries for some databases */
    switch (conn.server_type)
    {
        case ServerType::ORACLE:
            insert_query = "INSERT INTO lev_tr VALUES (seq_lev_tr.NextVal, ?, ?, ?, ?, ?, ?, ?)";
            seq_lev_tr = new dballe::sql::Sequence(conn, "seq_lev_tr");
            break;
        default: break;
    }

    /* Create the statement for select fixed */
    sstm = conn.odbcstatement(select_query).release();
    sstm->bind_in(1, working_row.ltype1);
    sstm->bind_in(2, working_row.l1);
    sstm->bind_in(3, working_row.ltype2);
    sstm->bind_in(4, working_row.l2);
    sstm->bind_in(5, working_row.pind);
    sstm->bind_in(6, working_row.p1);
    sstm->bind_in(7, working_row.p2);
    sstm->bind_out(1, working_row.id);

    /* Create the statement for select data */
    sdstm = conn.odbcstatement(select_data_query).release();
    sdstm->bind_in(1, working_row.id);
    sdstm->bind_out(1, working_row.ltype1);
    sdstm->bind_out(2, working_row.l1);
    sdstm->bind_out(3, working_row.ltype2);
    sdstm->bind_out(4, working_row.l2);
    sdstm->bind_out(5, working_row.pind);
    sdstm->bind_out(6, working_row.p1);
    sdstm->bind_out(7, working_row.p2);

    /* Create the statement for insert */
    istm = conn.odbcstatement(insert_query).release();
    istm->bind_in(1, working_row.ltype1);
    istm->bind_in(2, working_row.l1);
    istm->bind_in(3, working_row.ltype2);
    istm->bind_in(4, working_row.l2);
    istm->bind_in(5, working_row.pind);
    istm->bind_in(6, working_row.p1);
    istm->bind_in(7, working_row.p2);

    /* Create the statement for remove */
    dstm = conn.odbcstatement(remove_query).release();
    dstm->bind_in(1, working_row.id);
}

ODBCLevTrV6::~ODBCLevTrV6()
{
    delete seq_lev_tr;
    if (sstm) delete sstm;
    if (sdstm) delete sdstm;
    if (istm) delete istm;
    if (dstm) delete dstm;
}

int ODBCLevTrV6::get_id()
{
    sstm->execute();

    /* Get the result */
    int res;
    if (sstm->fetch_expecting_one())
        res = working_row.id;
    else
        res = -1;

    return res;
}

int ODBCLevTrV6::obtain_id(const Level& lev, const Trange& tr)
{
    working_row.ltype1 = lev.ltype1;
    working_row.l1 = lev.l1;
    working_row.ltype2 = lev.ltype2;
    working_row.l2 = lev.l2;
    working_row.pind = tr.pind;
    working_row.p1 = tr.p1;
    working_row.p2 = tr.p2;

    // Check for an existing lev_tr with these data
    int id = get_id();
    // If there is an existing record, use its ID and don't do an INSERT
    if (id != -1) return id;
    return insert();
}

int ODBCLevTrV6::insert()
{
    istm->execute_and_close();

    if (seq_lev_tr)
        return seq_lev_tr->read();
    else
        return conn.get_last_insert_id();
}

void ODBCLevTrV6::remove()
{
    dstm->execute_and_close();
}

const v6::LevTr::DBRow* ODBCLevTrV6::read(int id)
{
    working_row.id = id;
    sdstm->execute();
    if (!sdstm->fetch_expecting_one())
        return nullptr;
    return &working_row;
}

void ODBCLevTrV6::read_all(std::function<void(const LevTr::DBRow&)> dest)
{
    // Prefetch everything
    auto stm = conn.odbcstatement("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");
    stm->bind_out(1, working_row.id);
    stm->bind_out(2, working_row.ltype1);
    stm->bind_out(3, working_row.l1);
    stm->bind_out(4, working_row.ltype2);
    stm->bind_out(5, working_row.l2);
    stm->bind_out(6, working_row.pind);
    stm->bind_out(7, working_row.p1);
    stm->bind_out(8, working_row.p2);
    stm->execute();
    while (stm->fetch())
        dest(working_row);
}

void ODBCLevTrV6::dump(FILE* out)
{
    int id;
    int ltype1;
    int l1;
    int ltype2;
    int l2;
    int pind;
    int p1;
    int p2;

    auto stm = conn.odbcstatement("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY id");
    stm->bind_out(1, id);
    stm->bind_out(2, ltype1);
    stm->bind_out(3, l1);
    stm->bind_out(4, ltype2);
    stm->bind_out(5, l2);
    stm->bind_out(6, pind);
    stm->bind_out(7, p1);
    stm->bind_out(8, p2);
    stm->execute();
    int count;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    for (count = 0; stm->fetch(); ++count)
    {
        fprintf(out, " %4d ", (int)id);
        int written = Level(ltype1, l1, ltype2, l2).print(out);
        while (written++ < 21) putc(' ', out);
        written = Trange(pind, p1, p2).print(out);
        while (written++ < 11) putc(' ', out);
    }
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}

}
}
}
}
