#include "cursor.h"
#include "qbuilder.h"
#include "db.h"
#include "dballe/db/sql/repinfo.h"
#include "dballe/db/sql/station.h"
#include "dballe/db/sql/levtr.h"
#include "dballe/db/sql/datav6.h"

#include "wreport/var.h"
#include "dballe/core/defs.h"
#include "dballe/core/record.h"
#include "dballe/core/query.h"

#include <cstdio>
#include <cstring>

namespace {

// Consts used for to_record_todo
const unsigned int TOREC_PSEUDOANA   = 1 << 0;
const unsigned int TOREC_BASECONTEXT = 1 << 1;
const unsigned int TOREC_DATACONTEXT = 1 << 2;
const unsigned int TOREC_DATA        = 1 << 3;

}

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

Cursor::Cursor(v6::DB& db, unsigned int modifiers)
    : db(db), modifiers(modifiers)
{
}

Cursor::~Cursor()
{
}

dballe::DB& Cursor::get_db() const { return db; }

bool Cursor::next()
{
    ++cur;
    return cur < results.size();
}

void Cursor::discard_rest()
{
    cur = results.size();
}

int Cursor::attr_reference_id() const
{
    if (results[cur].out_id_ltr == -1)
        return MISSING_INT;
    return results[cur].out_id_data;
}

int Cursor::remaining() const
{
    if (cur == -1) return results.size();
    return results.size() - cur - 1;
}

int Cursor::get_station_id() const { return results[cur].out_ana_id; }
double Cursor::get_lat() const { return (double)results[cur].out_lat / 100000.0; }
double Cursor::get_lon() const { return (double)results[cur].out_lon / 100000.0; }
const char* Cursor::get_ident(const char* def) const
{
    if (results[cur].out_ident_size == -1 || results[cur].out_ident[0] == 0)
        return def;
    return results[cur].out_ident;
}
const char* Cursor::get_rep_memo() const
{
    return db.repinfo().get_rep_memo(results[cur].out_rep_cod);
}
Level Cursor::get_level() const
{
    if (results[cur].out_id_ltr == -1)
        return Level();
    return db.lev_tr_cache().to_level(results[cur].out_id_ltr);
}
Trange Cursor::get_trange() const
{
    if (results[cur].out_id_ltr == -1)
        return Trange();
    return db.lev_tr_cache().to_trange(results[cur].out_id_ltr);
}
Datetime Cursor::get_datetime() const
{
    return results[cur].out_datetime;
}
wreport::Varcode Cursor::get_varcode() const { return (wreport::Varcode)results[cur].out_varcode; }
wreport::Var Cursor::get_var() const
{
    return Var(varinfo(results[cur].out_varcode), results[cur].out_value);
}

void Cursor::to_record_pseudoana(Record& rec)
{
    rec.seti("ana_id", results[cur].out_ana_id);
    rec.seti("lat", results[cur].out_lat);
    rec.seti("lon", results[cur].out_lon);
    if (results[cur].out_ident_size != -1 && results[cur].out_ident[0] != 0)
    {
        rec.setc("ident", results[cur].out_ident);
        rec.seti("mobile", 1);
    } else {
        rec.unset("ident");
        rec.seti("mobile", 0);
    }
}

void Cursor::to_record_repinfo(Record& rec)
{
    db.repinfo().to_record(results[cur].out_rep_cod, rec);
}

void Cursor::to_record_ltr(Record& rec)
{
    if (results[cur].out_id_ltr != -1)
        db.lev_tr_cache().to_rec(results[cur].out_id_ltr, rec);
    else
    {
        rec.unset("leveltype1");
        rec.unset("l1");
        rec.unset("leveltype2");
        rec.unset("l2");
        rec.unset("pindicator");
        rec.unset("p1");
        rec.unset("p2");
    }
}

void Cursor::to_record_datetime(Record& rec)
{
    rec.set(results[cur].out_datetime);
}

void Cursor::to_record_varcode(Record& rec)
{
    char bname[7];
    snprintf(bname, 7, "B%02d%03d",
            WR_VAR_X(results[cur].out_varcode),
            WR_VAR_Y(results[cur].out_varcode));
    rec.setc("var", bname);
}

void Cursor::query_attrs(function<void(unique_ptr<Var>)> dest)
{
    db.query_attrs(results[cur].out_id_data, results[cur].out_varcode, dest);
}

void Cursor::attr_insert(const dballe::Record& attrs)
{
    db.attr_insert(results[cur].out_id_data, results[cur].out_varcode, attrs);
}

void Cursor::attr_remove(const AttrList& qcs)
{
    db.attr_remove(results[cur].out_id_data, results[cur].out_varcode, qcs);
}

void Cursor::add_station_info(Record& rec)
{
    db.station().add_station_vars(results[cur].out_ana_id, rec);
}

unique_ptr<Cursor> Cursor::run_station_query(DB& db, const core::Query& q)
{
    unsigned int modifiers = q.get_modifiers() | DBA_DB_MODIFIER_ANAEXTRA;

    StationQueryBuilder qb(db, q, modifiers);
    qb.build();

    unique_ptr<Cursor> res(new CursorStations(db, modifiers));
    res->load(qb);
    return res;
}

unique_ptr<Cursor> Cursor::run_data_query(DB& db, const core::Query& q)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(db, q, modifiers);
    qb.build();

    unique_ptr<Cursor> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        res.reset(new CursorBest(db, modifiers));
    } else {
        res.reset(new CursorData(db, modifiers));
    }
    res->load(qb);
    return res;
}

unique_ptr<Cursor> Cursor::run_summary_query(DB& db, const core::Query& q)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on summary queries");

    SummaryQueryBuilder qb(db, q, modifiers);
    qb.build();

    unique_ptr<Cursor> res(new CursorSummary(db, modifiers));
    res->load(qb);
    return res;
}

void Cursor::run_delete_query(DB& db, const core::Query& q)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on summary queries");

    IdQueryBuilder qb(db, q, modifiers);
    qb.build();

    db.data().remove(qb);
}

void Cursor::load(const QueryBuilder& qb)
{
    db.driver().run_built_query_v6(qb, [&](sql::SQLRecordV6& rec) {
        results.append(rec);
    });
    // We are done adding, prepare the structbuf for reading
    results.ready_to_read();
}

CursorStations::~CursorStations() {}

void CursorStations::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    add_station_info(rec);
}

unsigned CursorStations::test_iterate(FILE* dump)
{
    core::Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
            to_record(r);
            fprintf(dump, "%02d %02.4f %02.4f %-10s\n",
                    r.enq("ana_id", -1),
                    r.enq("lat", 0.0),
                    r.enq("lon", 0.0),
                    r.enq("ident", ""));
        }
    }
    return count;
}

CursorData::~CursorData() {}

void CursorData::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    if (results[cur].out_id_ltr == -1)
        rec.unset("context_id");
    else
        rec.seti("context_id", results[cur].out_id_data);
    to_record_varcode(rec);
    to_record_ltr(rec);
    to_record_datetime(rec);

    auto& r = core::Record::downcast(rec);
    r.clear_vars();
    rec.set(newvar(results[cur].out_varcode, results[cur].out_value));

    if (modifiers & DBA_DB_MODIFIER_ANAEXTRA)
        add_station_info(rec);
}

unsigned CursorData::test_iterate(FILE* dump)
{
    core::Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
/*
            to_record(r);
            fprintf(dump, "%02d %06d %06d %-10s\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_LAT, 0.0),
                    r.get(DBA_KEY_LON, 0.0),
                    r.get(DBA_KEY_IDENT, ""));
                    */
        }
    }
    return count;
}

CursorSummary::~CursorSummary() {}

void CursorSummary::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    to_record_varcode(rec);
    to_record_ltr(rec);

    if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
    {
        rec.seti("context_id", results[cur].out_id_data);
        rec.set(DatetimeRange(results[cur].out_datetime, results[cur].out_datetimemax));
    }

    /*
    // Min datetime
    rec.key(DBA_KEY_YEARMIN).seti(results[cur].out_datetime.year);
    rec.key(DBA_KEY_MONTHMIN).seti(results[cur].out_datetime.month);
    rec.key(DBA_KEY_DAYMIN).seti(results[cur].out_datetime.day);
    rec.key(DBA_KEY_HOURMIN).seti(results[cur].out_datetime.hour);
    rec.key(DBA_KEY_MINUMIN).seti(results[cur].out_datetime.minute);
    rec.key(DBA_KEY_SECMIN).seti(results[cur].out_datetime.second);

    // Max datetime
    rec.key(DBA_KEY_YEARMAX).seti(out_datetime_max.year);
    rec.key(DBA_KEY_MONTHMAX).seti(out_datetime_max.month);
    rec.key(DBA_KEY_DAYMAX).seti(out_datetime_max.day);
    rec.key(DBA_KEY_HOURMAX).seti(out_datetime_max.hour);
    rec.key(DBA_KEY_MINUMAX).seti(out_datetime_max.minute);
    rec.key(DBA_KEY_SECMAX).seti(out_datetime_max.second);

    // Abuse id_data and datetime for count and min(datetime)
    rec.key(DBA_KEY_LIMIT).seti(results[cur].out_id_data);
    */
}

unsigned CursorSummary::test_iterate(FILE* dump)
{
    core::Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
            to_record(r);
            fprintf(dump, "%02d %s %03d %s %04d-%02d-%02d %02d:%02d:%02d  %04d-%02d-%02d %02d:%02d:%02d  %d\n",
                    r.enq("ana_id", -1),
                    r.enq("rep_memo", ""),
                    (int)results[cur].out_id_ltr,
                    r.enq("var", ""),
                    r.enq("yearmin", 0), r.enq("monthmin", 0), r.enq("daymin", 0),
                    r.enq("hourmin", 0), r.enq("minumin", 0), r.enq("secmin", 0),
                    r.enq("yearmax", 0), r.enq("monthmax", 0), r.enq("daymax", 0),
                    r.enq("hourmax", 0), r.enq("minumax", 0), r.enq("secmax", 0),
                    r.enq("limit", -1));
        }
    }
    return count;
}

CursorBest::~CursorBest() {}

void CursorBest::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    if (results[cur].out_id_ltr == -1)
        rec.unset("context_id");
    else
        rec.seti("context_id", results[cur].out_id_data);
    to_record_varcode(rec);
    to_record_ltr(rec);
    to_record_datetime(rec);

    auto& r = core::Record::downcast(rec);
    r.clear_vars();
    rec.set(newvar(results[cur].out_varcode, results[cur].out_value));

    if (modifiers & DBA_DB_MODIFIER_ANAEXTRA)
        add_station_info(rec);
}

unsigned CursorBest::test_iterate(FILE* dump)
{
    core::Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        /*
        if (dump)
            fprintf(dump, "%03d\n", (int)results[cur].out_id_data);
            */
    }
    return count;
}

void CursorBest::load(const QueryBuilder& qb)
{
    db::sql::Repinfo& ri = db.repinfo();
    bool first = true;
    sql::SQLRecordV6 best;

    db.driver().run_built_query_v6(qb, [&](sql::SQLRecordV6& rec) {
        // Fill priority
        rec.priority = ri.get_priority(rec.out_rep_cod);

        // Filter results keeping only those with the best priority
        if (first)
        {
            // The first record initialises 'best'
            best = rec;
            first = false;
        } else if (rec.querybest_fields_are_the_same(best)) {
            // If they match, keep the record with the highest priority
            if (rec.priority > best.priority)
                best = rec;
        } else {
            // If they don't match, write out the previous best value
            results.append(best);
            // And restart with a new candidate best record for the next batch
            best = rec;
        }
    });

    // Write out the last best value
    if (!first)
        results.append(best);

    // We are done adding, prepare the structbuf for reading
    results.ready_to_read();
}

}
}
}
