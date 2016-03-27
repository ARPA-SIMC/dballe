#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/mysql.h"

using namespace wreport;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

MySQLRepinfoBase::MySQLRepinfoBase(MySQLConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

MySQLRepinfoBase::~MySQLRepinfoBase()
{
}

void MySQLRepinfoBase::read_cache()
{
    cache.clear();
    memo_idx.clear();

    auto res = conn.exec_store("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    while (auto row = res.fetch())
        cache_append(
            row.as_int(0),
            row.as_cstring(1),
            row.as_cstring(2),
            row.as_int(3),
            row.as_cstring(4),
            row.as_int(5)
        );

    // Rebuild the memo index as well
    rebuild_memo_idx();
}

void MySQLRepinfoBase::insert_auto_entry(const char* memo)
{
    auto res = conn.exec_store("SELECT MAX(id) FROM repinfo");
    unsigned id = res.expect_one_result().as_unsigned(0);

    res = conn.exec_store("SELECT MAX(prio) FROM repinfo");
    unsigned prio = res.expect_one_result().as_unsigned(0);

    ++id;
    ++prio;

    string escaped_memo = conn.escape(memo);

    Querybuf iq;
    iq.appendf(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (%u, '%s', '%s', %u, '0', 255)
    )", id, escaped_memo.c_str(), escaped_memo.c_str(), prio);
    conn.exec_no_data(iq);
}

int MySQLRepinfoBase::id_use_count(unsigned id, const char* name)
{
    Querybuf q;
    q.appendf("SELECT COUNT(1) FROM context WHERE id_report=%u", id);
    auto res = conn.exec_store(q);
    return res.expect_one_result().as_unsigned(0);
}

void MySQLRepinfoBase::delete_entry(unsigned id)
{
    Querybuf q;
    q.appendf("DELETE FROM repinfo WHERE id=%u", id);
    conn.exec_no_data(q);
}

void MySQLRepinfoBase::update_entry(const v6::repinfo::Cache& entry)
{
    Querybuf q;
    string escaped_memo = conn.escape(entry.new_memo);
    string escaped_desc = conn.escape(entry.new_desc);
    string escaped_descriptor = conn.escape(entry.new_descriptor);
    q.appendf(R"(
        UPDATE repinfo set memo='%s', description='%s', prio=%d, descriptor='%s', tablea=%u
         WHERE id=%u
    )", escaped_memo.c_str(),
        escaped_desc.c_str(),
        entry.new_prio,
        escaped_descriptor.c_str(),
        entry.new_tablea,
        entry.id);
    conn.exec_no_data(q);
}

void MySQLRepinfoBase::insert_entry(const v6::repinfo::Cache& entry)
{
    Querybuf q;
    string escaped_memo = conn.escape(entry.new_memo);
    string escaped_desc = conn.escape(entry.new_desc);
    string escaped_descriptor = conn.escape(entry.new_descriptor);
    q.appendf(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (%u, '%s', '%s', %d, '%s', %u)
    )", entry.id,
        escaped_memo.c_str(),
        escaped_desc.c_str(),
        entry.new_prio,
        escaped_descriptor.c_str(),
        entry.new_tablea);
    conn.exec_no_data(q);
}

void MySQLRepinfoBase::dump(FILE* out)
{
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");
    int count = 0;
    auto res = conn.exec_store("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    while (auto row = res.fetch())
    {
        fprintf(out, " %4d   %s  %s  %d  %s %d\n",
                row.as_int(0),
                row.as_cstring(1),
                row.as_cstring(2),
                row.as_int(3),
                row.as_cstring(4),
                row.as_int(5));
        ++count;
    }
    fprintf(out, "%d element%s in table repinfo\n", count, count != 1 ? "s" : "");
}

MySQLRepinfoV6::MySQLRepinfoV6(MySQLConnection& conn) : MySQLRepinfoBase(conn) {}

int MySQLRepinfoV6::id_use_count(unsigned id, const char* name)
{
    Querybuf q;
    q.appendf("SELECT COUNT(1) FROM data WHERE id_report=%u", id);
    auto res = conn.exec_store(q);
    return res.expect_one_result().as_unsigned(0);
}

}
}
}
}
