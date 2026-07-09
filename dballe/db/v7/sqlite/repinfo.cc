#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/sql/sqlite.h"

using namespace wreport;
using namespace std;
using dballe::sql::SQLiteConnection;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

SQLiteRepinfoV7::SQLiteRepinfoV7(SQLiteConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

SQLiteRepinfoV7::~SQLiteRepinfoV7() {}

void SQLiteRepinfoV7::read_cache()
{
    cache.clear();
    memo_idx.clear();

    auto stm =
        conn.sqlitestatement("SELECT id, memo, description, prio, descriptor, "
                             "tablea FROM repinfo ORDER BY id");
    stm->execute([&]() {
        string memo       = stm->column_string(1);
        string desc       = stm->column_string(2);
        string descriptor = stm->column_string(4);
        cache_append(stm->column_int(0), memo.c_str(), desc.c_str(),
                     stm->column_int(3), descriptor.c_str(),
                     stm->column_int(5));
    });

    // Rebuild the memo index as well
    rebuild_memo_idx();
}

void SQLiteRepinfoV7::insert_auto_entry(const char* memo)
{
    auto stm = conn.sqlitestatement("SELECT MAX(id) FROM repinfo");
    unsigned id;
    stm->execute_one([&]() { id = stm->column_int(0); });

    stm = conn.sqlitestatement("SELECT MAX(prio) FROM repinfo");
    unsigned prio;
    stm->execute_one([&]() { prio = stm->column_int(0); });

    ++id;
    ++prio;

    stm = conn.sqlitestatement(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
               VALUES (?, ?, ?, ?, '-', 255)
    )");
    stm->bind(id, memo, memo, prio);
    stm->execute();
}

int SQLiteRepinfoV7::id_use_count(unsigned id, const char* name)
{
    unsigned count = 0;
    auto stm = conn.sqlitestatement("SELECT COUNT(1) FROM station WHERE rep=?");
    stm->bind(id);
    stm->execute_one([&]() { count = stm->column_int(0); });
    return count;
}

void SQLiteRepinfoV7::delete_entry(unsigned id)
{
    auto stm = conn.sqlitestatement("DELETE FROM repinfo WHERE id=?");
    stm->bind(id);
    stm->execute();
}

void SQLiteRepinfoV7::update_entry(const v7::repinfo::Cache& entry)
{
    auto stm = conn.sqlitestatement(R"(
        UPDATE repinfo set memo=?, description=?, prio=?, descriptor=?, tablea=?
         WHERE id=?
    )");
    stm->bind(entry.new_memo, entry.new_desc, entry.new_prio,
              entry.new_descriptor.c_str(), entry.new_tablea, entry.id);
    stm->execute();
}

void SQLiteRepinfoV7::insert_entry(const v7::repinfo::Cache& entry)
{
    auto stm = conn.sqlitestatement(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (?, ?, ?, ?, ?, ?)
    )");
    stm->bind(entry.id, entry.new_memo, entry.new_desc, entry.new_prio,
              entry.new_descriptor, entry.new_tablea);
    stm->execute();
}

void SQLiteRepinfoV7::dump(FILE* out)
{
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");

    int count = 0;
    auto stm =
        conn.sqlitestatement("SELECT id, memo, description, prio, descriptor, "
                             "tablea FROM repinfo ORDER BY id");
    stm->execute([&]() {
        string memo       = stm->column_string(1);
        string desc       = stm->column_string(2);
        string descriptor = stm->column_string(4);
        fprintf(out, " %4d   %s  %s  %d  %s %d\n", stm->column_int(0),
                memo.c_str(), desc.c_str(), stm->column_int(3),
                descriptor.c_str(), stm->column_int(5));

        ++count;
    });
    fprintf(out, "%d element%s in table repinfo\n", count,
            count != 1 ? "s" : "");
}

} // namespace sqlite
} // namespace v7
} // namespace db
} // namespace dballe
