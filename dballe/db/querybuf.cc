/*
 * db/querybuf - Buffer used to build SQL queries
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "querybuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "dballe/core/vasprintf.h"

using namespace std;

namespace dballe {

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
	if (list_first)
		list_first = false;
	else
		append(list_sep);
	append(str);
}

void Querybuf::append_listf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (list_first)
		list_first = false;
	else
		append(list_sep);

	char* buf;
	int size = vasprintf(&buf, fmt, ap);
	append(buf, size);
	free(buf);

	va_end(ap);
}

}

/* vim:set ts=4 sw=4: */
