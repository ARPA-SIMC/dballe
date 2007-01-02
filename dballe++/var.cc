#include <dballe++/var.h>

using namespace std;

namespace dballe {

Varinfo Varinfo::create(dba_varcode code)
{
	dba_varinfo info;
	checked(dba_varinfo_query_local(code, &info));
	return Varinfo(info);
}

}

// vim:set ts=4 sw=4:
