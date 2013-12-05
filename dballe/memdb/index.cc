#include "index.h"
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
std::set<size_t> Index<T>::search(const T& el) const
{
    const_iterator i = this->find(el);
    if (i == this->end())
        return set<size_t>();
    else
        return i->second;
}

template<typename T>
bool Index<T>::search(const T& el, stl::SetIntersection<size_t>& out) const
{
    const_iterator i = this->find(el);
    if (i == this->end())
        return false;

    out.add(i->second);
    return true;
}

template<typename T>
bool Index<T>::search(const T& el, stl::Sequences<size_t>& out) const
{
    const_iterator i = this->find(el);
    if (i == this->end())
        return false;

    out.add(i->second);
    return true;
}

template<typename T>
bool Index<T>::search_from(const T& first, stl::Sequences<size_t>& out) const
{
    bool res = false;
    for (const_iterator i = this->lower_bound(first); i != this->end(); ++i)
    {
        out.add(i->second);
        res = true;
    }
    return res;
}

template<typename T>
bool Index<T>::search_to(const T& end, stl::Sequences<size_t>& out) const
{
    bool res = false;
    const_iterator i_end = this->upper_bound(end);
    for (const_iterator i = this->begin(); i != i_end; ++i)
    {
        out.add(i->second);
        res = true;
    }
    return res;
}

template<typename T>
bool Index<T>::search_between(const T& first, const T& end, stl::Sequences<size_t>& out) const
{
    bool res = false;
    const_iterator i_end = this->upper_bound(end);
    for (const_iterator i = this->lower_bound(first); i != i_end; ++i)
    {
        out.add(i->second);
        res = true;
    }
    return res;
}

#if 0
void Positions::dump(FILE* out) const
{
    for (const_iterator i = begin(); i != end(); ++i)
    {
        if (i != begin()) putc(',', out);
        fprintf(out, "%zu", *i);
    }
}
#endif

template class Index<std::string>;
template class Index<Coord>;
template class Index<Level>;
template class Index<Trange>;
template class Index<const Station*>;
template class Index<const LevTr*>;
template class Index<Date>;

}
}

#include "dballe/core/stlutils.tcc"

