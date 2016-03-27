#include "querybuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "dballe/core/vasprintf.h"
#include "dballe/var.h"

using namespace std;
using namespace wreport;

namespace dballe {
namespace sql {

Querybuf::Querybuf(size_t reserve)
{
	clear();
	this->reserve(512);
}

Querybuf::~Querybuf() {}

void Querybuf::clear()
{
	string::clear();
	list_first = true;
	list_sep[0] = 0;
}

void Querybuf::start_list(const char* sep)
{
	list_first = true;
	strncpy(list_sep, sep, 10);
	list_sep[9] = 0;
}

void Querybuf::start_list_item()
{
    if (list_first)
        list_first = false;
    else
        append(list_sep);
}

void Querybuf::append_int(int val)
{
    char buf[16];
    snprintf(buf, 16, "%d", val);
    append(buf);
}

void Querybuf::appendf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char* buf;
	int size = vasprintf(&buf, fmt, ap);
	append(buf, size);
	free(buf);
	va_end(ap);
}

void Querybuf::append_list(const char* str)
{
    start_list_item();
    append(str);
}

void Querybuf::append_listf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

    start_list_item();

	char* buf;
	int size = vasprintf(&buf, fmt, ap);
	append(buf, size);
	free(buf);

	va_end(ap);
}

void Querybuf::append_varlist(const std::string& varlist)
{
    size_t pos = 0;
    while (true)
    {
        size_t end = varlist.find(',', pos);
        Varcode code;
        if (end == string::npos)
            code = resolve_varcode(varlist.substr(pos));
        else
            code = resolve_varcode(varlist.substr(pos, end - pos));
        if (pos == 0)
            appendf("%d", code);
        else
            appendf(",%d", code);
        if (end == string::npos)
            break;
        pos = end + 1;
    }
}

void Querybuf::append_varlist(const std::set<wreport::Varcode>& varlist)
{
    bool first = true;
    for (const auto& v : varlist)
    {
        if (first)
        {
            appendf("%d", (int)v);
            first = false;
        }
        else
            appendf(",%d", (int)v);
    }
}

}
}
