#include "value.h"
#include "station.h"
#include "levtr.h"
#include <iomanip>
#include <ostream>

using namespace std;

namespace dballe {
namespace memdb {

std::ostream& operator<<(std::ostream& out, const Date& dt)
{
    out <<        setw(4) << setfill('0') << dt.year
        << '-' << setw(2) << setfill('0') << (unsigned)dt.month
        << '-' << setw(2) << setfill('0') << (unsigned)dt.day;
    return out;
}

std::ostream& operator<<(std::ostream& out, const Datetime& dt)
{
    out <<        setw(4) << setfill('0') << dt.year
        << '-' << setw(2) << setfill('0') << (unsigned)dt.month
        << '-' << setw(2) << setfill('0') << (unsigned)dt.day
        << 'T' << setw(2) << setfill('0') << (unsigned)dt.hour
        << ':' << setw(2) << setfill('0') << (unsigned)dt.minute
        << ':' << setw(2) << setfill('0') << (unsigned)dt.second;
    return out;
}

Value::~Value()
{
    delete var;
}

void Value::replace(std::auto_ptr<wreport::Var> var)
{
    delete this->var;
    this->var = var.release();
}

const Value& Values::insert_or_replace(
        const Station& station, const LevTr& levtr,
        const Datetime& datetime, std::auto_ptr<wreport::Var> var)
{
    Positions res = by_station.search(&station);
    by_levtr.refine(&levtr, res);
    by_date.refine(datetime, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i) && get(*i)->datetime == datetime && get(*i)->var->code() == var->code())
        {
            get(*i)->replace(var);
            return *get(*i);
        }

    // Station not found, create it
    size_t pos = value_add(new Value(station, levtr, datetime, var));
    // Index it
    by_station[&station].insert(pos);
    by_levtr[&levtr].insert(pos);
    by_date[datetime].insert(pos);
    // And return it
    return *get(pos);

}

bool Values::remove(const Station& station, const LevTr& levtr, const Datetime& datetime, wreport::Varcode code)
{
    Positions res = by_station.search(&station);
    by_levtr.refine(&levtr, res);
    by_date.refine(datetime, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i) && get(*i)->datetime == datetime && get(*i)->var->code() == code)
        {
            by_station[&station].erase(*i);
            by_levtr[&levtr].erase(*i);
            by_date[datetime].erase(*i);
            value_remove(*i);
            return true;
        }
    return false;
}

template class Index<const Station*>;

}
}
