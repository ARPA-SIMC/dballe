#include "data.h"
#if 0
#include "record-access.h"
#include "query.h"
#include "var.h"
#include "aliases.h"
#include "defs.h"
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#endif

using namespace wreport;

namespace dballe {
namespace core {

#if 0
Data::Data()
{
}
Record::Record(const Record& rec)
    : priomin(rec.priomin), priomax(rec.priomax), mobile(rec.mobile),
      station(rec.station), latrange(rec.latrange), lonrange(rec.lonrange),
      datetime(rec.datetime), level(rec.level), trange(rec.trange),
      var(rec.var), varlist(rec.varlist), query(rec.query),
      ana_filter(rec.ana_filter), data_filter(rec.data_filter), attr_filter(rec.attr_filter),
      count(rec.count)
{
    // Copy the variable list
    for (auto& var: rec.m_vars)
        m_vars.push_back(new Var(*var));
}

Record::Record(Record&& rec)
    : priomin(rec.priomin), priomax(rec.priomax), mobile(rec.mobile),
      station(rec.station), latrange(rec.latrange), lonrange(rec.lonrange),
      datetime(rec.datetime), level(rec.level), trange(rec.trange),
      var(rec.var), varlist(std::move(rec.varlist)), query(std::move(rec.query)),
      ana_filter(std::move(rec.ana_filter)), data_filter(std::move(rec.data_filter)), attr_filter(std::move(rec.attr_filter)),
      count(rec.count), m_vars(std::move(rec.m_vars))
{
}
#endif

Data::~Data()
{
}

#if 0
Record& Record::operator=(const Record& rec)
{
    // Prevent self-copying
    if (this == &rec) return *this;

    priomin = rec.priomin;
    priomax = rec.priomax;
    mobile = rec.mobile;
    station = rec.station;
    latrange = rec.latrange;
    lonrange = rec.lonrange;
    datetime = rec.datetime;
    level = rec.level;
    trange = rec.trange;
    var = rec.var;
    varlist = rec.varlist;
    query = rec.query;
    ana_filter = rec.ana_filter;
    data_filter = rec.data_filter;
    attr_filter = rec.attr_filter;
    count = rec.count;

    // Copy the variable list
    clear_vars();
    for (const auto& var: rec.m_vars)
        m_vars.push_back(new Var(*var));

    return *this;
}

Record& Record::operator=(Record&& rec)
{
    // Prevent self-copying
    if (this == &rec) return *this;

    priomin = rec.priomin;
    priomax = rec.priomax;
    mobile = rec.mobile;
    station = rec.station;
    latrange = rec.latrange;
    lonrange = rec.lonrange;
    datetime = rec.datetime;
    level = rec.level;
    trange = rec.trange;
    var = rec.var;
    varlist = std::move(rec.varlist);
    query = std::move(rec.query);
    ana_filter = std::move(rec.ana_filter);
    data_filter = std::move(rec.data_filter);
    attr_filter = std::move(rec.attr_filter);
    count = rec.count;

    // Copy the variable list
    clear_vars();
    m_vars = std::move(rec.m_vars);

    return *this;
}

unique_ptr<dballe::Record> Record::clone() const
{
    return unique_ptr<dballe::Record>(new Record(*this));
}
#endif

const Data& Data::downcast(const dballe::Data& data)
{
    const Data* ptr = dynamic_cast<const Data*>(&data);
    if (!ptr)
        throw error_consistency("data given is not a core::Data");
    return *ptr;
}

Data& Data::downcast(dballe::Data& data)
{
    Data* ptr = dynamic_cast<Data*>(&data);
    if (!ptr)
        throw error_consistency("data given is not a core::Data");
    return *ptr;
}

#if 0
wreport::Var& Record::obtain(wreport::Varcode key)
{
    int pos = find_item(key);
    if (pos == -1)
    {
        // Insertion sort the new variable

        // Enlarge the buffer
        m_vars.resize(m_vars.size() + 1);

        /* Insertionsort.  Crude, but our datasets should be too small for an
         * RB-Tree to be worth it */
        for (pos = m_vars.size() - 1; pos > 0; --pos)
            if (m_vars[pos - 1]->code() > key)
                m_vars[pos] = m_vars[pos - 1];
            else
                break;
        m_vars[pos] = newvar(key).release();
    }
    return *m_vars[pos];
}

void Record::unset_var(wreport::Varcode code)
{
    int pos = find_item(code);
    if (pos != -1)
    {
        delete m_vars[pos];
        m_vars.erase(m_vars.begin() + pos);
    }
}

void Record::set_datetime(const Datetime& dt)
{
    if (dt.is_missing())
    {
        datetime = DatetimeRange();
    } else {
        datetime.min = datetime.max = dt;
    }
}

void Record::set_datetimerange(const DatetimeRange& range)
{
    datetime = range;
}

void Record::set_level(const Level& lev)
{
    level = lev;
}

void Record::set_trange(const Trange& tr)
{
    trange = tr;
}

void Record::set_var(const wreport::Var& var)
{
    if (var.isset())
        obtain(var.code()).setval(var);
    else
        unset_var(var.code());
}

void Record::set_var_acquire(std::unique_ptr<wreport::Var>&& var)
{
    int pos = find_item(var->code());
    if (pos == -1)
    {
        // Insertion sort the new variable

        // Enlarge the buffer
        m_vars.resize(m_vars.size() + 1);

        /* Insertionsort.  Crude, but our datasets should be too small for an
         * RB-Tree to be worth it */
        for (pos = m_vars.size() - 1; pos > 0; --pos)
            if (m_vars[pos - 1]->code() > var->code())
                m_vars[pos] = m_vars[pos - 1];
            else
                break;
    } else
        delete m_vars[pos];
    m_vars[pos] = var.release();
}

void Record::set_latrange(const LatRange& lr)
{
    latrange = lr;
}

void Record::set_lonrange(const LonRange& lr)
{
    lonrange = lr;
}

bool Record::equals(const Record& rec) const
{
    if (std::tie(priomin, priomax, mobile, station, latrange, lonrange, datetime, level, trange, var, varlist, query, ana_filter, data_filter, attr_filter, count) != std::tie(rec.priomin, rec.priomax, rec.mobile, rec.station, rec.latrange, rec.lonrange, rec.datetime, rec.level, rec.trange, rec.var, rec.varlist, rec.query, rec.ana_filter, rec.data_filter, rec.attr_filter, rec.count))
        return false;

    // Compare the variables
    vector<Var*>::const_iterator i1 = m_vars.begin();
    vector<Var*>::const_iterator i2 = rec.m_vars.begin();
    for ( ; i1 != m_vars.end() && i2 != rec.m_vars.end(); ++i1, ++i2)
        if (**i1 != **i2) return false;
    if (i1 != m_vars.end() || i2 != rec.m_vars.end())
        return false;

    return true;
}
#endif

bool Data::operator==(const dballe::Data& other) const
{
    const auto& o = downcast(other);
    return std::tie(station, datetime, level, trange, values) == std::tie(o.station, o.datetime, o.level, o.trange, o.values);
}

bool Data::operator!=(const dballe::Data& other) const
{
    const auto& o = downcast(other);
    return std::tie(station, datetime, level, trange, values) != std::tie(o.station, o.datetime, o.level, o.trange, o.values);
}

void Data::clear_ids()
{
    station.id = MISSING_INT;
    values.clear_ids();
}

void Data::clear_vars()
{
    values.clear();
}

void Data::clear()
{
    station = DBStation();
    datetime = Datetime();
    level = Level();
    trange = Trange();
    values.clear();
}

#if 0
int Record::find_item(Varcode code) const noexcept
{
	/* Binary search */
	int low = 0, high = m_vars.size() - 1;
	while (low <= high)
	{
		int middle = low + (high - low)/2;
		int cmp = (int)code - (int)m_vars[middle]->code();
		if (cmp < 0)
			high = middle - 1;
		else if (cmp > 0)
			low = middle + 1;
		else
			return middle;
	}
	return -1;
}

Var& Record::get_item(Varcode code)
{
	int pos = find_item(code);
	if (pos == -1)
		error_notfound::throwf("looking for parameter \"B%02d%03d\"",
			WR_VAR_X(code), WR_VAR_Y(code));
	return *m_vars[pos];
}

const Var& Record::get_item(Varcode code) const
{
	int pos = find_item(code);
	if (pos == -1)
		error_notfound::throwf("looking for parameter \"B%02d%03d\"",
			WR_VAR_X(code), WR_VAR_Y(code));
	return *m_vars[pos];
}

void Record::remove_item(Varcode code)
{
	int pos = find_item(code);
	if (pos == -1) return;
	delete m_vars[pos];
	m_vars.erase(m_vars.begin() + pos);
}
#endif

#if 0
void Record::add(const dballe::Record& rec)
{
    const auto& source = downcast(rec);

	// Add the keyword table
	for (int i = 0; i < KEYWORD_TABLE_SIZE; i++)
	{
		if (source.keydata[i] != NULL)
		{
			if (keydata[i] != NULL)
				*keydata[i] = *source.keydata[i];
			else
				keydata[i] = new Var(*source.keydata[i]);
		}
	}

	// Add the variables list
	vector<Var*>::const_iterator src = source.m_vars.begin();
	size_t dst = 0;
	while (src != source.m_vars.end() && dst < m_vars.size())
	{
		if ((*src)->code() < m_vars[dst]->code())
		{
			// Insert
			m_vars.insert(m_vars.begin() + dst, new Var(**src));
			++src;
		} else if ((*src)->code() == m_vars[dst]->code()) {
			// Overwrite
			*m_vars[dst] = **src;
			++dst;
			++src;
		} else {
			++dst;
		}
	}
	// Append the remaining source vars
	for ( ; src != source.m_vars.end(); ++src)
		m_vars.push_back(new Var(**src));
}
#endif

#if 0
void Record::set_to_difference(const Record& source1, const Record& source2)
{
    // Copy the keyword table
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (!source2.keydata[i])
        {
            // Has been deleted: skip
            delete keydata[i];
            keydata[i] = nullptr;
        } else if (!source1.keydata[i]) {
            // Has been added in source2: add
            *keydata[i] = *source2.keydata[i];
        } else if (source1.keydata[i] != source2.keydata[i]) {
            // Has been changed in source2: add
            *keydata[i] = *source2.keydata[i];
        } else {
            // Has not been changed: skip
            delete keydata[i];
            keydata[i] = nullptr;
        }
    }

	// Copy the variables list
	clear_vars();

	vector<Var*>::const_iterator s1 = source1.m_vars.begin();
	vector<Var*>::const_iterator s2 = source2.m_vars.begin();
	while (s1 != source1.m_vars.end() && s2 != source2.m_vars.end())
	{
		if ((*s1)->code() < (*s2)->code())
			++s1;
		else if ((*s1)->code() == (*s2)->code())
		{
			if (**s1 != **s2)
				m_vars.push_back(new Var(**s2));
			++s1;
			++s2;
		}
		else if ((*s2)->code() < (*s1)->code())
		{
			m_vars.push_back(new Var(**s2));
			++s2;
		}
	}
	for ( ; s2 != source2.m_vars.end(); ++s2)
		m_vars.push_back(new Var(**s2));
}
#endif

#if 0
bool Record::contains(const dballe::Record& rec) const
{
    const auto& subset = downcast(rec);

	// Compare the keyword tables
	for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
	{
		if (subset.keydata[i] != NULL && keydata[i] == NULL)
			return false;
		if (subset.keydata[i] != NULL && keydata[i] != NULL
		 && *subset.keydata[i] != *keydata[i])
			return false;
	}

	// Compare the values
	vector<Var*>::const_iterator s1 = m_vars.begin();
	vector<Var*>::const_iterator s2 = subset.m_vars.begin();
	while (s1 != m_vars.end() && s2 != subset.m_vars.end())
	{
		if ((*s1)->code() < (*s2)->code())
			// s1 has a value not in s2
			++s1;
		else if ((*s1)->code() == (*s2)->code())
		{
			// they both have the same values
			if (**s1 != **s2)
				return false;
			++s1;
			++s2;
		}
		else if ((*s2)->code() < (*s1)->code())
		{
			// s2 has a value not in s1
			return false;
		}
	}
	return true;
}
#endif

#if 0
void Record::foreach_key_ref(std::function<void(const char*, const wreport::Var&)> dest) const
{
    // Generate keys
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (keydata[i] == NULL) continue;
        if (!keydata[i]->isset()) continue;
        dest(keyword_name((dba_keyword)i), *keydata[i]);
    }
    // Generate variables
    string varcode;
    for (const auto& i: m_vars)
    {
        varcode = varcode_format(i->code());
        dest(varcode.c_str(), *i);
    }
}

void Record::foreach_key_copy(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const
{
    // Generate keys
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (keydata[i] == NULL) continue;
        if (!keydata[i]->isset()) continue;
        dest(keyword_name((dba_keyword)i), move(newvar(*keydata[i])));
    }
    // Generate variables
    string varcode;
    for (const auto& i: m_vars)
    {
        varcode = varcode_format(i->code());
        dest(varcode.c_str(), move(newvar(*i)));
    }
}
#endif

#if 0
bool Record::iter_keys(std::function<bool(dba_keyword, const wreport::Var&)> f) const
{
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (keydata[i] == NULL) continue;
        if (!f((dba_keyword)i, *keydata[i])) return false;
    }
    return true;
}
#endif

#if 0
const std::vector<wreport::Var*>& Record::vars() const
{
    return m_vars;
}

Coords Record::get_coords() const
{
    return station.coords;
}

Ident Record::get_ident() const
{
    return station.ident;
}

Level Record::get_level() const
{
    return level;
}

Trange Record::get_trange() const
{
    return trange;
}

Datetime Record::get_datetime() const
{
    if (datetime.min != datetime.max)
        return Datetime();

    Datetime res = datetime.min;
    res.set_lower_bound();
    return res;
}

DatetimeRange Record::get_datetimerange() const
{
    DatetimeRange res = datetime;
    res.min.set_lower_bound();
    res.max.set_upper_bound();
    return res;
}

Station Record::get_station() const
{
    return station;
}

DBStation Record::get_dbstation() const
{
    return station;
}

const wreport::Var* Record::get_var(wreport::Varcode code) const
{
    int pos = find_item(code);
    if (pos == -1) return NULL;
    return m_vars[pos];
}

void Record::set_coords(const Coords& c)
{
    station.coords = c;
}

void Record::set_station(const Station& s)
{
    station.report = s.report;
    station.coords = s.coords;
    station.ident = s.ident;
    mobile = station.ident.is_missing() ? 0 : 1;
}

void Record::set_dbstation(const DBStation& s)
{
    station = s;
    mobile = station.ident.is_missing() ? 0 : 1;
}


void Record::set_from_string(const char* str)
{
    // Split the input as name=val
    const char* s = strchr(str, '=');

    if (!s) error_consistency::throwf("there should be an = between the name and the value in '%s'", str);

    string key(str, s - str);
    record_setf(*this, key.c_str(), s + 1);
}

void Record::set_from_test_string(const std::string& s)
{
    if (s.empty()) return;
    size_t cur = 0;
    while (true)
    {
        size_t next = s.find(", ", cur);
        if (next == string::npos)
        {
            set_from_string(s.substr(cur).c_str());
            break;
        } else {
            set_from_string(s.substr(cur, next - cur).c_str());
            cur = next + 2;
        }
    }
}
#endif

namespace {

#if 0
struct BufferPrinter
{
    std::stringstream s;
    bool first = true;

    template<typename KEY, typename VAL>
    void print(const KEY& key, const VAL& val)
    {
        if (first)
            first = false;
        else
            s << ",";
        s << key << "=" << val;
    }

    void print_varlist(const char* key, const std::set<wreport::Varcode>& varlist)
    {
        s << key << "=";
        bool first = true;
        for (const auto& code: varlist)
        {
            if (first)
                first = false;
            else
                s << ",";
            char buf[8];
            format_bcode(code, buf);
            s << buf;
        }
    }
};
#endif

struct FilePrinter
{
    FILE* out;

    template<typename VAL>
    void print(const char* key, const VAL& val)
    {
        fprintf(out, "%s=", key);
        val.print(out);
    }

    void print(const char* key, int val)
    {
        fprintf(out, "%s=%d\n", key, val);
    }

    void print(const char* key, const std::string& val)
    {
        fprintf(out, "%s=%s\n", key, val.c_str());
    }

    void print_varlist(const char* key, const std::set<wreport::Varcode>& varlist)
    {
        fprintf(out, "%s=", key);
        bool first = true;
        for (const auto& code: varlist)
        {
            if (first)
                first = false;
            else
                putc(',', out);
            char buf[8];
            format_bcode(code, buf);
            fputs(buf, out);
        }
    }
};

}

#if 0
std::string Record::to_string() const
{
    BufferPrinter printer;

    if (priomin != MISSING_INT) printer.print("priomin", priomin);
    if (priomax != MISSING_INT) printer.print("priomax", priomax);
    if (mobile != MISSING_INT) printer.print("mobile", mobile);
    if (!station.is_missing()) printer.print("station", station);
    if (!latrange.is_missing()) printer.print("latrange", latrange);
    if (!lonrange.is_missing()) printer.print("lonrange", lonrange);
    if (!datetime.is_missing()) printer.print("datetime", datetime);
    if (!level.is_missing()) printer.print("level", level);
    if (!trange.is_missing()) printer.print("trange", trange);
    if (var) printer.print("var", varcode_format(var));
    if (!varlist.empty()) printer.print_varlist("varlist", varlist);
    if (!query.empty()) printer.print("query", query);
    if (!ana_filter.empty()) printer.print("ana_filter", ana_filter);
    if (!data_filter.empty()) printer.print("data_filter", data_filter);
    if (!attr_filter.empty()) printer.print("attr_filter", attr_filter);
    if (count != MISSING_INT) printer.print("count", count);

    for (const auto& var: m_vars)
        printer.print(varcode_format(var->code()), var->format(""));

    return printer.s.str();
}
#endif

void Data::print(FILE* out) const
{
    FilePrinter printer;
    printer.out = out;

    if (!station.is_missing()) printer.print("station", station);
    if (!datetime.is_missing()) printer.print("datetime", datetime);
    if (!level.is_missing()) printer.print("level", level);
    if (!trange.is_missing()) printer.print("trange", trange);
    for (const auto& var: values)
        var.print(out);
}

#if 0
void Record::to_query(core::Query& q) const
{
    q.want_missing = 0;
    q.ana_id = station.id;
    q.prio_min = priomin;
    q.prio_max = priomax;
    q.rep_memo = station.report;
    q.mobile = mobile;
    q.ident = station.ident;
    q.latrange = latrange;
    q.lonrange.set(lonrange); // use set() so it checks for open ended ranges and x,x+360 ranges
    q.datetime = datetime;
    q.datetime.min.set_lower_bound();
    q.datetime.max.set_upper_bound();
    q.level = level;
    q.trange = trange;
    if (var)
    {
        q.varcodes.clear();
        q.varcodes.insert(var);
    }
    else
        q.varcodes = varlist;
    q.query = query;
    q.ana_filter = ana_filter;
    q.data_filter = data_filter;
    q.attr_filter = attr_filter;
    q.limit = count;
    // WMO block/station come from variables
    if (const Var* block = get_var(WR_VAR(0, 1, 1)))
        q.block = block->enq(MISSING_INT);
    else
        q.block = MISSING_INT;
    if (const Var* station = get_var(WR_VAR(0, 1, 2)))
        q.station = station->enq(MISSING_INT);
    else
        q.station = MISSING_INT;
}
#endif


#if 0
void dba_record_diff(dba_record rec1, dba_record rec2, int* diffs, FILE* out)
{
	int i;
	dba_record_cursor cur;

	/* First compare the keywords */
	for (i = 0; i < KEYWORD_TABLE_SIZE; i++)
	{
		if (rec1->keydata[i] == NULL && rec2->keydata[i] == NULL)
			continue;
		else
			dba_var_diff(rec1->keydata[i], rec2->keydata[i], diffs, out);
	}

	/* Then compare the hash tables */
	for (cur = dba_record_iterate_first(rec1); cur != NULL; cur = dba_record_iterate_next(rec1, cur))
	{
		dba_varcode code = dba_var_code(cur->var);
		if (!dba_record_has_item(rec2, code))
		{
			fprintf(out, "Variable %d%02d%03d only exists in first record\n",
					WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
			(*diffs)++;
		}
		{
			dba_item item2;
			for (item2 = rec2->vars; item2 != NULL; item2 = item2->next)
				if (dba_var_code(item2->var) == code)
				{
					dba_var_diff(cur->var, item2->var, diffs, out);
					break;
				}
		}
	}

	/* Check for the items in the second one not present in the first one */
	for (cur = dba_record_iterate_first(rec2); cur != NULL; cur = dba_record_iterate_next(rec2, cur))
	{
		dba_varcode code = dba_var_code(cur->var);
		if (!dba_record_has_item(rec1, code))
		{
			fprintf(out, "Variable %d%02d%03d only exists in second record\n",
					WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
			(*diffs)++;
		}
	}
}
#endif

#if 0
MatchedRecord::MatchedRecord(const Record& r)
    : r(r)
{
}

MatchedRecord::~MatchedRecord()
{
}

matcher::Result MatchedRecord::match_var_id(int val) const
{
    if (const wreport::Var* var = r.get_var(WR_VAR(0, 33, 195)))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_station_id(int val) const
{
    int ana_id = r.station.id;
    if (ana_id == MISSING_INT)
        return matcher::MATCH_NA;
    return ana_id == val ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedRecord::match_station_wmo(int block, int station) const
{
    if (const wreport::Var* var = r.get_var(WR_VAR(0, 1, 1)))
    {
        // Match block
        if (var->enqi() != block) return matcher::MATCH_NO;

        // If station was not requested, we are done
        if (station == -1) return matcher::MATCH_YES;

        // Match station
        if (const wreport::Var* var = r.get_var(WR_VAR(0, 1, 2)))
        {
            if (var->enqi() != station) return matcher::MATCH_NO;
            return matcher::MATCH_YES;
        }
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_datetime(const DatetimeRange& range) const
{
    Datetime dt = r.get_datetime();
    if (dt.is_missing()) return matcher::MATCH_NA;
    return range.contains(dt) ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedRecord::match_coords(const LatRange& latrange, const LonRange& lonrange) const
{
    int lat = r.station.coords.lat;
    matcher::Result r1 = matcher::MATCH_NA;
    if (lat != MISSING_INT)
        r1 = latrange.contains(lat) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (latrange.is_missing())
        r1 = matcher::MATCH_YES;

    int lon = r.station.coords.lon;
    matcher::Result r2 = matcher::MATCH_NA;
    if (lon != MISSING_INT)
        r2 = lonrange.contains(lon) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (lonrange.is_missing())
        r2 = matcher::MATCH_YES;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_rep_memo(const char* memo) const
{
    std::string report = r.station.report;
    if (report.empty())
        return matcher::MATCH_NA;
    return report == memo ? matcher::MATCH_YES : matcher::MATCH_NO;
}
#endif

}
}

