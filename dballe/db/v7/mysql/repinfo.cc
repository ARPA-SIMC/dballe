#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"

using namespace wreport;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

MySQLRepinfoV7::MySQLRepinfoV7(MySQLConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

MySQLRepinfoV7::~MySQLRepinfoV7() {}

void MySQLRepinfoV7::read_cache()
{
    cache.clear();
    memo_idx.clear();

    auto res = conn.exec_store("SELECT id, memo, description, prio, "
                               "descriptor, tablea FROM repinfo ORDER BY id");
    while (auto row = res.fetch())
        cache_append(row.as_int(0), row.as_cstring(1), row.as_cstring(2),
                     row.as_int(3), row.as_cstring(4), row.as_int(5));

    // Rebuild the memo index as well
    rebuild_memo_idx();
}

void MySQLRepinfoV7::insert_auto_entry(const char* memo)
{
    unsigned id = conn.exec_store("SELECT MAX(id) FROM repinfo")
                      .expect_one_result()
                      .as_int(0);
    unsigned prio = conn.exec_store("SELECT MAX(prio) FROM repinfo")
                        .expect_one_result()
                        .as_int(0);

    ++id;
    ++prio;

    string escaped_memo = conn.escape(memo);

    Querybuf iq;
    iq.appendf(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (%u, '%s', '%s', %u, '0', 255)
    )",
               id, escaped_memo.c_str(), escaped_memo.c_str(), prio);
    conn.exec_no_data(iq);
}

int MySQLRepinfoV7::id_use_count(unsigned id, const char* name)
{
    char query[64];
    snprintf(query, 64, "SELECT COUNT(1) FROM station WHERE rep=%d", id);
    return conn.exec_store(query).expect_one_result().as_int(0);
}

void MySQLRepinfoV7::delete_entry(unsigned id)
{
    char query[64];
    snprintf(query, 64, "DELETE FROM repinfo WHERE id=%d", id);
    conn.exec_no_data(query);
}

void MySQLRepinfoV7::update_entry(const v7::repinfo::Cache& entry)
{
    Querybuf q;
    string escaped_memo       = conn.escape(entry.new_memo);
    string escaped_desc       = conn.escape(entry.new_desc);
    string escaped_descriptor = conn.escape(entry.new_descriptor);
    q.appendf(R"(
        UPDATE repinfo set memo='%s', description='%s', prio=%d, descriptor='%s', tablea=%u
         WHERE id=%u
    )",
              escaped_memo.c_str(), escaped_desc.c_str(), entry.new_prio,
              escaped_descriptor.c_str(), entry.new_tablea, entry.id);
    conn.exec_no_data(q);
}

void MySQLRepinfoV7::insert_entry(const v7::repinfo::Cache& entry)
{
    Querybuf q;
    string escaped_memo       = conn.escape(entry.new_memo);
    string escaped_desc       = conn.escape(entry.new_desc);
    string escaped_descriptor = conn.escape(entry.new_descriptor);
    q.appendf(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (%u, '%s', '%s', %d, '%s', %u)
    )",
              entry.id, escaped_memo.c_str(), escaped_desc.c_str(),
              entry.new_prio, escaped_descriptor.c_str(), entry.new_tablea);
    conn.exec_no_data(q);
}

void MySQLRepinfoV7::dump(FILE* out)
{
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");
    int count = 0;
    auto res  = conn.exec_store("SELECT id, memo, description, prio, "
                                 "descriptor, tablea FROM repinfo ORDER BY id");
    while (auto row = res.fetch())
    {
        fprintf(out, " %4d   %s  %s  %d  %s %d\n", row.as_int(0),
                row.as_cstring(1), row.as_cstring(2), row.as_int(3),
                row.as_cstring(4), row.as_int(5));
        ++count;
    }
    fprintf(out, "%d element%s in table repinfo\n", count,
            count != 1 ? "s" : "");
}

} // namespace mysql
} // namespace v7
} // namespace db
} // namespace dballe
