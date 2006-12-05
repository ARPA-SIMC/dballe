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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>		/* malloc, strtod, getenv */
#include <string.h>		/* strncmp */
#include <ctype.h>		/* isspace */
#include <math.h>		/* rint */
#include <assert.h>		/* assert */
#include <limits.h>		/* PATH_MAX, INT_MIN, INT_MAX */

#if 0
#include <stdarg.h>
#include <strings.h>	/* bzero */
#include <fcntl.h>		/* O_RDONLY */
#endif

#include <dballe/core/dba_var.h>
#include <dballe/core/aliases.h>

DBA_ARR_DEFINE(dba_varcode, varcode);

struct _dba_vartable
{
	char id[20];
	int size;
	int alloc_size;
	struct _dba_varinfo* items;
};

static dba_vartable* tables = NULL;
static int tables_size = 0;
static int tables_alloc_size = 0;

static dba_err dba_vartable_read(const char* id, int* index);

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


/* Table to use to resolve DBALLE WMO parameter names */
static dba_vartable local_vars = NULL;

dba_err dba_varinfo_query_local(dba_varcode code, dba_varinfo* info)
{
	/* Load dballe WMO parameter resolution table */
	if (local_vars == NULL)
		DBA_RUN_OR_RETURN(dba_vartable_create("dballe", &local_vars));
	return dba_vartable_query(local_vars, code, info);
}

dba_err dba_varinfo_query_local_altered(dba_varcode code, dba_alteration change, dba_varinfo* info)
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

dba_err dba_vartable_create(const char* id, dba_vartable* table)
{
	int begin = -1, end = tables_size;

	/* Binary search the vartable in the index */
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;

		if (strcmp(tables[cur]->id, id) > 0)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || strcmp(tables[begin]->id, id) != 0)
		/* Table not found: load it */
		DBA_RUN_OR_RETURN(dba_vartable_read(id, &begin));

	*table = tables[begin];

	assert(strcmp(id, (*table)->id) == 0);

	return dba_error_ok();
}

const char* dba_vartable_id(dba_vartable table)
{
	return table->id;
}

dba_err dba_vartable_query(dba_vartable table, dba_varcode var, dba_varinfo* info)
{
	int begin, end;

	/* Then, binary search the varinfo value */
	begin = -1, end = table->size;
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (table->items[cur].var > var)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || table->items[begin].var != var)
		return dba_error_notfound(
				"looking up variable informations for variable %d%02d%03d in table %s",
				DBA_VAR_F(var), DBA_VAR_X(var), DBA_VAR_Y(var), table->id);
	else
		*info = &(table->items[begin]);

	assert((*info)->var == var);

	return dba_error_ok();
}

dba_err dba_vartable_query_altered(dba_vartable table, dba_varcode var, dba_alteration change, dba_varinfo* info)
{
	if (change == 0 || change == DBA_ALT(0, 0))
		return dba_vartable_query(table, var, info);

	/* Get the normal variable */
	dba_varinfo start, i;
	DBA_RUN_OR_RETURN(dba_vartable_query(table, var, &start));

	/* Look for an existing alteration */
	for (i = start; i->alteration != change && i->alterations != NULL ; i = i->alterations)
		;

	if (i->alteration != change)
	{
		/* Not found: we need to create it */
		int alt;

		/* Duplicate the original varinfo */
		i->alterations = (dba_varinfo)malloc(sizeof(struct _dba_varinfo));
		memcpy(i->alterations, start, sizeof(struct _dba_varinfo));
		i = i->alterations;

		fprintf(stderr, "Before alteration(w:%d,s:%d): bl %d len %d scale %d\n",
				DBA_ALT_WIDTH(change), DBA_ALT_SCALE(change),
				i->bit_len, i->len, i->scale);

		/* Apply the alterations */
		if ((alt = DBA_ALT_WIDTH(change)) != 0)
		{
			i->bit_len += alt;
			i->len = (int)ceil(log10(1 << i->bit_len));
		}
		if ((alt = DBA_ALT_SCALE(change)) != 0)
			i->scale += alt;

		fprintf(stderr, "After alteration(w:%d,s:%d): bl %d len %d scale %d\n",
				DBA_ALT_WIDTH(change), DBA_ALT_SCALE(change),
				i->bit_len, i->len, i->scale);

		/* Postprocess the data, filling in minval and maxval */
		if (!i->is_string)
		{
			if (i->len >= 10)
			{
				i->imin = INT_MIN;
				i->imax = INT_MAX;
			} else {
				i->imin = -(int)(exp10(i->len) - 1.0);
				i->imax = (int)(exp10(i->len) - 1.0);
			}
			i->dmin = dba_var_decode_int(i->imin, i);
			i->dmax = dba_var_decode_int(i->imax, i);
		}

		i->alteration = change;
		i->alterations = NULL;
	}

	*info = i;
	return dba_error_ok();
}

dba_err dba_vartable_iterate(dba_vartable table, dba_vartable_iterator func, void* data)
{
	int i;
	for (i = 0; i < table->size; i++)
		func(table->items + i, data);
	return dba_error_ok();
}

dba_varcode dba_descriptor_code(const char* entry)
{
	int res = 0;
	switch (entry[0])
	{
		case 'B':
		case '0':
			res = 0; break;
		case 'R':
		case '1':
			res = 1 << 14; break;
		case 'C':
		case '2':
			res = 2 << 14; break;
		case 'D':
		case '3':
			res = 3 << 14; break;
		default:
			return dba_varcode_alias_resolve(entry);
	}
	return res | DBA_STRING_TO_VAR(entry+1);
}

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

static dba_err vtb_create(dba_vartable* v, const char* id)
{
	/* We do not need calloc, since the table file parsing routine will fill in
	 * all the fields */

	/* Allocate space for the structure */
	if (((*v) = (dba_vartable)malloc(sizeof(struct _dba_vartable))) == NULL)
		return dba_error_alloc("allocating space for a new dba_vartable");

	/* Allocate space for the items */
	(*v)->items = (struct _dba_varinfo*)malloc(1500 * sizeof(struct _dba_varinfo));
	if ((*v)->items == NULL)
	{
		free(*v);
		*v = NULL;
		return dba_error_alloc("allocating space for 1500 dba_varinfo structures");
	}
	(*v)->alloc_size = 1500;
	(*v)->size = 0;
	strncpy((*v)->id, id, 18);
	(*v)->id[19] = 0;
	return dba_error_ok();
}

static void vtb_delete(dba_vartable v)
{
	if (v->items != NULL)
		free(v->items);
	free(v);
}

static dba_err vtb_new_entry(dba_vartable v, dba_varinfo* info)
{
	if (v->size >= v->alloc_size)
	{
		/* Need to enlarge the buffer */
		struct _dba_varinfo* orig = v->items;
		v->items = (struct _dba_varinfo*)realloc(
						v->items,
						(v->alloc_size + 20) * sizeof(struct _dba_varinfo));
		if (v->items == NULL)
		{
			v->items = orig;
			return dba_error_alloc("Enlarging varinfo table by 20 items");
		}
		v->alloc_size += 20;
	}
	*info = v->items + v->size;
	v->size++;
	return dba_error_ok();
}

static dba_err dba_vartable_read(const char* id, int* index)
{
	dba_err err = DBA_OK;
	dba_vartable vartable = NULL;
	const char* file = id_to_pathname(id);
	FILE* in = fopen(file, "rt");
	char line[200];
	int line_no = 0;
	dba_varcode last_code = 0;

	if (in == NULL)
		return dba_error_system("opening BUFR/CREX table file %s", file);

	DBA_RUN_OR_GOTO(cleanup, vtb_create(&vartable, id));

	while (fgets(line, 200, in) != NULL)
	{
		/* FMT='(1x,A,1x,A64,47x,A24,I3,8x,I3)' */
		int i;
		dba_varinfo entry = NULL;

		DBA_RUN_OR_GOTO(cleanup, vtb_new_entry(vartable, &entry));

		/*fprintf(stderr, "Line: %s\n", line);*/

		line_no++;

		if (strlen(line) < 119)
		{
			err = dba_error_parse(file, line_no, "line too short");
			goto cleanup;
		}

		/* Read starting B code */
		/*fprintf(stderr, "Entry: B%05d\n", bcode);*/
		entry->var = DBA_STRING_TO_VAR(line + 2);

		if (entry->var < last_code)
		{
			err = dba_error_parse(file, line_no, "input file is not sorted");
			goto cleanup;
		}
		last_code = entry->var;

		/* Read the description */
		memcpy(entry->desc, line+8, 64);
		/* Zero-terminate the description */
		for (i = 63; i >= 0 && isspace(entry->desc[i]); i--)
			;
		entry->desc[i+1] = 0;
		
		/* Read the BUFR type */
		memcpy(entry->unit, line+73, 24);
		/* Zero-terminate the type */
		for (i = 23; i >= 0 && isspace(entry->unit[i]); i--)
			;
		entry->unit[i+1] = 0;

		entry->is_string = (
				strcmp(entry->unit, "CCITTIA5") == 0 /*||
				strncmp(entry->unit, "CODE TABLE", 10) == 08*/
		);

		entry->scale = strtol(line+98, 0, 10);
		entry->bit_ref = strtol(line+102, 0, 10);
		entry->bit_len = strtol(line+115, 0, 10);

		if (strlen(line) < 157)
		{
			entry->ref = 0;
			if (entry->is_string)
			{
				entry->len = entry->bit_len / 8;
			} else {
				int len = 1 << entry->bit_len;
				for (entry->len = 0; len != 0; entry->len++)
					len /= 10;
			}
		} else {
			int crex_is_string;

			/* Read the CREX type */
			memcpy(entry->unit, line+119, 24);
			/* Zero-terminate the type */
			for (i = 23; i >= 0 && isspace(entry->unit[i]); i--)
				;
			entry->unit[i+1] = 0;

			entry->scale = strtol(line+138, 0, 10);
			entry->ref = 0;
			entry->len = strtol(line+149, 0, 10);

			crex_is_string = (
					strcmp(entry->unit, "CHARACTER") == 0 /* ||
					strncmp(entry->unit, "CODE TABLE", 10) == 0 */
			);

			if (entry->is_string != crex_is_string)
			{
				err = dba_error_parse(file, line_no,
						"CREX is_string (%d) is different than BUFR is_string (%d)",
						crex_is_string, entry->is_string);
				goto cleanup;
			}
		}

		/* Postprocess the data, filling in minval and maxval */
		if (entry->is_string)
		{
			entry->imin = entry->imax = 0;
			entry->dmin = entry->dmax = 0.0;
		} else {
			if (entry->len >= 10)
			{
				entry->imin = INT_MIN;
				entry->imax = INT_MAX;
			} else {
				entry->imin = -(int)(exp10(entry->len) - 1.0);
				entry->imax = (int)(exp10(entry->len) - 1.0);
			}
			entry->dmin = dba_var_decode_int(entry->imin, entry);
			entry->dmax = dba_var_decode_int(entry->imax, entry);
		}

		entry->alteration = 0;
		entry->alterations = NULL;

		/*
		fprintf(stderr, "Debug: B%05d len %d scale %d type %s desc %s\n",
				bcode, entry->len, entry->scale, entry->type, entry->desc);
		*/
	}

	/* The file parsed successfully: file the new data under the appropriate
	 * slot */
	DBA_RUN_OR_GOTO(cleanup, vartable_insert(vartable, index));
	
cleanup:
	if (in != NULL)
		fclose(in);
	if (err != DBA_OK)
		vtb_delete(vartable);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
