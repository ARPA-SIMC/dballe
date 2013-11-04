#include "value.h"
#include "station.h"
#include "levtr.h"
#include "dballe/core/stlutils.h"
#include "dballe/core/record.h"
#include "query.h"
#include <iomanip>
#include <ostream>
#include <sstream>
#include <cstdlib>
#include <cstring>

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

namespace {

struct MatchVarcode : public Match<Value>
{
    Varcode code;

    MatchVarcode(Varcode code) : code(code) {}
    virtual bool operator()(const Value& val) const
    {
        return val.var->code() == code;
    }
};

struct MatchVarcodes : public Match<Value>
{
    std::set<Varcode> codes;

    MatchVarcodes(std::set<Varcode> codes) : codes(codes) {}
    virtual bool operator()(const Value& val) const
    {
        return codes.find(val.var->code()) != codes.end();
    }
};

}

void Values::query(const Record& rec, const Results<Station>& stations, const Results<LevTr>& tranges, Results<Value>& res) const
{
    if (const char* data_id = rec.key_peek_value(DBA_KEY_CONTEXT_ID))
    {
        trace_query("Found data_id %s\n", data_id);
        size_t pos = strtoul(data_id, 0, 10);
        if (pos >= 0 && pos < values.size() && values[pos])
        {
            trace_query(" intersect with %zu\n", pos);
            res.intersect(pos);
        } else {
            trace_query(" set to empty result set\n");
            res.set_to_empty();
            return;
        }
    }

    match::Strategy<Value> strategy;

    if (!stations.is_select_all())
    {
        bool found = false;
        for (Results<Station>::const_iterator i = stations.begin(); i != stations.end(); ++i)
            found |= strategy.add(by_station, &*i);
        if (!found)
        {
            res.set_to_empty();
            return;
        }
    }

    if (!tranges.is_select_all())
    {
        bool found = false;
        for (Results<LevTr>::const_iterator i = tranges.begin(); i != tranges.end(); ++i)
            found |= strategy.add(by_levtr, &*i);
        if (!found)
        {
            res.set_to_empty();
            return;
        }
    }

    int mind[6], maxd[6];
    rec.parse_date_extremes(mind, maxd);
    if (mind[0] != -1 || maxd[0] != -1)
    {
        if (mind[0] == maxd[0] && mind[1] == maxd[1] && mind[2] == maxd[2])
        {
            Date d(mind);
            strategy.add(by_date, d);
        } else if (mind[0] == -1) {
            Date d(maxd);
            strategy.add_until(by_date, by_date.upper_bound(d));
        } else if (maxd[0] == -1) {
            Date d(mind);
            strategy.add_since(by_date, by_date.lower_bound(d));
        } else {
            Date dmin(mind);
            Date dmax(maxd);
            strategy.add(by_date, dmin, dmax);
        }
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        trace_query("Found varcode=%s\n", val);
        strategy.add(new MatchVarcode(descriptor_code(val)));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        set<Varcode> codes;
        size_t pos;
        size_t len;
        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
            codes.insert(WR_STRING_TO_VAR(val + pos + 1));
        strategy.add(new MatchVarcodes(codes));
    }

#if 0
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
#endif

#if 0
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
