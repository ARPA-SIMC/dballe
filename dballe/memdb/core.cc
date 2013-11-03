#include "core.h"
#include "dballe/core/defs.h"
#include "dballe/core/stlutils.h"
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

template<typename T>
bool Index<T>::search(const T& el, stl::SetIntersection<size_t>& out) const
{
    typename std::map<T, Positions>::const_iterator i = this->find(el);
    if (i == this->end())
        return false;

    out.add(i->second);
    return true;
}

void Positions::dump(FILE* out) const
{
    for (const_iterator i = begin(); i != end(); ++i)
    {
        if (i != begin()) putc(',', out);
        fprintf(out, "%zu", *i);
    }
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
