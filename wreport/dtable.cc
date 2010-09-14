/*
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

#include "vartable.h"
#include "error.h"

#include "config.h"

#include "dtable.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>		/* malloc, strtod, getenv */
#include <string.h>		/* strncmp */
#include <strings.h>	/* bzero */
#include <ctype.h>		/* isspace */
#include <assert.h>		/* assert */
#include <limits.h>		/* PATH_MAX */
#include <fcntl.h>		/* O_RDONLY */

#include <map>
#include <string>

using namespace std;

namespace wreport {

namespace {
struct fd_closer
{
	FILE* fd;
	fd_closer(FILE* fd) : fd(fd) {}
	~fd_closer() { fclose(fd); }
};
}

void DTable::load(const std::string& id)
{
	std::string file = Vartable::id_to_pathname(id.c_str());
	FILE* in = fopen(file.c_str(), "rt");
	char line[200];
	int line_no = 0;
	int nentries_check = 0;

	if (in == NULL)
		throw error_system("opening D table file " + file);
	fd_closer closer(in); // Close `in' on exit

	unsigned begin = 0; // Begin of the last code block
	Varcode dcode;
	while (fgets(line, 200, in) != NULL)
	{
		line_no++;

		if (strlen(line) < 18)
			throw error_parse(file.c_str(), line_no, "line too short");

		if (line[1] == 'D' || line[1] == '3')
		{
			int last_count = varcodes.size() - begin;
			if (last_count != nentries_check)
				error_parse::throwf(file.c_str(), line_no, "advertised number of expansion items (%d) does not match the number of items found (%d)", nentries_check, last_count);

			nentries_check = strtol(line + 8, 0, 10);
			if (nentries_check < 1)
				throw error_parse(file.c_str(), line_no, "less than one entry advertised in the expansion");

			if (!varcodes.empty())
				entries.push_back(dtable::Entry(dcode, begin, varcodes.size()));
			begin = varcodes.size();
			dcode = descriptor_code(line + 1);
			varcodes.push_back(descriptor_code(line + 11));

			/* fprintf(stderr, "Debug: D%05d %d entries\n", dcode, nentries); */
		}
		else if (strncmp(line, "           ", 11) == 0)
		{
			int last_count;
			/* Check that there has been at least one entry filed before */
			if (varcodes.empty())
				throw error_parse(file.c_str(), line_no, "expansion line found before the first entry");
			/* Check that we are not appending too many entries */
			last_count = varcodes.size() - begin;
			if (last_count == nentries_check)
				error_parse::throwf(file.c_str(), line_no, "too many entries found (expected %d)", nentries_check);

			/* Finally append the code */
			Varcode code = descriptor_code(line + 11);
			varcodes.push_back(code);
		}
		else
			error_parse::throwf(file.c_str(), line_no, "unrecognized line: \"%s\"", line);
	}

	/* Check that we actually read something */
	if (varcodes.empty())
		throw error_parse(file.c_str(), line_no, "no entries found in the file");
	else
		entries.push_back(dtable::Entry(dcode, begin, varcodes.size()));

	/* Check that the last entry is complete */
	int last_count = varcodes.size() - begin;
	if (last_count != nentries_check)
		error_parse::throwf(file.c_str(), line_no, "advertised number of expansion items (%d) does not match the number of items found (%d)", nentries_check, last_count);

	m_id = id;
}

static std::map<string, DTable> tables;

const DTable* DTable::get(const char* id)
{
	// Return it from cache if we have it
	std::map<string, DTable>::const_iterator i = tables.find(id);
	if (i != tables.end())
		return &(i->second);

	// Else, instantiate it
	DTable* res = &tables[id];
	res->load(id);

	return res;
}


DTable::DTable() {}
DTable::~DTable() {}

Opcodes DTable::query(Varcode var) const
{
	int begin, end;

	/* Then, binary search the varinfo value */
	begin = -1, end = entries.size();
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (entries[cur].code > var)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || entries[begin].code != var)
		error_notfound::throwf(
				"looking up D table expansion for variable %d%02d%03d in table %s",
				WR_VAR_F(var), WR_VAR_X(var), WR_VAR_Y(var), m_id.c_str());
	else
		return Opcodes(varcodes, entries[begin].begin, entries[begin].end);
}

}

/* vim:set ts=4 sw=4: */
