#include "var.h"
#include "core/aliases.h"
#include <wreport/vartable.h>

using namespace std;
using namespace wreport;

namespace {
const Vartable* local = nullptr;
}

namespace dballe {

wreport::Varinfo varinfo(wreport::Varcode code)
{
    if (!local) local = Vartable::get("dballe");
    return local->query(code);
}

wreport::Varinfo varinfo(const char* code)
{
    if (!local) local = Vartable::get("dballe");
    return local->query(resolve_varcode(code));
}

wreport::Varinfo varinfo(const std::string& code)
{
    if (!local) local = Vartable::get("dballe");
    return local->query(resolve_varcode(code));
}

wreport::Varcode resolve_varcode(const char* name)
{
    if (!name)
        throw error_consistency("cannot parse a Varcode out of a NULL");
    if (!name[0])
        throw error_consistency("cannot parse a Varcode out of an empty string");

    // Try looking up among aliases
    Varcode res = varcode_alias_resolve(name);
    if (res) return res;

    if (name[0] != 'B')
        error_consistency::throwf("cannot parse a Varcode out of '%s'", name);

    // Ensure that B is followed by 5 integers
    for (unsigned i = 1; i < 6; ++i)
        if (name[i] and !isdigit(name[i]))
            error_consistency::throwf("cannot parse a Varcode out of '%s'", name);

    return WR_STRING_TO_VAR(name + 1);
}

wreport::Varcode resolve_varcode(const std::string& name)
{
    if (name.empty())
        throw error_consistency("cannot parse a Varcode out of an empty string");

    // Try looking up among aliases
    Varcode res = varcode_alias_resolve(name);
    if (res) return res;

    if (name[0] != 'B')
        error_consistency::throwf("cannot parse a Varcode out of '%s'", name.c_str());

    // Ensure that B is followed by 5 integers
    for (unsigned i = 1; i < 6; ++i)
        if (name[i] and !isdigit(name[i]))
            error_consistency::throwf("cannot parse a Varcode out of '%s'", name.c_str());

    return WR_STRING_TO_VAR(name.data() + 1);
}


}
