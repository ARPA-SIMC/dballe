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

size_t Values::insert(
        const Station& station, const LevTr& levtr,
        const Datetime& datetime, std::auto_ptr<Var> var, bool replace)
{
    Positions res = by_station.search(&station);
    by_levtr.refine(&levtr, res);
    by_date.refine(datetime, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
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
    Positions res = by_station.search(&station);
    by_levtr.refine(&levtr, res);
    by_date.refine(datetime, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
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

template class Index<const Station*>;

}
}
