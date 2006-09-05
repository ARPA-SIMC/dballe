/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#define _GNU_SOURCE
/* _GNU_SOURCE is defined to have asprintf */
#include "config.h"

#include <dballe/formatter.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>     /* PATH_MAX */
#include <stdlib.h>     /* strtoul, getenv */

#define levels_count 258
#define tranges_count 256
static const char* def_desc = "Reserved";
static const char* ldescs[levels_count];
static const char* tdescs[tranges_count];
static int initialized = 0;

// FIXME: this is copied from dba_vartable.
static const char* id_to_pathname(const char* id)
{
	static char buf[PATH_MAX];
	char* env = getenv("DBA_TABLES");
	int i, j = strlen(id);

	/* First check the DBA_TABLES env var */
	/* If that doesn't exist, then use the TABLE_DIR constant from autoconf */
	if (env != NULL && (i = strlen(env)) < PATH_MAX - j - 6)
		strncpy(buf, env, i);
	else if ((i = strlen(TABLE_DIR)) >= PATH_MAX - j - 6)
		return NULL;
	else
		strncpy(buf, TABLE_DIR, i);

	buf[i++] = '/';
	strncpy(buf+i, id, j);
	strcpy(buf+i+j, ".txt");

	return buf;
}

static dba_err read_strings(const char* tag, const char *descs[], int size)
{
	dba_err err = DBA_OK;
	char line[1000];
	FILE* in = NULL;
	int line_no = 1;
	const char* fname = id_to_pathname(tag);

	if (fname == NULL)
	{
		err = dba_error_consistency("Cannot find a suitable location to load %", tag);
		goto cleanup;
	}
	
	if ((in = fopen(fname, "rt")) == NULL)
	{
		err = dba_error_system("Opening description file %s", fname);
		goto cleanup;
	}

	for ( ; fgets(line, 1000, in) != NULL; ++line_no)
	{
		char* s = line;
		int index;
		// Skip leading spaces
		while (*s && isspace(*s)) ++s;
		// Skip empty lines
		if (!*s) continue;
		// Skip comment lines
		if (*s == '#') continue;
		// Lines must start with numbers
		if (!isdigit(*s))
		{
			err = dba_error_consistency("%s:%d: the line should start with a number",
					fname, line_no);
			goto cleanup;
		}
		index = strtoul(s, &s, 10);
		if (index >= size)
		{
			err = dba_error_consistency("%s:%d: the code should be less than %d",
					fname, line_no, size);
			goto cleanup;
		}
		// Look for the description after the number
		while (*s && isspace(*s)) ++s;
		if (!*s)
		{
			err = dba_error_consistency("%s:%d: there should be a description after the number",
					fname, line_no);
			goto cleanup;
		}

		// Trim trailing spaces and newlines
		for (s = line + strlen(line) - 1; s > line; --s)
			if (isspace(*s))
				*s = 0;
			else
				break;

		descs[index] = strdup(line);

	}

	fclose(in);
	in = NULL;

cleanup:
	if (in != NULL)
		fclose(in);
	return err != DBA_OK ? err : dba_error_ok();
}

static dba_err want_levels()
{
	int i;
	if (initialized)
		return dba_error_ok();

	for (i = 0; i < levels_count; ++i)
		ldescs[i] = def_desc;

	DBA_RUN_OR_RETURN(read_strings("levels", ldescs, levels_count));

	initialized = 1;
	return dba_error_ok();
}

static dba_err want_tranges()
{
	int i;
	if (initialized)
		return dba_error_ok();

	for (i = 0; i < tranges_count; ++i)
		tdescs[i] = def_desc;

	DBA_RUN_OR_RETURN(read_strings("tranges", tdescs, tranges_count));

	initialized = 1;
	return dba_error_ok();
}


dba_err dba_formatter_describe_level(int ltype, int l1, int l2, char** buf)
{
	if (ltype < 0 || ltype >= levels_count)
	{
		return dba_error_consistency("level type to format (%d) is not between 0 and %d (inclusive)", ltype, levels_count);
	}

	DBA_RUN_OR_RETURN(want_levels());

	if (asprintf(buf, ldescs[ltype], l1, l2) == -1)
		return dba_error_system("formatting description string %s (ltype: %d, l1: %d, l2: %d)",
				ldescs[ltype], ltype, l1, l2);

	return dba_error_ok();
}

dba_err dba_formatter_describe_trange(int ptype, int p1, int p2, char** buf)
{
	if (ptype < 0 || ptype >= tranges_count)
	{
		return dba_error_consistency("time range to format (%d) is not between 0 and %d (inclusive)", ptype, tranges_count);
	}

	DBA_RUN_OR_RETURN(want_tranges());

	if (asprintf(buf, tdescs[ptype], p1, p2) == -1)
		return dba_error_system("formatting description string %s (ltype: %d, l1: %d, l2: %d)",
				tdescs[ptype], ptype, p1, p2);

	return dba_error_ok();
}


/* vim:set ts=4 sw=4: */
