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
    mutable std::vector<Item> items;
    mutable size_t dirty = 0;

    typedef typename std::vector<Item>::const_iterator const_iterator;
    typedef typename std::vector<Item>::iterator iterator;
    typedef typename std::vector<Item>::const_reverse_iterator const_reverse_iterator;
    typedef typename std::vector<Item>::reverse_iterator reverse_iterator;

    iterator begin() { return items.begin(); }
    iterator end() { return items.end(); }
    const_iterator begin() const { return items.begin(); }
    const_iterator end() const { return items.end(); }
    reverse_iterator rbegin() { return items.rbegin(); }
    reverse_iterator rend() { return items.rend(); }
    const_reverse_iterator rbegin() const { return items.rbegin(); }
    const_reverse_iterator rend() const { return items.rend(); }
    size_t size() const { return items.size(); }
    bool empty() const { return items.empty(); }

    bool operator==(const SmallSet<Parent, Item, Value>& o) const
    {
        if (dirty) rearrange_dirty();
        if (o.dirty) o.rearrange_dirty();
        return items == o.items;
    }

    bool operator!=(const SmallSet<Parent, Item, Value>& o) const
    {
        if (dirty) rearrange_dirty();
        if (o.dirty) o.rearrange_dirty();
        return items == o.items;
    }

    void clear()
    {
        items.clear();
        dirty = 0;
    }

    int binary_search(const Value& value) const
    {
        int begin, end;
        begin = -1, end = items.size();
        while (end - begin > 1)
        {
            int cur = (end + begin) / 2;
            if (value < Parent::_smallset_get_value(items[cur]))
                end = cur;
            else
                begin = cur;
        }
        if (begin == -1 || Parent::_smallset_get_value(items[begin]) != value)
            return -1;
        else
            return begin;
    }

    const_iterator find(const Value& value) const
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
            rearrange_dirty();
        }

        int pos = binary_search(value);
        if (pos == -1)
            return items.end();
        else
            return items.begin() + pos;
    }

    iterator find(const Value& value)
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
            rearrange_dirty();
        }

        int pos = binary_search(value);
        if (pos == -1)
            return items.end();
        else
            return items.begin() + pos;
    }

    Item& add(const Item& item)
    {
        ++dirty;
        items.emplace_back(item);
        return items.back();
    }

    // static const Value& _smallset_get_value(const Item&);

    void rearrange_dirty() const
    {
        // Rearrange newly inserted items by insertion sort
        for (size_t i = items.size() - dirty; i < items.size(); ++i)
            for (size_t j = i; j > 0 && Parent::_smallset_get_value(items[j]) < Parent::_smallset_get_value(items[j - 1]); --j)
                std::swap(items[j], items[j - 1]);
        dirty = 0;
    }
};


template<typename Value>
struct SmallUniqueValueSet : protected SmallSet<SmallUniqueValueSet<Value>, Value, Value>
{
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::iterator;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::const_iterator;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::reverse_iterator;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::const_reverse_iterator;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::begin;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::end;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::rbegin;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::rend;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::empty;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::size;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::clear;

    bool operator==(const SmallUniqueValueSet& o) const { return SmallSet<SmallUniqueValueSet<Value>, Value, Value>::operator==(o); }
    bool operator!=(const SmallUniqueValueSet& o) const { return SmallSet<SmallUniqueValueSet<Value>, Value, Value>::operator!=(o); }

    void add(const Value& val)
    {
        auto i = this->find(val);
        if (i != this->end()) return;
        SmallSet<SmallUniqueValueSet<Value>, Value, Value>::add(val);
    }

    bool has(const Value& val) const
    {
        return this->find(val) != this->end();
    }

    static const Value& _smallset_get_value(const Value& value) { return value; }
};


template<typename Value>
struct SortedSmallUniqueValueSet : public SmallUniqueValueSet<Value>
{
    typedef typename SmallSet<SmallUniqueValueSet<Value>, Value, Value>::iterator iterator;
    typedef typename SmallSet<SmallUniqueValueSet<Value>, Value, Value>::const_iterator const_iterator;
    typedef typename SmallSet<SmallUniqueValueSet<Value>, Value, Value>::reverse_iterator reverse_iterator;
    typedef typename SmallSet<SmallUniqueValueSet<Value>, Value, Value>::const_reverse_iterator const_reverse_iterator;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::end;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::rend;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::empty;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::size;
    using SmallSet<SmallUniqueValueSet<Value>, Value, Value>::clear;

    iterator begin()
    {
        if (this->dirty) this->rearrange_dirty();
        return SmallUniqueValueSet<Value>::begin();
    }
    const_iterator begin() const
    {
        if (this->dirty) this->rearrange_dirty();
        return SmallUniqueValueSet<Value>::begin();
    }
    reverse_iterator rbegin()
    {
        if (this->dirty) this->rearrange_dirty();
        return SmallUniqueValueSet<Value>::rbegin();
    }
    const_reverse_iterator rbegin() const
    {
        if (this->dirty) this->rearrange_dirty();
        return SmallUniqueValueSet<Value>::rbegin();
    }

    bool operator==(const SortedSmallUniqueValueSet& o) const { return SmallUniqueValueSet<Value>::operator==(o); }
    bool operator!=(const SortedSmallUniqueValueSet& o) const { return SmallUniqueValueSet<Value>::operator!=(o); }

    void add(const Value& val)
    {
        auto i = this->find(val);
        if (i != this->end()) return;
        SmallSet<SmallUniqueValueSet<Value>, Value, Value>::add(val);
    }

    bool has(const Value& val) const
    {
        return this->find(val) != this->end();
    }

    static const Value& _smallset_get_value(const Value& value) { return value; }
};

}
}

#endif
