#include "value.h"
#include "station.h"
#include "levtr.h"
#include <iomanip>
#include <ostream>

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

const Value& Values::insert_or_replace(
        const Station& station, const LevTr& levtr,
        const Datetime& datetime, std::auto_ptr<Var> var)
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

const Value& Values::insert_or_replace(
        const Station& station, const LevTr& levtr,
        const Datetime& datetime, const Var& var)
{
    auto_ptr<Var> copy(new Var(var));
    return insert_or_replace(station, levtr, datetime, copy);
}

bool Values::remove(const Station& station, const LevTr& levtr, const Datetime& datetime, Varcode code)
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
