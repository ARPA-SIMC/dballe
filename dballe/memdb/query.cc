#include "query.h"
#include <algorithm>
#ifdef TRACE_QUERY
#include <cstdio>
#include <cstdarg>
#endif

using namespace std;

namespace dballe {
namespace memdb {

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


