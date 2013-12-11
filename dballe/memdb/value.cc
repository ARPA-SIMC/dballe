#include "value.h"
#include "station.h"
#include "levtr.h"
#include "dballe/core/stlutils.h"
#include "dballe/core/record.h"
#include "dballe/core/varmatch.h"
#include "results.h"
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

void Value::replace(const Var& var)
{
    this->var->copy_val_only(var);
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
    stl::SetIntersection<size_t> res;
    if (by_station.search(&station, res) && by_levtr.search(&levtr, res) && by_date.search(datetime.date, res))
        for (stl::SetIntersection<size_t>::const_iterator i = res.begin(); i != res.end(); ++i)
        {
            Value* v = (*this)[*i];
            if (v && v->datetime == datetime && v->var->code() == var.code())
            {
                if (!replace)
                    throw error_consistency("cannot replace an existing value");
                v->replace(var);
                return *i;
            }
        }

    // Station not found, create it
    auto_ptr<Var> copy(new Var(var));
    size_t pos = value_add(new Value(station, levtr, datetime, copy));
    // Index it
    by_station[&station].insert(pos);
    by_levtr[&levtr].insert(pos);
    by_date[datetime.date].insert(pos);
    // And return it
    return pos;
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

void Values::erase(size_t idx)
{
    const Value& val = *(*this)[idx];
    by_station[&val.station].erase(idx);
    by_levtr[&val.levtr].erase(idx);
    by_date[val.datetime.date].erase(idx);
    value_remove(idx);
}

namespace {

struct MatchDateExact : public Match<Value>
{
    Datetime dt;
    MatchDateExact(const Datetime& dt) : dt(dt) {}
    virtual bool operator()(const Value& val) const
    {
        return val.datetime == dt;
    }
};
struct MatchDateMin : public Match<Value>
{
    Datetime dt;
    MatchDateMin(const Datetime& dt) : dt(dt) {}
    virtual bool operator()(const Value& val) const
    {
        return !(val.datetime < dt);
    }
};
struct MatchDateMax : public Match<Value>
{
    Datetime dt;
    MatchDateMax(const Datetime& dt) : dt(dt) {}
    virtual bool operator()(const Value& val) const
    {
        return !(val.datetime > dt);
    }
};
struct MatchDateMinMax : public Match<Value>
{
    Datetime dtmin;
    Datetime dtmax;
    MatchDateMinMax(const Datetime& dtmin, const Datetime& dtmax) : dtmin(dtmin), dtmax(dtmax) {}
    virtual bool operator()(const Value& val) const
    {
        return !(dtmin > val.datetime) && !(val.datetime > dtmax);
    }
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
        if (mind[0] == maxd[0] && mind[1] == maxd[1] && mind[2] == maxd[2])
        {
            Date d(mind);
            const set<size_t>* s = by_date.search(d);
            trace_query("Found exact date %04d-%02d-%02d\n", d.year, d.month, d.day);
            if (!s)
            {
                trace_query(" date not found in index, setting to the empty result set\n");
                res.set_to_empty();
                return;
            }
            if (by_date.size() == 1)
            {
                trace_query(" date matches the whole index: no point in adding a filter\n");
            } else {
                res.add(*s);
            }
            if (mind[3] == maxd[3] && mind[4] == maxd[4] && mind[5] == maxd[5])
                res.add(new MatchDateExact(mind));
            else
                res.add(new MatchDateMinMax(mind, maxd));
        } else {
            bool found;
            auto_ptr< stl::Sequences<size_t> > sequences;
            auto_ptr< Match<Value> > extra_match;

            if (maxd[0] == -1) {
                Date d(mind);
                sequences = by_date.search_from(mind, found);
                extra_match.reset(new MatchDateMin(mind));
                trace_query("Found date min %04d-%02d-%02d\n", d.year, d.month, d.day);
            } else if (mind[0] == -1) {
                // FIXME: we need to add 1 second to maxd, as it is right extreme excluded
                Date d(maxd);
                sequences = by_date.search_to(d, found);
                extra_match.reset(new MatchDateMax(maxd));
                trace_query("Found date max %04d-%02d-%02d\n", d.year, d.month, d.day);
            } else {
                // FIXME: we need to add 1 second to maxd, as it is right extreme excluded
                Date dmin(mind);
                Date dmax(maxd);
                sequences = by_date.search_between(dmin, dmax, found);
                extra_match.reset(new MatchDateMinMax(mind, maxd));
                trace_query("Found date range %04d-%02d-%02d to %04d-%02d-%02d\n",
                        dmin.year, dmin.month, dmin.day, dmax.year, dmax.month, dmax.day);
            }

            if (!found)
            {
                trace_query(" no matching dates found, setting to the empty result set\n");
                res.set_to_empty();
                return;
            }

            if (sequences.get())
                res.add_union(sequences);
            else
                trace_query(" date range matches the whole index: no point in adding a filter\n");

            res.add(extra_match.release());
        }
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        trace_query("Found varcode=%s\n", val);
        res.add(new match::Varcode<Value>(descriptor_code(val)));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        set<Varcode> codes;
        size_t pos;
        size_t len;
        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
            codes.insert(WR_STRING_TO_VAR(val + pos + 1));
        res.add(new match::Varcodes<Value>(codes));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_DATA_FILTER))
    {
        trace_query("Found data_filter=%s\n", val);
        res.add(new match::DataFilter<Value>(val));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_ATTR_FILTER))
    {
        trace_query("Found attr_filter=%s\n", val);
        res.add(new match::AttrFilter<Value>(val));
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
#include "results.tcc"

namespace dballe {
namespace memdb {
template class Results<Value>;
}
}
