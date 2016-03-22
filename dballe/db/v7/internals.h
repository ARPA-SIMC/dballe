#ifndef DBALLE_DB_V7_INTERNALS_H
#define DBALLE_DB_V7_INTERNALS_H

#include <wreport/var.h>
#include <vector>

namespace dballe {
namespace db {
namespace v7 {

/// Store a list of attributes to be inserted/updated in the database
struct AttributeList : public std::vector<std::pair<wreport::Varcode, const char*>>
{
    void add(wreport::Varcode code, const char* value)
    {
        push_back(std::make_pair(code, value));
    }

    /// Get a value by code, returns nullptr if not found
    const char* get(wreport::Varcode code) const
    {
        for (const_iterator i = begin(); i != end(); ++i)
            if (i->first == code) return i->second;
        return nullptr;
    }

    /**
     * Get a value by code, returns nullptr if not found, removes it from the
     * AttributeList
     */
    const char* pop(wreport::Varcode code)
    {
        const char* res = nullptr;
        for (iterator i = begin(); i != end(); ++i)
        {
            if (i->first == code)
            {
                res = i->second;
                i->second = nullptr;
                break;
            }
        }
        while (!empty() && back().second == nullptr)
            pop_back();
        return res;
    }
};

}
}
}

#endif
