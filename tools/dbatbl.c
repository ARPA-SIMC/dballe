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

#define _GNU_SOURCE  /* Needed for strcasestr */

#include <dballe/cmdline.h>
#include <dballe/bufrex/bufrex_dtable.h>
#include <dballe/core/conv.h>
#include <dballe/formatter.h>

#include <stdlib.h>
#include <string.h>

dba_vartable btable;
bufrex_dtable dtable;
static int op_csv;
int op_verbose = 0;

void unit_check(dba_varinfo info, void* data)
{
	dba_varcode varcode = info->var;
	dba_varinfo local;
	int* is_ok = (int*)data;

	/*
	if (var % 1000 == 0)
		fprintf(stderr, "Testing %s %d\n", ids[i], var);
	*/

	if (varcode != 0 && !info->is_string)
	{
		double dval;
		if (dba_varinfo_query_local(varcode, &local) != DBA_OK)
		{
			fprintf(stderr, "Checking conversion for var B%02d%03d: ",
					DBA_VAR_X(varcode), DBA_VAR_Y(varcode));
			dba_cmdline_print_dba_error();
			*is_ok = 0;
		}
		else if (dba_convert_units(info->unit, local->unit, 1.0, &dval) != DBA_OK)
		{
			fprintf(stderr, "Checking conversion for var B%02d%03d: ",
					DBA_VAR_X(varcode), DBA_VAR_Y(varcode));
			dba_cmdline_print_dba_error();
			*is_ok = 0;
		}
	}
}


/* Check that all unit conversions are allowed by dba_uniconv */
static dba_err check_unit_conversions(const char* id, int* is_ok)
{
	dba_vartable othertable;
	DBA_RUN_OR_RETURN(dba_vartable_create(id, &othertable));

	*is_ok = 1;
	return dba_vartable_iterate(othertable, unit_check, is_ok);
}

static void print_varinfo(dba_varinfo info)
{
	char fmtdesc[100];

	if (info->is_string)
		snprintf(fmtdesc, 99, "%d characters", info->len);
	else if (info->scale == 0)
		snprintf(fmtdesc, 99, "%d digits", info->len);
	else if (info->scale > 0)
	{
		int i, j;
		for (i = 0; i < info->len - info->scale && i < 99; i++)
			fmtdesc[i] = '#';
		fmtdesc[i++] = '.';
		for (j = 0; j < info->scale && i < 99; i++, j++)
			fmtdesc[i] = '#';
		fmtdesc[i] = 0;
	}
	else if (info->scale < 0)
	{
		int i, j;
		for (i = 0; i < info->len && i < 99; i++)
			fmtdesc[i] = '#';
		for (j = 0; j < -info->scale && i < 99; i++, j++)
			fmtdesc[i] = '0';
		fmtdesc[i] = 0;
	}

#if 0
	if (info->is_string) 
		printf("%d%02d%03d %s [%s, %s]\n", DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var),
				info->desc,
				info->unit,
				fmtdesc);
	else
		printf("%d%02d%03d %s [%s, %s] %f<=x<=%f\n", DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var),
				info->desc,
				info->unit,
				fmtdesc,
				info->dmin,
				info->dmax);
#else
	printf("%d%02d%03d %s [%s, %s]\n", DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var),
			info->desc,
			info->unit,
			fmtdesc);
#endif
}

static void print_varinfo_csv(dba_varinfo info)
{
	char fmtdesc[100];
	const char* s;

	if (info->is_string)
		snprintf(fmtdesc, 99, "%d characters", info->len);
	else if (info->scale == 0)
		snprintf(fmtdesc, 99, "%d digits", info->len);
	else if (info->scale > 0)
	{
		int i, j;
		for (i = 0; i < info->len - info->scale && i < 99; i++)
			fmtdesc[i] = '#';
		fmtdesc[i++] = '.';
		for (j = 0; j < info->scale && i < 99; i++, j++)
			fmtdesc[i] = '#';
		fmtdesc[i] = 0;
	}
	else if (info->scale < 0)
	{
		int i, j;
		for (i = 0; i < info->len && i < 99; i++)
			fmtdesc[i] = '#';
		for (j = 0; j < -info->scale && i < 99; i++, j++)
			fmtdesc[i] = '0';
		fmtdesc[i] = 0;
	}

	printf("%d%02d%03d,", DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var));
	for (s = info->desc; *s != 0; ++s)
		if (*s != ',' && *s != '"')
			putc(*s, stdout);
	printf(",%s,%s\n", info->unit, fmtdesc);
}

static dba_err expand_table_entry(dba_varcode val, int level)
{
	int i;
	for (i = 0; i < level; i++)
		printf("\t");

	switch (DBA_VAR_F(val))
	{
		case 0:
		{
			dba_varinfo info;
			DBA_RUN_OR_RETURN(dba_vartable_query(btable, val, &info));
			print_varinfo(info);
			break;
		}
		case 3:
		{
			bufrex_opcode exp;
			bufrex_opcode cur;
			DBA_RUN_OR_RETURN(bufrex_dtable_query(dtable, val, &exp));

			printf("%d%02d%03d\n", DBA_VAR_F(val), DBA_VAR_X(val), DBA_VAR_Y(val));

			for (cur = exp; cur != NULL; cur = cur->next)
				expand_table_entry(cur->val, level+1);

			bufrex_opcode_delete(&exp);

			break;
		}
		default:
			printf("%d%02d%03d\n", DBA_VAR_F(val), DBA_VAR_X(val), DBA_VAR_Y(val));
	}

	return dba_error_ok();
}

static void print_varinfo_adapter(dba_varinfo info, void* data)
{
	if (op_csv)
		print_varinfo_csv(info);
	else
		print_varinfo(info);
}

static void print_varinfo_grepping_adapter(dba_varinfo info, void* data)
{
	const char* pattern = (const char*)data;

	if (strcasestr(info->desc, pattern) != NULL)
	{
		if (op_csv)
			print_varinfo_csv(info);
		else
			print_varinfo(info);
	}
}

dba_err do_cat(poptContext optCon)
{
	const char* item;

	/* Throw away the command name */
	poptGetArg(optCon);

	if (poptPeekArg(optCon) == NULL)
		item = "dballe";
	else
		item = poptGetArg(optCon);

	while (item != NULL)
	{
		dba_vartable table;
		DBA_RUN_OR_RETURN(dba_vartable_create(item, &table));
		DBA_RUN_OR_RETURN(dba_vartable_iterate(table, print_varinfo_adapter, NULL));

		item = poptGetArg(optCon);
	}
	
	return dba_error_ok();
}

dba_err do_grep(poptContext optCon)
{
	dba_vartable table;
	const char* pattern;

	/* Throw away the command name */
	poptGetArg(optCon);

	if (poptPeekArg(optCon) == NULL)
		dba_cmdline_error(optCon, "there should be at least one B or D item to expand.  Examples are: B01002 or D03001");
	pattern = poptGetArg(optCon);

	DBA_RUN_OR_RETURN(dba_vartable_create("dballe", &table));
	DBA_RUN_OR_RETURN(dba_vartable_iterate(table, print_varinfo_grepping_adapter, (void*)pattern));
	
	return dba_error_ok();
}

dba_err do_expand(poptContext optCon)
{
	const char* item;

	/* Throw away the command name */
	poptGetArg(optCon);

	DBA_RUN_OR_RETURN(dba_vartable_create("dballe", &btable));
	DBA_RUN_OR_RETURN(bufrex_dtable_create("D000203", &dtable));

	if (poptPeekArg(optCon) == NULL)
		dba_cmdline_error(optCon, "there should be at least one B or D item to expand.  Examples are: B01002 or D03001");

	while ((item = poptGetArg(optCon)) != NULL)
		DBA_RUN_OR_RETURN(expand_table_entry(dba_descriptor_code(item), 0));
	
	return dba_error_ok();
}

dba_err do_expandcode(poptContext optCon)
{
	const char* item;

	/* Throw away the command name */
	poptGetArg(optCon);
	while ((item = poptGetArg(optCon)) != NULL)
	{
		int code = strtol(item, NULL, 10);
		char c = 'B';
		switch (DBA_VAR_F(code))
		{
			case 0: c = 'B'; break;
			case 1: c = 'R'; break;
			case 2: c = 'C'; break;
			case 3: c = 'D'; break;
		}
		printf("%s: %c%02d%03d\n", item, c, DBA_VAR_X(code), DBA_VAR_Y(code));
	}
	
	return dba_error_ok();
}

static char* table_type = "b";

dba_err do_index(poptContext optCon)
{
	const char* file;
	const char* id;

	/* Throw away the command name */
	poptGetArg(optCon);
	file = poptGetArg(optCon);
	id = poptGetArg(optCon);
	if (file == NULL)
		dba_cmdline_error(optCon, "input file has not been specified");
	if (id == NULL)
		dba_cmdline_error(optCon, "indexed table ID has not been specified");

	if (strcmp(table_type, "b") == 0)
	{
		if (strcmp(id, "dballe") != 0)
		{
			/* If it's an external table, check unit conversions to DBALLE
			 * correspondents */
			int is_ok;
			DBA_RUN_OR_RETURN(check_unit_conversions(id, &is_ok));
			if (!is_ok)
				fprintf(stderr, "Warning: some variables cannot be converted from %s to dballe\n", id);
		}
	}
	else if (strcmp(table_type, "d") == 0)
		; /* DBA_RUN_OR_RETURN(bufrex_dtable_index(file, id)); */
	/*
	else if (strcmp(type, "conv") == 0)
		DBA_RUN_OR_RETURN(bufrex_convtable_index_csv(file, id));
	*/
	else
		dba_cmdline_error(optCon, "'%s' is not a valid table type", table_type);

	return dba_error_ok();
}

dba_err do_describe(poptContext optCon)
{
	const char* what;

	/* Throw away the command name */
	poptGetArg(optCon);
	if ((what = poptGetArg(optCon)) == NULL)
		dba_cmdline_error(optCon, "you need to specify what you want to describe.  Available options are: 'level' and 'trange'");

	if (strcmp(what, "level") == 0)
	{
		const char* sltype = poptGetArg(optCon);
		const char* sl1 = poptGetArg(optCon);
		const char* sl2 = poptGetArg(optCon);
		char* formatted;
		if (sltype == NULL)
			dba_cmdline_error(optCon, "you need provide 1, 2 or 3 numbers that identify the level");
		DBA_RUN_OR_RETURN(dba_formatter_describe_level(
				strtoul(sltype, NULL, 10),
				sl1 == NULL ? 0 : strtoul(sl1, NULL, 10),
				sl2 == NULL ? 0 : strtoul(sl2, NULL, 10),
				&formatted));
		puts(formatted);
		free(formatted);
	} else if (strcmp(what, "trange") == 0) {
		const char* sptype = poptGetArg(optCon);
		const char* sp1 = poptGetArg(optCon);
		const char* sp2 = poptGetArg(optCon);
		char* formatted;
		if (sptype == NULL)
			dba_cmdline_error(optCon, "you need provide 1, 2 or 3 numbers that identify the time range");
		DBA_RUN_OR_RETURN(dba_formatter_describe_trange(
				strtoul(sptype, NULL, 10),
				sp1 == NULL ? 0 : strtoul(sp1, NULL, 10),
				sp2 == NULL ? 0 : strtoul(sp2, NULL, 10),
				&formatted));
		puts(formatted);
		free(formatted);
	} else
		dba_cmdline_error(optCon, "cannot handle %s.  Available options are: 'level' and 'trange'.", what);
	
	return dba_error_ok();
}


static struct tool_desc dbatbl;

struct poptOption dbatbl_cat_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "csv", 'c', POPT_ARG_NONE, &op_csv, 0, "output variables in CSV format" },
	POPT_TABLEEND
};

struct poptOption dbatbl_expand_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	POPT_TABLEEND
};

struct poptOption dbatbl_expandcode_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	POPT_TABLEEND
};

struct poptOption dbatbl_index_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "type", 't', POPT_ARG_STRING, &table_type, 0,
		"format of the table to index ('b', 'd', 'conv')", "type" },
	POPT_TABLEEND
};

struct poptOption dbatbl_describe_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	POPT_TABLEEND
};

static void init()
{
	dbatbl.desc = "Manage on-disk reference tables for DB-ALLe";
	dbatbl.longdesc =
		"This tool allows to index and query the tables that are "
		"needed for normal functioning of DB-ALLe";
	dbatbl.ops = (struct op_dispatch_table*)calloc(7, sizeof(struct op_dispatch_table));

	dbatbl.ops[0].func = do_cat;
	dbatbl.ops[0].aliases[0] = "cat";
	dbatbl.ops[0].usage = "cat tableid [tableid [...]]";
	dbatbl.ops[0].desc = "Output all the contents of a WMO B table.";
	dbatbl.ops[0].longdesc = NULL;
	dbatbl.ops[0].optable = dbatbl_cat_options;

	dbatbl.ops[1].func = do_grep;
	dbatbl.ops[1].aliases[0] = "grep";
	dbatbl.ops[1].usage = "grep string";
	dbatbl.ops[1].desc = "Output all the contents of the local B table whose description contains the given string.";
	dbatbl.ops[1].longdesc = NULL;
	dbatbl.ops[1].optable = dbatbl_cat_options;

	dbatbl.ops[2].func = do_expand;
	dbatbl.ops[2].aliases[0] = "expand";
	dbatbl.ops[2].usage = "expand table-entry [table-entry [...]]";
	dbatbl.ops[2].desc = "Describe a WMO B table entry or expand a WMO D table entry in its components.";
	dbatbl.ops[2].longdesc = NULL;
	dbatbl.ops[2].optable = dbatbl_expand_options;

	dbatbl.ops[3].func = do_expandcode;
	dbatbl.ops[3].aliases[0] = "expandcode";
	dbatbl.ops[3].usage = "expandcode varcode [varcode [...]]";
	dbatbl.ops[3].desc = "Expand the value of a packed variable code";
	dbatbl.ops[3].longdesc = NULL;
	dbatbl.ops[3].optable = dbatbl_expandcode_options;

	dbatbl.ops[4].func = do_index;
	dbatbl.ops[4].aliases[0] = "index";
	dbatbl.ops[4].usage = "index [options] filename index-id";
	dbatbl.ops[4].desc = "Index the contents of a table file";
	dbatbl.ops[4].longdesc = NULL;
	dbatbl.ops[4].optable = dbatbl_index_options;

	dbatbl.ops[5].func = do_describe;
	dbatbl.ops[5].aliases[0] = "describe";
	dbatbl.ops[5].usage = "describe [options] what [values]";
	dbatbl.ops[5].desc = "Invoke the formatter to describe the given values";
	dbatbl.ops[5].longdesc = "Supported so far are: \"level ltype l1 l2\", \"trange pind p1 p2\"";
	dbatbl.ops[5].optable = dbatbl_describe_options;

	dbatbl.ops[6].func = NULL;
	dbatbl.ops[6].usage = NULL;
	dbatbl.ops[6].desc = NULL;
	dbatbl.ops[6].longdesc = NULL;
	dbatbl.ops[6].optable = NULL;
};

int main (int argc, const char* argv[])
{
	init();
	return dba_cmdline_dispatch_main(&dbatbl, argc, argv);
}

/* vim:set ts=4 sw=4: */
