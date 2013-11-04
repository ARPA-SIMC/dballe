#include "value.h"
#include "station.h"
#include "levtr.h"
#include "dballe/core/stlutils.h"
#include "query.h"
#include <iomanip>
#include <ostream>
#include <sstream>

using namespace std;
using namespace wreport;

namespace dballe {
namespace memdb {

Value::~Value()
{
    delete var;
}

void Value::replace(std::auto_ptr<Var> var)
{
    delete this->var;
    this->var = var.release();
}

void Values::clear()
{
    by_station.clear();
    by_levtr.clear();
    by_date.clear();
    ValueStorage<Value>::clear();
}

size_t Values::insert(
        const Station& station, const LevTr& levtr,
        const Datetime& datetime, std::auto_ptr<Var> var, bool replace)
{
    stl::SetIntersection<size_t> res;
    if (by_station.search(&station, res) && by_levtr.search(&levtr, res) && by_date.search(datetime, res))
        for (stl::SetIntersection<size_t>::const_iterator i = res.begin(); i != res.end(); ++i)
        {
            Value* v = (*this)[*i];
            if (v && v->datetime == datetime && v->var->code() == var->code())
            {
                if (!replace)
                    throw error_consistency("cannot replace an existing value");
                v->replace(var);
                return *i;
            }
        }

    // Station not found, create it
    size_t pos = value_add(new Value(station, levtr, datetime, var));
    // Index it
    by_station[&station].insert(pos);
    by_levtr[&levtr].insert(pos);
    by_date[datetime].insert(pos);
    // And return it
    return pos;

}

size_t Values::insert(
        const Station& station, const LevTr& levtr,
        const Datetime& datetime, const Var& var, bool replace)
{
    auto_ptr<Var> copy(new Var(var));
    return insert(station, levtr, datetime, copy, replace);
}

bool Values::remove(const Station& station, const LevTr& levtr, const Datetime& datetime, Varcode code)
{
    stl::SetIntersection<size_t> res;
    if (!by_station.search(&station, res) || !by_levtr.search(&levtr, res) || !by_date.search(datetime, res))
        return false;

    for (stl::SetIntersection<size_t>::const_iterator i = res.begin(); i != res.end(); ++i)
    {
        Value* v = (*this)[*i];
        if (v && v->datetime == datetime && v->var->code() == code)
        {
            by_station[&station].erase(*i);
            by_levtr[&levtr].erase(*i);
            by_date[datetime].erase(*i);
            value_remove(*i);
            return true;
        }
    }
    return false;
}

void Values::query(const Record& rec, const Results<Station>& stations, Results<Value>& res) const
{
    match::Strategy<Value> strategy;

    if (!stations.is_select_all())
    {
        bool found = false;
        for (Results<Station>::const_iterator i = stations.begin(); i != stations.end(); ++i)
        {
            Index<const Station*>::const_iterator ids = by_station.find(&*i);
            if (ids == by_station.end())
                continue;
            strategy.add(ids->second);
            found = true;
        }
        if (!found)
        {
            res.set_to_empty();
            return;
        }
    }

#if 0
    if (query_data_id != MISSING_INT)
    {
        // Skip arbitrary limits on id_lev_tr if data_id is queried, since we
        // must allow to select either a station or a data value

        //TRACE("found %s: adding %s. val is %d\n", info(key)->desc, sql, *out);
        sql_where.append_listf("d.id=%d", query_data_id);
    } else {
        if (query_station_vars)
            sql_where.append_list("d.id_lev_tr == -1");
        else
            sql_where.append_list("d.id_lev_tr != -1");
    }

bool QueryBuilder::add_dt_where(const char* tbl)
{
    if (rec.get(DBA_KEY_LEVELTYPE1, 0) == 257)
        return false;

    bool found = false;
    int minvalues[6], maxvalues[6];
    rec.parse_date_extremes(minvalues, maxvalues);

    if (minvalues[0] != -1 || maxvalues[0] != -1)
    {
        if (memcmp(minvalues, maxvalues, 6 * sizeof(int)) == 0)
        {
            /* Add constraint on the exact date interval */
            qargs.sel_dtmin.year = minvalues[0];
            qargs.sel_dtmin.month = minvalues[1];
            qargs.sel_dtmin.day = minvalues[2];
            qargs.sel_dtmin.hour = minvalues[3];
            qargs.sel_dtmin.minute = minvalues[4];
            qargs.sel_dtmin.second = minvalues[5];
            qargs.sel_dtmin.fraction = 0;
            sql_where.append_listf("%s.datetime=?", tbl);
            TRACE("found exact time: adding AND %s.datetime={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                    tbl, minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
            stm.bind_in(qargs.input_seq++, qargs.sel_dtmin);
            found = true;
        }
        else
        {
            if (minvalues[0] != -1)
            {
                /* Add constraint on the minimum date interval */
                qargs.sel_dtmin.year = minvalues[0];
                qargs.sel_dtmin.month = minvalues[1];
                qargs.sel_dtmin.day = minvalues[2];
                qargs.sel_dtmin.hour = minvalues[3];
                qargs.sel_dtmin.minute = minvalues[4];
                qargs.sel_dtmin.second = minvalues[5];
                qargs.sel_dtmin.fraction = 0;
                sql_where.append_listf("%s.datetime>=?", tbl);
                TRACE("found min time: adding AND %s.datetime>={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                    tbl, minvalues[0], minvalues[1], minvalues[2], minvalues[3], minvalues[4], minvalues[5]);
                stm.bind_in(qargs.input_seq++, qargs.sel_dtmin);
                found = true;
            }
            if (maxvalues[0] != -1)
            {
                qargs.sel_dtmax.year = maxvalues[0];
                qargs.sel_dtmax.month = maxvalues[1];
                qargs.sel_dtmax.day = maxvalues[2];
                qargs.sel_dtmax.hour = maxvalues[3];
                qargs.sel_dtmax.minute = maxvalues[4];
                qargs.sel_dtmax.second = maxvalues[5];
                qargs.sel_dtmax.fraction = 0;
                sql_where.append_listf("%s.datetime<=?", tbl);
                TRACE("found max time: adding AND %s.datetime<={ts '%04d-%02d-%02d %02d:%02d:%02d.000'}\n",
                        tbl, maxvalues[0], maxvalues[1], maxvalues[2], maxvalues[3], maxvalues[4], maxvalues[5]);
                stm.bind_in(qargs.input_seq++, qargs.sel_dtmax);
                found = true;
            }
        }
    }

    return found;
}

bool QueryBuilder::add_ltr_where(const char* tbl)
{
    if (query_station_vars) return false;

    Constraints c(rec, tbl, sql_where);
    c.add_int(DBA_KEY_LEVELTYPE1, "%s.ltype1=%d");
    c.add_int(DBA_KEY_L1, "%s.l1=%d");
    c.add_int(DBA_KEY_LEVELTYPE2, "%s.ltype2=%d");
    c.add_int(DBA_KEY_L2, "%s.l2=%d");
    c.add_int(DBA_KEY_PINDICATOR, "%s.ptype=%d");
    c.add_int(DBA_KEY_P1, "%s.p1=%d");
    c.add_int(DBA_KEY_P2, "%s.p2=%d");
    return c.found;
}

bool QueryBuilder::add_varcode_where(const char* tbl)
{
    bool found = false;

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        sql_where.append_listf("%s.id_var=%d", tbl, descriptor_code(val));
        TRACE("found b: adding AND %s.id_var=%d [from %s]\n", tbl, (int)descriptor_code(val), val);
        found = true;
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        size_t pos;
        size_t len;
        sql_where.append_listf("%s.id_var IN (", tbl);
        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
        {
            Varcode code = WR_STRING_TO_VAR(val + pos + 1);
            if (pos == 0)
                sql_where.appendf("%d", code);
            else
                sql_where.appendf(",%d", code);
        }
        sql_where.append(")");
        TRACE("found blist: adding AND %s.id_var IN (%s)\n", tbl, val);
        found = true;
    }

    return found;
}

bool QueryBuilder::add_repinfo_where(const char* tbl)
{
    Constraints c(rec, tbl, sql_where);
 
    if (rec.key_peek(DBA_KEY_PRIORITY) || rec.key_peek(DBA_KEY_PRIOMIN) || rec.key_peek(DBA_KEY_PRIOMAX))
    {
        // Filter the repinfo cache and build a IN query
        std::vector<int> ids = db.repinfo().ids_by_prio(rec);
        if (ids.empty())
        {
            // No repinfo matches, so we just introduce a false value
            sql_where.append_list("1=0");
        } else {
            sql_where.append_listf("%s.id_report IN (", tbl);
            for (std::vector<int>::const_iterator i = ids.begin(); i != ids.end(); ++i)
            {
                if (i == ids.begin())
                    sql_where.appendf("%d", *i);
                else
                    sql_where.appendf(",%d", *i);
            }
            sql_where.append(")");
        }
        c.found = true;
    }

    // rep_memo has priority over rep_cod
    if (const char* val = rec.key_peek_value(DBA_KEY_REP_MEMO))
    {
        int src_val = db.repinfo().get_id(val);
        sql_where.append_listf("%s.id_report=%d", tbl, src_val);
        TRACE("found rep_memo %s: adding AND %s.id_report=%d\n", val, tbl, (int)src_val);
        c.found = true;
    } else
        c.add_int(DBA_KEY_REP_COD, "%s.id_report=%d");

    return c.found;
}

bool QueryBuilder::add_datafilter_where(const char* tbl)
{
    const char* val = rec.key_peek_value(DBA_KEY_DATA_FILTER);
    if (!val) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(val, &op, &value, &value1);

    sql_where.append_listf("%s.id_var=%d", tbl, (int)info->var);

    if (value[0] == '\'')
        if (value1 == NULL)
            sql_where.append_listf("%s.value%s%s", tbl, op, value);
        else
            sql_where.append_listf("%s.value BETWEEN %s AND %s", tbl, value, value1);
    else
    {
        const char* type = (db.conn->server_type == MYSQL) ? "SIGNED" : "INT";
        if (value1 == NULL)
            sql_where.append_listf("CAST(%s.value AS %s)%s%s", tbl, type, op, value);
        else
            sql_where.append_listf("CAST(%s.value AS %s) BETWEEN %s AND %s", tbl, type, value, value1);
    }

    return true;
}

bool QueryBuilder::add_attrfilter_where(const char* tbl)
{
    const char* val = rec.key_peek_value(DBA_KEY_ATTR_FILTER);
    if (!val) return false;

    const char *op, *value, *value1;
    Varinfo info = decode_data_filter(val, &op, &value, &value1);

    sql_from.appendf(" JOIN attr %s_atf ON %s.id=%s_atf.id_data AND %s_atf.type=%d", tbl, tbl, tbl, tbl, info->var);
    if (value[0] == '\'')
        if (value1 == NULL)
            sql_where.append_listf("%s_atf.value%s%s", tbl, op, value);
        else
            sql_where.append_listf("%s_atf.value BETWEEN %s AND %s", tbl, value, value1);
    else
    {
        const char* type = (db.conn->server_type == MYSQL) ? "SIGNED" : "INT";
        if (value1 == NULL)
            sql_where.append_listf("CAST(%s_atf.value AS %s)%s%s", tbl, type, op, value);
        else
            sql_where.append_listf("CAST(%s_atf.value AS %s) BETWEEN %s AND %s", tbl, type, value, value1);
    }
    return true;
}
#endif
    strategy.activate(res);

}

void Values::dump(FILE* out) const
{
    fprintf(out, "Values:\n");
    for (size_t pos = 0; pos < values.size(); ++pos)
    {
        if (values[pos])
        {
            stringstream buf;
            buf << values[pos]->levtr.level
                << "\t" << values[pos]->levtr.trange
                << "\t" << values[pos]->datetime
                << "\t";
            values[pos]->var->print_without_attrs(buf);

            fprintf(out, " %4zu: %4zu\t%s", pos, values[pos]->station.id, buf.str().c_str());
            // TODO: print attrs
        } else
            fprintf(out, " %4zu: (empty)\n", pos);
    }
#if 0
    fprintf(out, " coord index:\n");
    for (Index<Coord>::const_iterator i = by_coord.begin(); i != by_coord.end(); ++i)
    {
        fprintf(out, "  %d %d -> ", i->first.lat, i->first.lon);
        i->second.dump(out);
        putc('\n', out);
    }
    fprintf(out, " ident index:\n");
    for (Index<string>::const_iterator i = by_ident.begin(); i != by_ident.end(); ++i)
    {
        fprintf(out, "  %s -> \n", i->first.c_str());
        i->second.dump(out);
        putc('\n', out);
    }
#endif
};

template class Index<const Station*>;
template class ValueStorage<Value>;

}
}

#include "core.tcc"
#include "query.tcc"

namespace dballe {
namespace memdb {
template class Results<Value>;
}
}
