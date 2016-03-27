#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/sql/postgresql.h"
#include "dballe/sql/querybuf.h"

using namespace wreport;
using namespace std;
using dballe::sql::PostgreSQLConnection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

PostgreSQLRepinfo::PostgreSQLRepinfo(PostgreSQLConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

PostgreSQLRepinfo::~PostgreSQLRepinfo()
{
}

void PostgreSQLRepinfo::read_cache()
{
    cache.clear();
    memo_idx.clear();

    auto stm = conn.exec("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    for (unsigned row = 0; row < stm.rowcount(); ++row)
    {
        cache_append(
            stm.get_int4(row, 0),
            stm.get_string(row, 1),
            stm.get_string(row, 2),
            stm.get_int4(row, 3),
            stm.get_string(row, 4),
            stm.get_int4(row, 5)
        );
    }

    // Rebuild the memo index as well
    rebuild_memo_idx();
}

void PostgreSQLRepinfo::insert_auto_entry(const char* memo)
{
    unsigned id = conn.exec_one_row("SELECT MAX(id) FROM repinfo").get_int4(0, 0);
    int prio = conn.exec_one_row("SELECT MAX(prio) FROM repinfo").get_int4(0, 0);

    ++id;
    ++prio;

    Querybuf query(500);
    query.appendf(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
               VALUES (%u, $1::text, $1::text, %d, '-', 255)
    )", id, prio);
    conn.exec_no_data(query, memo);
}

int PostgreSQLRepinfo::id_use_count(unsigned id, const char* name)
{
    Querybuf query(500);
    query.appendf("SELECT COUNT(1) FROM station WHERE rep=%u", id);
    return conn.exec_one_row(query).get_int4(0, 0);
}

void PostgreSQLRepinfo::delete_entry(unsigned id)
{
    conn.exec_no_data("DELETE FROM repinfo WHERE id=$1::int4", (int32_t)id);
}

void PostgreSQLRepinfo::update_entry(const v7::repinfo::Cache& entry)
{
    conn.exec_no_data(R"(
        UPDATE repinfo SET memo=$2::text, description=$3::text, prio=$4::int4, descriptor=$5::text, tablea=$6::int4
         WHERE id=$1::int4
    )", (int32_t)entry.id,
        entry.new_memo,
        entry.new_desc,
        (int32_t)entry.new_prio,
        entry.new_descriptor,
        (int32_t)entry.new_tablea);
}

void PostgreSQLRepinfo::insert_entry(const v7::repinfo::Cache& entry)
{
    conn.exec_no_data(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES ($1::int4, $2::text, $3::text, $4::int4, $5::text, $6::int4)
    )", (int32_t)entry.id,
        entry.new_memo,
        entry.new_desc,
        (int32_t)entry.new_prio,
        entry.new_descriptor,
        (int32_t)entry.new_tablea);
}

void PostgreSQLRepinfo::dump(FILE* out)
{
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");

    int count = 0;
    auto stm = conn.exec("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    for (unsigned row = 0; row < stm.rowcount(); ++row)
    {
        string memo = stm.get_string(row, 1);
        string desc = stm.get_string(row, 2);
        string descriptor = stm.get_string(row, 4);
        fprintf(out, " %4d   %s  %s  %d  %s %d\n",
                stm.get_int4(row, 0),
                memo.c_str(),
                desc.c_str(),
                stm.get_int4(row, 3),
                descriptor.c_str(),
                stm.get_int4(row, 5));
        ++count;
    };
    fprintf(out, "%d element%s in table repinfo\n", count, count != 1 ? "s" : "");
}

}
}
}
}
