/*
 * wreport/vartable - Load variable information from on-disk tables
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>		/* malloc, strtod, getenv */
#include <string.h>		/* strncmp */
#include <unistd.h>		/* access */
#include <ctype.h>		/* isspace */
#include <math.h>		/* rint */
#include <assert.h>		/* assert */
#include <limits.h>		/* PATH_MAX, INT_MIN, INT_MAX */

#include "vartable.h"
#include "aliases.h"
#include "error.h"

#include <map>

using namespace std;

namespace wreport {

static std::map<string, Vartable> tables;

#if 0
/* Table to use to resolve DB-All.e WMO parameter names */
static const Vartable* local_vars = NULL;
#endif

Vartable::Vartable() {}
Vartable::~Vartable() {}

Varinfo Vartable::query(Varcode var) const
{
	int begin, end;

	// Binary search
	begin = -1, end = size();
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if ((*this)[cur].var > var)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || (*this)[begin].var != var)
		error_notfound::throwf(
				"looking up variable informations for variable %d%02d%03d in table %s",
				WR_VAR_F(var), WR_VAR_X(var), WR_VAR_Y(var), m_id.c_str());
	else
		return Varinfo(&(*this)[begin]);
}

Varinfo Vartable::query_altered(Varcode var, Alteration change) const
{
	if (change == 0 || change == DBA_ALT(0, 0))
		return query(var);

	/* Get the normal variable */
	Varinfo start = query(var);

	/* Look for an existing alteration */
	const _Varinfo* i = NULL;
	for (i = start.impl(); i->alteration != change && i->alterations != NULL ; i = i->alterations)
		;

	if (i->alteration != change)
	{
		/* Not found: we need to create it */
		int alt;

		/* Duplicate the original varinfo */
		_Varinfo* newvi = new _Varinfo(*start);
		i->alterations = newvi;
		i = i->alterations;

		newvi->_ref = 1;
		newvi->alteration = change;
		newvi->alterations = NULL;

#if 0
		fprintf(stderr, "Before alteration(w:%d,s:%d): bl %d len %d scale %d\n",
				DBA_ALT_WIDTH(change), DBA_ALT_SCALE(change),
				i->bit_len, i->len, i->scale);
#endif

		/* Apply the alterations */
		if ((alt = DBA_ALT_WIDTH(change)) != 0)
		{
			newvi->bit_len += alt;
			newvi->len = (int)ceil(log10(1 << i->bit_len));
		}
		if ((alt = DBA_ALT_SCALE(change)) != 0)
		{
			newvi->scale += alt;
			newvi->bufr_scale += alt;
		}

#if 0
		fprintf(stderr, "After alteration(w:%d,s:%d): bl %d len %d scale %d\n",
				DBA_ALT_WIDTH(change), DBA_ALT_SCALE(change),
				i->bit_len, i->len, i->scale);
#endif

		/* Postprocess the data, filling in minval and maxval */
		newvi->compute_range();
	}

	return Varinfo(i);
}


const Vartable* Vartable::get(const char* id)
{
	// Return it from cache if we have it
	std::map<string, Vartable>::const_iterator i = tables.find(id);
	if (i != tables.end())
		return &(i->second);

	// Else, instantiate it
	Vartable* res = &tables[id];
	res->load(id);

#if 0
	if (!local_vars && strcmp(id, "dballe") == 0)
		local_vars = res;
#endif

	return res;
}

#if 0
Varinfo Vartable::query_local(Varcode code)
{
	/* Load wreport WMO parameter resolution table */
	if (local_vars == NULL)
		Vartable::get("dballe");
	return local_vars->query(code);
}
#endif


namespace {
struct fd_closer
{
	FILE* fd;
	fd_closer(FILE* fd) : fd(fd) {}
	~fd_closer() { fclose(fd); }
};

static long getnumber(char* str)
{
	while (*str && isspace(*str))
		++str;
	if (!*str) return 0;
	if (*str == '-')
	{
		++str;
		// Eat spaces after the - (oh my this makes me sad)
		while (*str && isspace(*str))
			++str;
		return -strtol(str, 0, 10);
	} else
		return strtol(str, 0, 10);
}
}

void Vartable::load(const char* id)
{
	string file = id_to_pathname(id);
	FILE* in = fopen(file.c_str(), "rt");
	char line[200];
	int line_no = 0;
	Varcode last_code = 0;
	bool is_bufr = strlen(id) != 7;

	if (in == NULL)
		throw error_system("opening BUFR/CREX table file " + file);

	fd_closer closer(in); // Close `in' on exit

	while (fgets(line, 200, in) != NULL)
	{
		/* FMT='(1x,A,1x,A64,47x,A24,I3,8x,I3)' */
		int i;

		// Append a new entry;
		resize(size()+1);
		_Varinfo* entry = &back();

		entry->flags = 0;

		/*fprintf(stderr, "Line: %s\n", line);*/

		line_no++;

		if (strlen(line) < 119)
			throw error_parse(file.c_str(), line_no, "line too short");

		/* Read starting B code */
		/*fprintf(stderr, "Entry: B%05d\n", bcode);*/
		entry->var = DBA_STRING_TO_VAR(line + 2);

		if (entry->var < last_code)
			throw error_parse(file.c_str(), line_no, "input file is not sorted");
		last_code = entry->var;

		/* Read the description */
		memcpy(entry->desc, line+8, 64);
		/* Zero-terminate the description */
		for (i = 63; i >= 0 && isspace(entry->desc[i]); i--)
			;
		entry->desc[i+1] = 0;
		
		/* Read the BUFR type */
		memcpy(entry->unit, line+73, 24);
		memcpy(entry->bufr_unit, line+73, 24);
		/* Zero-terminate the type */
		for (i = 23; i >= 0 && isspace(entry->unit[i]); i--)
			;
		entry->unit[i+1] = 0;
		entry->bufr_unit[i+1] = 0;

		if (
				strcmp(entry->unit, "CCITTIA5") == 0 /*||
				strncmp(entry->unit, "CODE TABLE", 10) == 08*/
		) entry->flags |= VARINFO_FLAG_STRING;

		entry->scale = getnumber(line+98);
		entry->bufr_scale = getnumber(line+98);
		entry->bit_ref = getnumber(line+102);
		entry->bit_len = getnumber(line+115);

		if (strlen(line) < 157 || is_bufr)
		{
			entry->ref = 0;
			if (entry->is_string())
			{
				entry->len = entry->bit_len / 8;
			} else {
				int len = 1 << entry->bit_len;
				for (entry->len = 0; len != 0; entry->len++)
					len /= 10;
			}
		} else {
			/* Read the CREX type */
			memcpy(entry->unit, line+119, 24);
			/* Zero-terminate the type */
			for (i = 23; i >= 0 && isspace(entry->unit[i]); i--)
				;
			entry->unit[i+1] = 0;

			entry->scale = getnumber(line+138);
			entry->ref = 0;
			entry->len = getnumber(line+149);

			bool crex_is_string = (
					strcmp(entry->unit, "CHARACTER") == 0 /* ||
					strncmp(entry->unit, "CODE TABLE", 10) == 0 */
			);

			if (entry->is_string() != crex_is_string)
				error_parse::throwf(file.c_str(), line_no,
						"CREX is_string (%d) is different than BUFR is_string (%s)",
						(int)crex_is_string, (int)entry->is_string());
		}

		/* Postprocess the data, filling in minval and maxval */
		entry->compute_range();

		entry->alteration = 0;
		entry->alterations = NULL;

		/*
		fprintf(stderr, "Debug: B%05d len %d scale %d type %s desc %s\n",
				bcode, entry->len, entry->scale, entry->type, entry->desc);
		*/
		entry->do_ref();
	}

	m_id = id;
}

#if 0

/**
 * Insert a new vartable index in 'tables', keeping it sorted
 *
 * @returns The position at which the new table index can now be found
 */
static dba_err vartable_insert(dba_vartable v, int* index)
{
	int pos;

	if (tables == NULL)
	{
		/* First initialization special case */
		if ((tables = (dba_vartable*)malloc(sizeof(dba_vartable) * 10)) == NULL)
			return dba_error_alloc("allocating space for a new vartable index");
		tables_alloc_size = 10;
		pos = 0;
	} else {
		/* Look for insertion point */
		for (pos = 0; pos < tables_size && strcmp(v->id, tables[pos]->id) > 0; pos++)
			;

		/* Avoid double inserts */
		if (pos < tables_size && strcmp(v->id, tables[pos]->id) == 0)
			return dba_error_consistency("table %s already present", v->id);

		/* Enlarge the buffer if needed */
		if (tables_size + 1 > tables_alloc_size)
		{
			dba_vartable* old_tables = tables;

			if ((tables = (dba_vartable*)realloc(tables, sizeof(dba_vartable) * (tables_alloc_size + 10))) == NULL)
			{
				tables = old_tables;
				return dba_error_alloc("allocating space for 10 more vartable indexes");
			}
			tables_alloc_size += 10;
		}

		/* Shift the next items forward */
		memmove(tables + pos + 1, tables + pos, (tables_size - pos) * sizeof(dba_vartable));
	}

	tables[pos] = v;
	++tables_size;
	*index = pos;

	return dba_error_ok();
}


dba_err dba_varinfo_query_local_altered(dba_varcode code, dba_alteration change, const dba_varinfo* info)
{
	/* Load dballe WMO parameter resolution table */
	if (local_vars == NULL)
		DBA_RUN_OR_RETURN(dba_vartable_create("dballe", &local_vars));
	return dba_vartable_query_altered(local_vars, code, change, info);
}

dba_err dba_varinfo_get_local_table(dba_vartable* table)
{
	/* Load dballe WMO parameter resolution table */
	if (local_vars == NULL)
		DBA_RUN_OR_RETURN(dba_vartable_create("dballe", &local_vars));
	*table = local_vars;
	return dba_error_ok();
}

const char* dba_vartable_id(dba_vartable table)
{
	return table->id;
}

dba_err dba_vartable_iterate(dba_vartable table, dba_vartable_iterator func, void* data)
{
	int i;
	for (i = 0; i < table->size; i++)
		func(table->items + i, data);
	return dba_error_ok();
}


#endif

std::string Vartable::id_to_pathname(const char* id)
{
	// First check the DBA_TABLES env var; if that doesn't exist, then use
	// the TABLE_DIR constant from autoconf
	char* env = getenv("WREPORT_TABLES");
	string res = env ? env : TABLE_DIR;
	res += '/';
	res += id;
	res += ".txt";
	return res;
}

bool Vartable::exists(const char* id)
{
	return access(id_to_pathname(id).c_str(), F_OK) == 0;
}


}

/* vim:set ts=4 sw=4: */
