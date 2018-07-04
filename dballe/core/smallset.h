#ifndef DBALLE_CORE_SMALLSET_H
#define DBALLE_CORE_SMALLSET_H

#include <vector>
#include <algorithm>

namespace dballe {
namespace core {

/**
 * Set structure optimized for a small number of items
 */
template<typename Parent, typename Item, typename Value=Item>
struct SmallSet
{
    std::vector<Item> items;
    size_t dirty = 0;

    typedef typename std::vector<Item>::const_iterator const_iterator;

    const_iterator begin() const { return items.begin(); }
    const_iterator end() const { return items.end(); }

    const_iterator find(const Value& value)
    {
        if (items.empty()) return end();

        // Stick to linear search if the vector size is small
        if (items.size() < 6)
        {
            for (auto it = std::begin(items); it != std::end(items); ++it)
                if (Parent::_smallset_get_value(*it) == value)
                    return it;
            return end();
        }

        // Use binary search for larger vectors

        if (dirty > 16)
        {
            std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
                return Parent::_smallset_get_value(a) < Parent::_smallset_get_value(b);
            });
            dirty = 0;
        } else if (dirty) {
            // Use insertion sort, if less than 16 new elements appeared since the
            // last sort
            insertion_sort();
        }

        // Binary search
        int begin, end;
        begin = -1, end = items.size();
        while (end - begin > 1)
        {
            int cur = (end + begin) / 2;
            if (Parent::_smallset_get_value(items[cur]) > value)
                end = cur;
            else
                begin = cur;
        }
        if (begin == -1 || Parent::_smallset_get_value(items[begin]) != value)
            return items.end();
        else
            return items.begin() + begin;
    }

    Item& add(Item item)
    {
        ++dirty;
        items.emplace_back(item);
        return items.back();
    }

    // static const Value& _smallset_get_value(const Item&);

    void insertion_sort()
    {
        for (size_t i = items.size() - dirty; i < items.size(); ++i)
            for (size_t j = i; j > 0 && Parent::_smallset_get_value(items[j - 1]) > Parent::_smallset_get_value(items[j]); --j)
                std::swap(items[j], items[j - 1]);
        dirty = 0;
    }
};


}
}

#endif
