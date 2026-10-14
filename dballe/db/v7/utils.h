#ifndef DBALLE_DB_V7_UTILS_H
#define DBALLE_DB_V7_UTILS_H

#include <unordered_set>
#include <wreport/varinfo.h>

namespace dballe {
namespace db {
namespace v7 {

struct IdVarcode
{
    int id;
    wreport::Varcode varcode;

    IdVarcode(int id, wreport::Varcode varcode) : id(id), varcode(varcode) {}

    bool operator==(const IdVarcode& o) const
    {
        return std::tie(id, varcode) == std::tie(o.id, o.varcode);
    }
    bool operator!=(const IdVarcode& o) const
    {
        return std::tie(id, varcode) != std::tie(o.id, o.varcode);
    }
    bool operator<(const IdVarcode& o) const
    {
        return std::tie(id, varcode) < std::tie(o.id, o.varcode);
    }
    bool operator>(const IdVarcode& o) const
    {
        return std::tie(id, varcode) > std::tie(o.id, o.varcode);
    }
};

} // namespace v7
} // namespace db
} // namespace dballe

namespace std {
template <> struct hash<dballe::db::v7::IdVarcode>
{
    typedef dballe::db::v7::IdVarcode argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
        result_type const h1(std::hash<int>{}(s.id));
        result_type const h2(std::hash<wreport::Varcode>{}(s.varcode));
        return h1 ^ (h2 << 1);
    }
};
} // namespace std

#endif
