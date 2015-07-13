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
const std::set<size_t>* Index<T>::search(const T& el) const
{
    const_iterator i = this->find(el);
    if (i == this->end())
        return 0;
    else
        return &(i->second);
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
std::unique_ptr< stl::Sequences<size_t> > Index<T>::search_from(const T& first, bool& found) const
{
    typedef std::unique_ptr< stl::Sequences<size_t> > RES;

    const_iterator i = this->lower_bound(first);
    if (i == this->begin())
    {
        // Whole index match
        found = true;
        return RES();
    }

    if (i == this->end())
    {
        // No match
        found = false;
        return RES();
    }

    found = true;
    RES res(new stl::Sequences<size_t>);
    for ( ; i != this->end(); ++i)
        res->add(i->second);
    return res;
}

template<typename T>
std::unique_ptr< stl::Sequences<size_t> > Index<T>::search_to(const T& end, bool& found) const
{
    typedef std::unique_ptr< stl::Sequences<size_t> > RES;

    const_iterator i_end = this->upper_bound(end);
    if (i_end == this->end())
    {
        // Whole index match
        found = true;
        return RES();
    }

    if (i_end == this->begin())
    {
        // No match
        found = false;
        return RES();
    }

    found = true;
    RES res(new stl::Sequences<size_t>);
    for (const_iterator i = this->begin(); i != i_end; ++i)
        res->add(i->second);
    return res;
}

template<typename T>
std::unique_ptr< stl::Sequences<size_t> > Index<T>::search_between(const T& first, const T& end, bool& found) const
{
    typedef std::unique_ptr< stl::Sequences<size_t> > RES;

    if (end < first)
    {
        // Impossible match
        found = false;
        return RES();
    }

    const_iterator i = this->lower_bound(first);
    const_iterator i_end = this->upper_bound(end);

    if (i == this->begin() && i_end == this->end())
    {
        // Whole index match
        found = true;
        return RES();
    }

    if (i == this->end() && i_end == this->begin())
    {
        // No match
        found = false;
        return RES();
    }

    found = true;
    RES res(new stl::Sequences<size_t>);
    for ( ; i != i_end; ++i)
        res->add(i->second);
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
template class Index<int>;
template class Index<Level>;
template class Index<Trange>;
template class Index<const Station*>;
template class Index<const LevTr*>;
template class Index<Date>;

}
}

#include "dballe/core/stlutils.tcc"

