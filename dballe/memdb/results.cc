#include "results.h"
#include <algorithm>
#ifdef TRACE_QUERY
#include <cstdio>
#include <cstdarg>
#endif

using namespace std;

namespace dballe {
namespace memdb {

namespace results {

Base::Base() : others_to_intersect(0), indices(0), all(true), empty(false) {}
Base::~Base()
{
    if (indices) delete indices;
    if (others_to_intersect) delete others_to_intersect;
    for (std::vector<std::set<size_t>*>::iterator i = transient_sets.begin();
            i != transient_sets.end(); ++i)
        delete *i;
}

void Base::add_union(std::unique_ptr< stl::Sequences<size_t> > seq)
{
    all = false;
    if (!others_to_intersect)
        others_to_intersect = new stl::Sequences<size_t>;
    others_to_intersect->add_union(seq);
}

void Base::add_singleton(size_t singleton)
{
    all = false;
    if (!others_to_intersect)
        others_to_intersect = new stl::Sequences<size_t>;
    others_to_intersect->add_singleton(singleton);
}

void Base::add_set(const std::set<size_t>& p)
{
    all = false;
    if (!indices)
        indices = new stl::SetIntersection<size_t>;
    indices->add(p);
}

void Base::add_set(std::unique_ptr< std::set<size_t> > p)
{
    transient_sets.push_back(p.release());
    add_set(*transient_sets.back());
}

}

#ifdef TRACE_QUERY
void trace_query(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
#endif

}
}

#include "dballe/core/stlutils.tcc"
