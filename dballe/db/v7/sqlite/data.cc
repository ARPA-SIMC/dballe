#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/batch.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/sqlite.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include "dballe/core/values.h"
#include "dballe/core/varmatch.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

template class SQLiteDataCommon<StationData>;
template class SQLiteDataCommon<Data>;

template<typename Parent>
SQLiteDataCommon<Parent>::SQLiteDataCommon(dballe::sql::SQLiteConnection& conn)
    : conn(conn)
{
    char query[64];
    snprintf(query, 64, "UPDATE %s set value=?, attrs=? WHERE id=?", Parent::table_name);
    ustm = conn.sqlitestatement(query).release();
}

template<typename Parent>
SQLiteDataCommon<Parent>::~SQLiteDataCommon()
{
    delete read_attrs_stm;
    delete write_attrs_stm;
    delete remove_attrs_stm;
    delete sstm;
    delete istm;
    delete ustm;
}

template<typename Parent>
void SQLiteDataCommon<Parent>::read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    if (!read_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "SELECT attrs FROM %s WHERE id=?", Parent::table_name);
        read_attrs_stm = conn.sqlitestatement(query).release();
    }
    read_attrs_stm->bind_val(1, id_data);
    read_attrs_stm->execute_one([&]() {
        Values::decode(read_attrs_stm->column_blob(0), dest);
    });
}

template<typename Parent>
void SQLiteDataCommon<Parent>::write_attrs(int id_data, const Values& values)
{
    if (!write_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=? WHERE id=?", Parent::table_name);
        write_attrs_stm = conn.sqlitestatement(query).release();
    }
    vector<uint8_t> encoded = values.encode();
    write_attrs_stm->bind_val(1, encoded);
    write_attrs_stm->bind_val(2, id_data);
    write_attrs_stm->execute();
}

template<typename Parent>
void SQLiteDataCommon<Parent>::remove_all_attrs(int id_data)
{
    if (!remove_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=NULL WHERE id=?", Parent::table_name);
        remove_attrs_stm = conn.sqlitestatement(query).release();
    }
    remove_attrs_stm->bind_val(1, id_data);
    remove_attrs_stm->execute();
}

namespace {

bool match_attrs(const Varmatch& match, const std::vector<uint8_t>& attrs)
{
    bool found = false;
    Values::decode(attrs, [&](std::unique_ptr<wreport::Var> var) {
        if (match(*var))
            found = true;
    });
    return found;
}

}

template<typename Parent>
void SQLiteDataCommon<Parent>::remove(const v7::IdQueryBuilder& qb)
{
    char query[64];
    snprintf(query, 64, "DELETE FROM %s WHERE id=?", Parent::table_name);
    auto stmd = conn.sqlitestatement(query);
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    std::unique_ptr<Varmatch> attr_filter;
    if (!qb.query.attr_filter.empty())
        attr_filter = Varmatch::parse(qb.query.attr_filter);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        if (attr_filter.get() && !match_attrs(*attr_filter, stm->column_blob(1))) return;

        // Compile the DELETE query for the data
        stmd->bind_val(1, stm->column_int(0));
        stmd->execute();
    });
}

template<typename Parent>
void SQLiteDataCommon<Parent>::update(dballe::db::v7::Transaction& t, std::vector<typename Parent::BatchValue>& vars, bool with_attrs)
{
    for (auto& v: vars)
    {
        ustm->bind_val(1, v.var->enqc());
        values::Encoder enc;
        if (with_attrs && v.var->next_attr())
        {
            enc.append_attributes(*v.var);
            ustm->bind_val(2, enc.buf);
        }
        else
            ustm->bind_null_val(2);
        ustm->bind_val(3, v.id);

        ustm->execute();
    }
}


SQLiteStationData::SQLiteStationData(SQLiteConnection& conn)
    : SQLiteDataCommon(conn)
{
    sstm = conn.sqlitestatement("SELECT id, code FROM station_data WHERE id_station=?").release();
    istm = conn.sqlitestatement("INSERT INTO station_data (id_station, code, value, attrs) VALUES (?, ?, ?, ?)").release();
}

void SQLiteStationData::query(int id_station, std::function<void(int id, wreport::Varcode code)> dest)
{
    sstm->bind_val(1, id_station);
    sstm->execute([&]() {
        int id = sstm->column_int(0);
        wreport::Varcode code = sstm->column_int(1);
        dest(id, code);
    });
}

void SQLiteStationData::insert(dballe::db::v7::Transaction& t, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());
    istm->bind_val(1, id_station);
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        istm->bind_val(2, v->var->code());
        istm->bind_val(3, v->var->enqc());
        values::Encoder enc;
        if (with_attrs && v->var->next_attr())
        {
            enc.append_attributes(*v->var);
            istm->bind_val(4, enc.buf);
        }
        else
            istm->bind_null_val(4);
        istm->execute();

        v->id = conn.get_last_insert_id();
    }
}

void SQLiteStationData::dump(FILE* out)
{
    StationDataDumper dumper(out);

    dumper.print_head();
    auto stm = conn.sqlitestatement("SELECT id, id_station, code, value, attrs FROM station_data");
    stm->execute([&]() {
        const char* val = stm->column_isnull(3) ? nullptr : stm->column_string(3);
        dumper.print_row(stm->column_int(0), stm->column_int(1), stm->column_int(2), val, stm->column_blob(4));
    });
    dumper.print_tail();
}


SQLiteData::SQLiteData(SQLiteConnection& conn)
    : SQLiteDataCommon(conn)
{
    sstm = conn.sqlitestatement("SELECT id, id_levtr, code FROM data WHERE id_station=? AND datetime=?").release();
    istm = conn.sqlitestatement("INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (?, ?, ?, ?, ?, ?)").release();
}

void SQLiteData::query(int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest)
{
    sstm->bind_val(1, id_station);
    sstm->bind_val(2, datetime);
    sstm->execute([&]() {
        int id_levtr = sstm->column_int(1);
        wreport::Varcode code = sstm->column_int(2);
        int id = sstm->column_int(0);
        dest(id, id_levtr, code);
    });
}

void SQLiteData::insert(dballe::db::v7::Transaction& t, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());
    istm->bind_val(1, id_station);
    istm->bind_val(3, datetime);
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        istm->bind_val(2, v->id_levtr);
        istm->bind_val(4, v->var->code());
        istm->bind_val(5, v->var->enqc());
        values::Encoder enc;
        if (with_attrs && v->var->next_attr())
        {
            enc.append_attributes(*v->var);
            istm->bind_val(6, enc.buf);
        }
        else
            istm->bind_null_val(6);
        istm->execute();

        v->id = conn.get_last_insert_id();
    }
}

void SQLiteData::dump(FILE* out)
{
    DataDumper dumper(out);

    dumper.print_head();
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_levtr, datetime, code, value, attrs FROM data");
    stm->execute([&]() {
        const char* val = stm->column_isnull(5) ? nullptr : stm->column_string(5);
        dumper.print_row(stm->column_int(0), stm->column_int(1), stm->column_int(2), stm->column_datetime(3), stm->column_int(4), val, stm->column_blob(6));
    });
    dumper.print_tail();
}

}
}
}
}
