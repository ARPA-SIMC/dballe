#include "core.h"
#include <dballe/core/defs.h>
#include <string>
#include <algorithm>

using namespace std;

namespace dballe {
namespace memdb {

struct Station;
struct LevTr;

template<typename T>
Positions Index<T>::search(const T& el) const
{
    typename std::map<T, Positions>::const_iterator i = this->find(el);
    if (i == this->end())
        return Positions();
    else
        return i->second;
}

Results::Results()
    : select_all(true)
{
}

void Results::intersect(size_t pos)
{
    if (select_all)
    {
        values.push_back(pos);
        select_all = false;
        return;
    }

    vector<size_t>::const_iterator i = lower_bound(values.begin(), values.end(), pos);
    bool found = i != values.end();
    values.clear();
    if (found)
        values.push_back(pos);
}

template<typename T>
void Index<T>::refine(const T& el, Positions& res)
{
    if (res.empty()) return;

    typename std::map<T, Positions>::const_iterator i = this->find(el);
    // If we have no results, the result set becomes empty
    if (i == this->end())
    {
        res.clear();
        return;
    }

    res.inplace_intersect(i->second);
}

template class Index<std::string>;
template class Index<Coord>;
template class Index<Level>;
template class Index<Trange>;
template class Index<const Station*>;
template class Index<const LevTr*>;
template class Index<Date>;

}
}


