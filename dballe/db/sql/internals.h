/*
 * db/sql/internals - Support structures not part of any public API
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include <wreport/var.h>
#include <vector>

namespace dballe {
namespace db {
namespace sql {

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
