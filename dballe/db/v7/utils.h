#ifndef DBALLE_DB_V7_UTILS_H
#define DBALLE_DB_V7_UTILS_H

#include <wreport/varinfo.h>
#include <unordered_set>

namespace dballe {
namespace db {
namespace v7 {

struct IdVarcode
{
    int id;
    wreport::Varcode varcode;

    IdVarcode(int id, wreport::Varcode varcode)
        : id(id), varcode(varcode)
    {
    }

    bool operator==(const IdVarcode& o) const { return o.id == id && o.varcode == varcode; }
};

}
}
}

namespace std
{
    template<> struct hash<dballe::db::v7::IdVarcode>
    {
        typedef dballe::db::v7::IdVarcode argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const noexcept
        {
            result_type const h1 ( std::hash<int>{}(s.id) );
            result_type const h2 ( std::hash<wreport::Varcode>{}(s.varcode) );
            return h1 ^ (h2 << 1);
        }
    };
}


#endif
