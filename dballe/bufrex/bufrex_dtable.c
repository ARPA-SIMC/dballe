#define _GNU_SOURCE
#include "config.h"

#include "bufrex_dtable.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>		/* malloc, strtod, getenv */
#include <string.h>		/* strncmp */
#include <strings.h>	/* bzero */
#include <ctype.h>		/* isspace */
#include <assert.h>		/* assert */
#include <limits.h>		/* PATH_MAX */
#include <fcntl.h>		/* O_RDONLY */

struct _bdt_entry {
	dba_varcode code;
	int first;
	int count;
};

struct _bufrex_dtable
{
	char id[20];
	int code_size;
	int code_alloc;
	dba_varcode* codes;
	int tbl_size;
	int tbl_alloc;
	struct _bdt_entry* entries; 
};

static bufrex_dtable* tables = NULL;
static int tables_size = 0;
static int tables_alloc_size = 0;

static dba_err bufrex_dtable_read(const char* id, int* index);

/**
 * Insert a new dtable index in 'tables', keeping it sorted
 *
 * @returns The position at which the new table index can now be found
 */
static dba_err dtable_insert(bufrex_dtable v, int* index)
{
	int pos;

	if (tables == NULL)
	{
		/* First initialization special case */
		if ((tables = (bufrex_dtable*)malloc(sizeof(bufrex_dtable) * 10)) == NULL)
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
			bufrex_dtable* old_tables = tables;

			if ((tables = (bufrex_dtable*)realloc(tables, sizeof(bufrex_dtable) * (tables_alloc_size + 10))) == NULL)
			{
				tables = old_tables;
				return dba_error_alloc("allocating space for 10 more vartable indexes");
			}
			tables_alloc_size += 10;
		}

		/* Shift the next items forward */
		memmove(tables + pos + 1, tables + pos, (tables_size - pos) * sizeof(bufrex_dtable));
	}

	tables[pos] = v;
	++tables_size;
	*index = pos;

	return dba_error_ok();
}

dba_err bufrex_dtable_create(const char* id, bufrex_dtable* table)
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
		DBA_RUN_OR_RETURN(bufrex_dtable_read(id, &begin));

	*table = tables[begin];

	assert(strcmp(id, (*table)->id) == 0);

	return dba_error_ok();
}

dba_err bufrex_dtable_query(bufrex_dtable table, dba_varcode var, bufrex_opcode* chain)
{
	int begin, end;

	/* Then, binary search the varinfo value */
	begin = -1, end = table->tbl_size;
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (table->entries[cur].code > var)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || table->entries[begin].code != var)
		return dba_error_notfound(
				"looking up D table expansion for variable %d%02d%03d in table %s",
				DBA_VAR_F(var), DBA_VAR_X(var), DBA_VAR_Y(var), table->id);
	else
	{
		int first = table->entries[begin].first;
		int count = table->entries[begin].count;
		int i;

		*chain = 0;
		for (i = 0; i < count; i++)
		{
			dba_varcode code = table->codes[first + i];
			dba_err err;
			if ((err = bufrex_opcode_append(chain, code)) != DBA_OK)
			{
				bufrex_opcode_delete(chain);
				return err;
			}
		}
	}

	return dba_error_ok();
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

static dba_err dtb_create(bufrex_dtable* v, const char* id)
{
	/* We do not need calloc, since the table file parsing routine will fill in
	 * all the fields */

	/* Allocate space for the structure */
	if (((*v) = (bufrex_dtable)malloc(sizeof(struct _bufrex_dtable))) == NULL)
		return dba_error_alloc("allocating space for a new bufrex_dtable");

	/* Allocate space for the items */
	if (((*v)->codes = (dba_varcode*)malloc(3000 * sizeof(dba_varcode))) == NULL)
	{
		free(*v);
		*v = NULL;
		return dba_error_alloc("allocating space for 3000 dba_varcode structures");
	}
	if (((*v)->entries = (struct _bdt_entry*)malloc(400 * sizeof(struct _bdt_entry))) == NULL)
	{
		free((*v)->codes);
		free(*v);
		*v = NULL;
		return dba_error_alloc("allocating space for 400 bufrex_dtable entries");
	}

	strncpy((*v)->id, id, 18);
	(*v)->id[19] = 0;
	(*v)->code_alloc = 3000;
	(*v)->code_size = 0;
	(*v)->tbl_alloc = 400;
	(*v)->tbl_size = 0;

	return dba_error_ok();
}

static void dtb_delete(bufrex_dtable v)
{
	if (v->codes != NULL)
		free(v->codes);
	if (v->entries != NULL)
		free(v->entries);
	free(v);
}

static dba_err dtb_append_entry(bufrex_dtable v, dba_varcode code)
{
	if (v->tbl_size >= v->tbl_alloc)
	{
		/* Need to enlarge the buffer */
		struct _bdt_entry* orig = v->entries;
		v->entries = (struct _bdt_entry*)realloc(
						v->entries,
						(v->tbl_alloc + 10) * sizeof(struct _bdt_entry));
		if (v->entries == NULL)
		{
			v->entries = orig;
			return dba_error_alloc("Enlarging bufrex_dtable entries table by 10 items");
		}
		v->tbl_alloc += 10;
	}
	v->entries[v->tbl_size].code = code;
	v->entries[v->tbl_size].first = v->code_size;
	v->entries[v->tbl_size].count = 0;
	++v->tbl_size;
	return dba_error_ok();
}

static dba_err dtb_append_code(bufrex_dtable v, dba_varcode code)
{
	if (v->code_size >= v->code_alloc)
	{
		/* Need to enlarge the buffer */
		dba_varcode* orig = v->codes;
		v->codes = (dba_varcode*)realloc(
						v->codes,
						(v->code_alloc + 30) * sizeof(dba_varcode));
		if (v->codes == NULL)
		{
			v->codes = orig;
			return dba_error_alloc("Enlarging bufrex_dtable codes table by 30 items");
		}
		v->code_alloc += 30;
	}
	v->codes[v->code_size++] = code;
	v->entries[v->tbl_size - 1].count++;
	return dba_error_ok();
}

static dba_err bufrex_dtable_read(const char* id, int* index)
{
	dba_err err = DBA_OK;
	bufrex_dtable dtable = NULL;
	const char* file = id_to_pathname(id);
	FILE* in = fopen(file, "rt");
	char line[200];
	int line_no = 0;
	int nentries_check = 0;

	if (in == NULL)
		return dba_error_system("opening D table file %s", file);

	DBA_RUN_OR_GOTO(cleanup, dtb_create(&dtable, id));

	while (fgets(line, 200, in) != NULL)
	{
		line_no++;

		/* fprintf(stderr, "Line: %s\n", line); */
		
		if (strlen(line) < 18)
		{
			err = dba_error_parse(file, line_no, "line too short");
			goto cleanup;
		}

		if (line[1] == 'D' || line[1] == '3')
		{
			int last_count = dtable->tbl_size > 0 ? dtable->entries[dtable->tbl_size - 1].count : 0;
			if (last_count != nentries_check)
			{
				err = dba_error_parse(file, line_no, "advertised number of expansion items (%d) does not match the number of items found (%d)", nentries_check, last_count);
				goto cleanup;
			}

			nentries_check = strtol(line + 8, 0, 10);
			if (nentries_check < 1)
			{
				err = dba_error_parse(file, line_no, "less than one entry advertised in the expansion");
				goto cleanup;
			}

			/* Append the entry */
			DBA_RUN_OR_GOTO(cleanup, dtb_append_entry(dtable, dba_descriptor_code(line + 1)));

			/* Append the first code belonging to this entry */
			DBA_RUN_OR_GOTO(cleanup, dtb_append_code(dtable, dba_descriptor_code(line + 11)));

			/* fprintf(stderr, "Debug: D%05d %d entries\n", dcode, nentries); */
		}
		else if (strncmp(line, "           ", 11) == 0)
		{
			int last_count;
			/* Check that there has been at least one entry filed before */
			if (dtable->tbl_size == 0)
			{
				err = dba_error_parse(file, line_no, "expansion line found before the first entry");
				goto cleanup;
			}
			/* Check that we are not appending too many entries */
			last_count = dtable->entries[dtable->tbl_size - 1].count;
			if (last_count == nentries_check)
			{
				err = dba_error_parse(file, line_no, "too many entries found (expected %d)", nentries_check);
				goto cleanup;
			}

			/* Finally append the code */
			DBA_RUN_OR_GOTO(cleanup, dtb_append_code(dtable, dba_descriptor_code(line + 11)));
		}
		else
		{
			err = dba_error_parse(file, line_no, "unrecognized line: \"%s\"", line);
			goto cleanup;
		}
	}

	/* Check that we actually read something */
	if (dtable->tbl_size == 0)
	{
		err = dba_error_parse(file, line_no, "no entries found in the file");
		goto cleanup;
	}
	
	/* Check that the last entry is complete */
	if (dtable->entries[dtable->tbl_size - 1].count != nentries_check)
	{
		err = dba_error_parse(file, line_no, "advertised number of expansion items (%d) does not match the number of items found (%d)", nentries_check, dtable->entries[dtable->tbl_size - 1].count);
		goto cleanup;
	}

	/* The file parsed successfully: file the new data under the appropriate
	 * slot */
	DBA_RUN_OR_GOTO(cleanup, dtable_insert(dtable, index));
	
cleanup:
	if (in != NULL)
		fclose(in);
	if (err != DBA_OK)
		dtb_delete(dtable);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
