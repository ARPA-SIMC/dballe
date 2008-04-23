#include <dballe++/var.h>
#include <cstring>

using namespace std;

namespace dballe {

Varinfo::Varinfo(dba_varcode code)
{
    checked(dba_varinfo_query_local(code, &m_info));
}

Varinfo Varinfo::create(dba_varcode code)
{
    dba_varinfo info;
    checked(dba_varinfo_query_local(code, &info));
    return Varinfo(info);
}

bool Var::operator==(const char* val) const
{
	return strcmp(enqc(), val) == 0;
}

Var Var::clone(dba_var var)
{
    dba_var copy;
    checked(dba_var_copy(var, &copy));
    return Var(copy);
}

}

// vim:set ts=4 sw=4:
