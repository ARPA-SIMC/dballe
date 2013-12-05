#include "value.h"
#include "station.h"
#include "levtr.h"
#include "dballe/core/stlutils.h"
#include "dballe/core/record.h"
#include "dballe/core/varmatch.h"
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

void Value::dump(FILE* out) const
{
    stringstream buf;
    buf << station.id
        << "\t" << levtr.level
        << "\t" << levtr.trange
        << "\t" << datetime
        << "\t";
    var->print_without_attrs(buf);
    fputs(buf.str().c_str(), out);
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
    if (by_station.search(&station, res) && by_levtr.search(&levtr, res) && by_date.search(datetime.date, res))
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
    by_date[datetime.date].insert(pos);
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
    if (!by_station.search(&station, res) || !by_levtr.search(&levtr, res) || !by_date.search(datetime.date, res))
        return false;

    for (stl::SetIntersection<size_t>::const_iterator i = res.begin(); i != res.end(); ++i)
    {
        Value* v = (*this)[*i];
        if (v && v->datetime == datetime && v->var->code() == code)
        {
            by_station[&station].erase(*i);
            by_levtr[&levtr].erase(*i);
            by_date[datetime.date].erase(*i);
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

struct MatchDataFilter : public Match<Value>
{
    Varmatch* match;

    MatchDataFilter(const std::string& expr) : match(Varmatch::parse(expr).release()) {}
    ~MatchDataFilter() { delete match; }

    virtual bool operator()(const Value& val) const
    {
        return (*match)(*val.var);
    }

private:
    MatchDataFilter(const MatchDataFilter&);
    MatchDataFilter& operator=(const MatchDataFilter&);
};

struct MatchAttrFilter : public Match<Value>
{
    Varmatch* match;

    MatchAttrFilter(const std::string& expr) : match(Varmatch::parse(expr).release()) {}
    ~MatchAttrFilter() { delete match; }

    virtual bool operator()(const Value& val) const
    {
        const Var* a = val.var->enqa(match->code);
        if (!a) return false;
        return (*match)(*a);
    }

private:
    MatchAttrFilter(const MatchAttrFilter&);
    MatchAttrFilter& operator=(const MatchAttrFilter&);
};


}

void Values::query(const Record& rec, Results<Station>& stations, Results<LevTr>& levtrs, Results<Value>& res) const
{
    if (const char* data_id = rec.key_peek_value(DBA_KEY_CONTEXT_ID))
    {
        trace_query("Found data_id %s\n", data_id);
        size_t pos = strtoul(data_id, 0, 10);
        if (pos >= 0 && pos < values.size() && values[pos])
        {
            trace_query(" intersect with %zu\n", pos);
            res.add(pos);
        } else {
            trace_query(" set to empty result set\n");
            res.set_to_empty();
            return;
        }
    }

    if (!stations.is_select_all())
    {
        trace_query("Adding selected stations to strategy\n");
        match::SequenceBuilder<Station> lookup_by_station(by_station);
        stations.copy_valptrs_to(stl::trivial_inserter(lookup_by_station));
        if (!lookup_by_station.found_items_in_index())
        {
            trace_query(" no matching stations found, setting empty result\n");
            res.set_to_empty();
            return;
        }
        // OR the results together into a single sequence
        res.add_union(lookup_by_station.release_sequences());
    }

    if (!levtrs.is_select_all())
    {
        trace_query("Adding selected levtrs to strategy\n");
        match::SequenceBuilder<LevTr> lookup_by_levtr(by_levtr);
        levtrs.copy_valptrs_to(stl::trivial_inserter(lookup_by_levtr));
        if (!lookup_by_levtr.found_items_in_index())
        {
            trace_query(" no matching levtrs found, setting empty result\n");
            res.set_to_empty();
            return;
        }
        // OR the results together into a single sequence
        res.add_union(lookup_by_levtr.release_sequences());
    }

    int mind[6], maxd[6];
    rec.parse_date_extremes(mind, maxd);
    if (mind[0] != -1 || maxd[0] != -1)
    {
        auto_ptr< stl::Sequences<size_t> > sequences(new stl::Sequences<size_t>);
        bool found = false;
        if (mind[0] == maxd[0] && mind[1] == maxd[1] && mind[2] == maxd[2])
        {
            Date d(mind);
            found |= by_date.search(d, *sequences);
            trace_query("Found exact date %04d-%02d-%02d\n", d.year, d.month, d.day);
        } else if (mind[0] == -1) {
            Date d(maxd);
            found |= by_date.search_to(d, *sequences);
            trace_query("Found date max %04d-%02d-%02d\n", d.year, d.month, d.day);
        } else if (maxd[0] == -1) {
            Date d(mind);
            found |= by_date.search_from(d, *sequences);
            IF_TRACE_QUERY {
                for (Index<Date>::const_iterator i = by_date.lower_bound(d); i != by_date.end(); ++i)
                    trace_query(" Selecting %04d-%02d-%02d %zd elements\n", i->first.year, i->first.month, i->first.day, i->second.size());
            }
            trace_query("Found date min %04d-%02d-%02d\n", d.year, d.month, d.day);
        } else {
            Date dmin(mind);
            Date dmax(maxd);
            found |= by_date.search_between(dmin, dmax, *sequences);
            trace_query("Found date range %04d-%02d-%02d to %04d-%02d-%02d\n",
                    dmin.year, dmin.month, dmin.day, dmax.year, dmax.month, dmax.day);
        }
        if (!found)
        {
            trace_query("No matching dates found, setting to empty result set\n");
            res.set_to_empty();
            return;
        }
        // OR the results together into a single sequence
        res.add_union(sequences);

#warning TODO: also add a matcher to match datetimes fully, since the index only selects on dates
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        trace_query("Found varcode=%s\n", val);
        res.add(new MatchVarcode(descriptor_code(val)));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        set<Varcode> codes;
        size_t pos;
        size_t len;
        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
            codes.insert(WR_STRING_TO_VAR(val + pos + 1));
        res.add(new MatchVarcodes(codes));
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

    if (const char* val = rec.key_peek_value(DBA_KEY_DATA_FILTER))
    {
        trace_query("Found data_filter=%s\n", val);
        res.add(new MatchDataFilter(val));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_ATTR_FILTER))
    {
        trace_query("Found attr_filter=%s\n", val);
        res.add(new MatchAttrFilter(val));
    }

    //trace_query("Strategy activated, %zu results\n", res.size());
}

void Values::dump(FILE* out) const
{
    fprintf(out, "Values:\n");
    for (size_t pos = 0; pos < values.size(); ++pos)
    {
        if (values[pos])
        {
            fprintf(out, " %4zu: ", pos);
            values[pos]->dump(out);
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
