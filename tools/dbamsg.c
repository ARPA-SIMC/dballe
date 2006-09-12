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

#include <dballe/msg/dba_msg.h>

#include <dballe/aof/decoder.h>
#include <dballe/core/dba_record.h>
#include <dballe/io/dba_rawfile.h>
#include <dballe/io/writers.h>
#include <dballe/cmdline.h>

#include "processor.h"
#include "conversion.h"

#include <stdlib.h>

static int op_dump_interpreted = 0;
static char* op_input_type = "auto";
static char* op_output_type = "bufr";
int op_verbose = 0;

struct grep_t grepdata = { -1, -1, -1, 0, 0, "" };
struct poptOption grepTable[] = {
	{ "category", 0, POPT_ARG_INT, &grepdata.category, 0,
		"match messages with the given data category", "num" },
	{ "subcategory", 0, POPT_ARG_INT, &grepdata.subcategory, 0,
		"match BUFR messages with the given data subcategory", "num" },
	{ "check-digit", 0, POPT_ARG_INT, &grepdata.checkdigit, 0,
		"match CREX messages with check digit (if 1) or without check digit (if 0)", "num" },
	{ "unparsable", 0, 0, &grepdata.unparsable, 0,
		"match only messages that cannot be parsed" },
	{ "parsable", 0, 0, &grepdata.parsable, 0,
		"match only messages that can be parsed" },
	{ "index", 0, POPT_ARG_STRING, &grepdata.index, 0,
		"match messages with the index in the given range (ex.: 1-5,9,22-30)", "expr" },
	POPT_TABLEEND
};

static int count_nonnulls(bufrex_raw raw)
{
	int i, count = 0;
	for (i = 0; i < raw->vars_count; i++)
		if (dba_var_value(raw->vars[i]) != NULL)
			count++;
	return count;
}

static dba_err print_bufr_header(dba_rawmsg rmsg, bufrex_raw braw)
{
	int size;
	const char *table_id;
	const unsigned char *buf;

	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(rmsg, &buf, &size));
	DBA_RUN_OR_RETURN(bufrex_raw_get_table_id(braw, &table_id));

	printf("#%d BUFR message: %d bytes, category %d, subcategory %d, table %s, %d/%d values",
			rmsg->index, size, braw->type, braw->subtype, table_id, count_nonnulls(braw), braw->vars_count);

	return dba_error_ok();
}

static dba_err print_crex_header(dba_rawmsg rmsg, bufrex_raw braw)
{
	int size/*, checkdigit*/;
	const char *table_id;
	const unsigned char *buf;

	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(rmsg, &buf, &size));
	DBA_RUN_OR_RETURN(bufrex_raw_get_table_id(braw, &table_id));

	/* DBA_RUN_OR_RETURN(crex_message_has_check_digit(msg, &checkdigit)); */

	printf("#%d CREX message: %d bytes, category %d, subcategory %d, table %s, %scheck digit, %d/%d values",
			rmsg->index, size, braw->type, braw->subtype, table_id, /*checkdigit ? "" : "no "*/"? ", count_nonnulls(braw), braw->vars_count);

	return dba_error_ok();
}

static dba_err print_aof_header(dba_rawmsg rmsg)
{
	int size, category, subcategory;
	const unsigned char *buf;

	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(rmsg, &buf, &size));
	DBA_RUN_OR_RETURN(aof_decoder_get_category(rmsg, &category, &subcategory));
	/* DBA_RUN_OR_RETURN(bufrex_message_get_vars(msg, &vars, &count)); */

	printf("#%d AOF message: %d bytes, category %d, subcategory %d",
			rmsg->index, size, category, subcategory);

	return dba_error_ok();
}

static dba_err summarise_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	switch (rmsg->encoding)
	{
		case BUFR:
			if (braw == NULL) return dba_error_ok();
			DBA_RUN_OR_RETURN(print_bufr_header(rmsg, braw)); puts(".");
			break;
		case CREX:
			if (braw == NULL) return dba_error_ok();
			DBA_RUN_OR_RETURN(print_crex_header(rmsg, braw)); puts(".");
			break;
		case AOF:
			DBA_RUN_OR_RETURN(print_aof_header(rmsg)); puts(".");
			break;
	}
	return dba_error_ok();
}

static dba_err dump_dba_vars(bufrex_raw msg)
{
	int i;

	for (i = 0; i < msg->vars_count; i++)
		dba_var_print(msg->vars[i], stdout);

	return dba_error_ok();
}

static dba_err dump_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	switch (rmsg->encoding)
	{
		case BUFR:
			if (braw == NULL) return dba_error_ok();
			DBA_RUN_OR_RETURN(print_bufr_header(rmsg, braw)); puts(":");
			DBA_RUN_OR_RETURN(dump_dba_vars(braw));
			break;
		case CREX:
			if (braw == NULL) return dba_error_ok();
			DBA_RUN_OR_RETURN(print_crex_header(rmsg, braw)); puts(":");
			DBA_RUN_OR_RETURN(dump_dba_vars(braw));
			break;
		case AOF:
			DBA_RUN_OR_RETURN(print_aof_header(rmsg)); puts(":");
			aof_decoder_dump(rmsg, stdout);
			break;
	}
	return dba_error_ok();
}

static dba_err dump_cooked_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	if (msg == NULL) return dba_error_ok();
	printf("#%d ", rmsg->index);
	dba_msg_print(msg, stdout);
	return dba_error_ok();
}

dba_err write_raw_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	dba_file* file = (dba_file*)data;
	if (*file == NULL)
		DBA_RUN_OR_RETURN(dba_file_create(file, rmsg->encoding, "(stdout)", "w"));
	DBA_RUN_OR_RETURN(dba_file_write_raw(*file, rmsg));
	return dba_error_ok();
}

dba_err do_scan(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	DBA_RUN_OR_RETURN(process_all(optCon, 
				dba_cmdline_stringToMsgType(op_input_type, optCon),
				&grepdata, summarise_message, 0));

	return dba_error_ok();
}

dba_err do_dump(poptContext optCon)
{
	action action = op_dump_interpreted ? dump_cooked_message : dump_message;

	/* Throw away the command name */
	poptGetArg(optCon);

	DBA_RUN_OR_RETURN(process_all(optCon, 
				dba_cmdline_stringToMsgType(op_input_type, optCon),
				&grepdata, action, 0));

	return dba_error_ok();
}

dba_err do_cat(poptContext optCon)
{
	/* Throw away the command name */
	dba_file file = NULL;

	poptGetArg(optCon);

	/*DBA_RUN_OR_RETURN(aof_file_write_header(file, 0, 0)); */
	DBA_RUN_OR_RETURN(process_all(optCon, 
				dba_cmdline_stringToMsgType(op_input_type, optCon),
				&grepdata, write_raw_message, &file));
	if (file != NULL)
		dba_file_delete(file);

	return dba_error_ok();
}

dba_err do_convert(poptContext optCon)
{
	dba_file file;
	dba_encoding intype, outtype;

	/* Throw away the command name */
	poptGetArg(optCon);

	intype = dba_cmdline_stringToMsgType(op_input_type, optCon);
	outtype = dba_cmdline_stringToMsgType(op_output_type, optCon);

	DBA_RUN_OR_RETURN(dba_file_create(&file, outtype, "(stdout)", "w"));
	/* DBA_RUN_OR_RETURN(dba_file_write_header(file, 0, 0)); */

	DBA_RUN_OR_RETURN(process_all(optCon, intype, &grepdata, convert_message, (void*)file));

	dba_file_delete(file);

	return dba_error_ok();
}

dba_err do_compare(poptContext optCon)
{
	dba_file file1;
	dba_file file2;
	const char* file1_name;
	const char* file2_name;
	dba_msg msg1;
	dba_msg msg2;
	int found1 = 1, found2 = 1;
	int idx = 0;

	/* Throw away the command name */
	poptGetArg(optCon);

	/* Read the file names */
	file1_name = poptGetArg(optCon);
	if (file1_name == NULL)
		dba_cmdline_error(optCon, "input file needs to be specified");

	file2_name = poptGetArg(optCon);
	if (file2_name == NULL)
		file2_name = "(stdin)";

	DBA_RUN_OR_RETURN(dba_file_create(&file1, dba_cmdline_stringToMsgType(op_input_type, optCon), file1_name, "r"));
	DBA_RUN_OR_RETURN(dba_file_create(&file2, dba_cmdline_stringToMsgType(op_output_type, optCon), file2_name, "r"));

	while (found1 && found2)
	{
		int diffs = 0;
		idx++;
		DBA_RUN_OR_RETURN(dba_file_read(file1, &msg1, &found1));
		DBA_RUN_OR_RETURN(dba_file_read(file2, &msg2, &found2));

		if (found1 != found2)
			return dba_error_consistency("The files contain a different number of messages");

		if (found1 && found2)
		{
			dba_msg_diff(msg1, msg2, &diffs, stderr);
			if (diffs > 0)
				return dba_error_consistency("Messages #%d contain %d differences", idx, diffs);

			dba_msg_delete(msg1);
			dba_msg_delete(msg2);
		}
	}

	dba_file_delete(file1);
	dba_file_delete(file2);

	return dba_error_ok();
}

struct filter_data
{
	dba_file output;
	int mindate[6];
	int maxdate[6];
	double latmin, latmax, lonmin, lonmax;
};

inline static int get_date_value(dba_msg msg, int value)
{
	dba_msg_datum d = dba_msg_find_by_id(msg, value);
	dba_var var = d == NULL ? NULL : d->var;
	const char* val = var == NULL ? NULL : dba_var_value(var);
	if (val == NULL)
		return -1;
	else
		return strtoul(val, 0, 10);
}

inline static double get_lat_value(dba_msg msg, int value)
{
	double res;
	dba_msg_datum d = dba_msg_find_by_id(msg, value);
	dba_var var = d == NULL ? NULL : d->var;
	if (var == NULL || dba_var_value(var) == NULL)
		return -1000000;
	dba_var_enqd(var, &res);
	return res;
}


dba_err filter_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	struct filter_data* fdata = (struct filter_data*)data;
	double dval;
	
	if (msg == NULL) return dba_error_ok();

	if (fdata->mindate[0] != -1 || fdata->maxdate[0] != -1)
	{
		int i, date[6];
		date[0] = get_date_value(msg, DBA_MSG_YEAR);
		date[1] = get_date_value(msg, DBA_MSG_MONTH);
		date[2] = get_date_value(msg, DBA_MSG_DAY);
		date[3] = get_date_value(msg, DBA_MSG_HOUR);
		date[4] = get_date_value(msg, DBA_MSG_MINUTE);
		date[5] = 0;

		/* Check that the message contains proper datetime indications */
		for (i = 0; i < 6; i++)
			if (date[i] == -1)
				return dba_error_ok();

		/* Compare with the two extremes */

		if (fdata->mindate[0] != -1)
			for (i = 0; i < 6; i++)
				if (fdata->mindate[i] > date[i])
					return dba_error_ok();

		if (fdata->maxdate[0] != -1)
			for (i = 0; i < 6; i++)
				if (fdata->maxdate[i] < date[i])
					return dba_error_ok();
	}

	/* Latitude and longitude must exist */
	if ((dval = get_lat_value(msg, DBA_MSG_LATITUDE)) == -1000000)
		return dba_error_ok();
	if (fdata->latmin != -1000000 && fdata->latmin > dval)
		return dba_error_ok();
	if (fdata->latmax != -1000000 && fdata->latmax < dval)
		return dba_error_ok();

	if ((dval = get_lat_value(msg, DBA_MSG_LONGITUDE)) == -1000000)
		return dba_error_ok();
	if (fdata->lonmin != -1000000 && fdata->lonmin > dval)
		return dba_error_ok();
	if (fdata->lonmax != -1000000 && fdata->lonmax < dval)
		return dba_error_ok();

	DBA_RUN_OR_RETURN(dba_file_write_raw(fdata->output, rmsg));

	return dba_error_ok();
}

inline static double get_input_lat_value(dba_record rec, dba_keyword key)
{
	dba_var var = dba_record_key_peek(rec, key); 
	double res;
	if (var == NULL)
		return -1000000;
	if (dba_var_value(var) == NULL)
		return -1000000;
	dba_var_enqd(var, &res);
	return res;
}

dba_err do_filter(poptContext optCon)
{
	dba_encoding type;
	dba_encoding otype;
	struct filter_data fdata;
	dba_record query;

	/* Throw away the command name */
	poptGetArg(optCon);

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	/* Digest the query in good values for filter_data */
	DBA_RUN_OR_RETURN(dba_record_parse_date_extremes(query, fdata.mindate, fdata.maxdate));
	fdata.latmin = get_input_lat_value(query, DBA_KEY_LATMIN);
	fdata.latmax = get_input_lat_value(query, DBA_KEY_LATMAX);
	fdata.lonmin = get_input_lat_value(query, DBA_KEY_LONMIN);
	fdata.lonmax = get_input_lat_value(query, DBA_KEY_LONMAX);

	type = dba_cmdline_stringToMsgType(op_input_type, optCon);
	otype = dba_cmdline_stringToMsgType(op_output_type, optCon);

	DBA_RUN_OR_RETURN(dba_file_create(&fdata.output, otype, "(stdout)", "w"));
	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, filter_message, &fdata));
	/*DBA_RUN_OR_RETURN(aof_file_write_header(file, 0, 0)); */
	dba_file_delete(fdata.output);

	return dba_error_ok();
}

dba_err do_fixaof(poptContext optCon)
{
	dba_err err = DBA_OK;
	const char* filename;
	dba_rawfile file = NULL;
	int count = 0;

	/* Throw away the command name */
	poptGetArg(optCon);

	while ((filename = poptGetArg(optCon)) != NULL)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_rawfile_create(&file, filename, "rb+"));
		DBA_RUN_OR_GOTO(cleanup, aof_writer_fix_header(file));
		dba_rawfile_delete(file);
		file = NULL;
		count++;
	}

	if (count == 0)
		dba_cmdline_error(optCon, "at least one input file needs to be specified");

cleanup:
	if (file != NULL)
		dba_rawfile_delete(file);
	return err == DBA_OK ? dba_error_ok() : err;
}


static struct tool_desc dbamsg;

struct poptOption dbamsg_scan_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_dump_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the unput data ('bufr', 'crex', 'aof')", "type" },
	{ "interpreted", 0, 0, &op_dump_interpreted, 0,
		"dump the message as understood by the importer" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_cat_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_convert_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_compare_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type1", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the first file to compare ('bufr', 'crex', 'aof')", "type" },
	{ "type2", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the second file to compare ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_filter_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_fixaof_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	POPT_TABLEEND
};

static void init()
{
	dbamsg.desc = "Work with encoded meteorological data";
	dbamsg.longdesc =
		"Examine, dump and convert files containing meteorological data. "
		"It supports observations encoded in BUFR, CREX and AOF formats";
	dbamsg.ops = (struct op_dispatch_table*)calloc(8, sizeof(struct op_dispatch_table));

	dbamsg.ops[0].func = do_scan;
	dbamsg.ops[0].aliases[0] = "scan";
	dbamsg.ops[0].usage = "scan [options] filename [filename [...]]";
	dbamsg.ops[0].desc = "Summarise the contents of a file with meteorological data";
	dbamsg.ops[0].longdesc = NULL;
	dbamsg.ops[0].optable = dbamsg_scan_options;

	dbamsg.ops[1].func = do_dump;
	dbamsg.ops[1].aliases[0] = "dump";
	dbamsg.ops[1].usage = "dump [options] filename [filename [...]]";
	dbamsg.ops[1].desc = "Dump the contents of a file with meteorological data";
	dbamsg.ops[1].longdesc = NULL;
	dbamsg.ops[1].optable = dbamsg_dump_options;

	dbamsg.ops[2].func = do_cat;
	dbamsg.ops[2].aliases[0] = "cat";
	dbamsg.ops[2].usage = "cat [options] filename [filename [...]]";
	dbamsg.ops[2].desc = "Dump the raw data of a file with meteorological data";
	dbamsg.ops[2].longdesc = NULL;
	dbamsg.ops[2].optable = dbamsg_cat_options;

	dbamsg.ops[3].func = do_convert;
	dbamsg.ops[3].aliases[0] = "convert";
	dbamsg.ops[3].aliases[1] = "conv";
	dbamsg.ops[3].usage = "convert [options] filename [filename [...]]";
	dbamsg.ops[3].desc = "Convert meteorological data between different formats";
	dbamsg.ops[3].longdesc = NULL;
	dbamsg.ops[3].optable = dbamsg_convert_options;

	dbamsg.ops[4].func = do_compare;
	dbamsg.ops[4].aliases[0] = "compare";
	dbamsg.ops[4].aliases[1] = "cmp";
	dbamsg.ops[4].usage = "compare [options] filename1 [filename2]";
	dbamsg.ops[4].desc = "Compare two files with meteorological data";
	dbamsg.ops[4].longdesc = NULL;
	dbamsg.ops[4].optable = dbamsg_compare_options;

	dbamsg.ops[5].func = do_filter;
	dbamsg.ops[5].aliases[0] = "filter";
	dbamsg.ops[5].usage = "filter [options] filename [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbamsg.ops[5].desc = "Copy only those messages whose contents match the given query parameters";
	dbamsg.ops[5].longdesc = NULL;
	dbamsg.ops[5].optable = dbamsg_filter_options;

	dbamsg.ops[6].func = do_fixaof;
	dbamsg.ops[6].aliases[0] = "fixaof";
	dbamsg.ops[6].usage = "fixaof [options] filename [filename1 [...]]]";
	dbamsg.ops[6].desc = "Recomputes the start and end of observation period in the headers of the given AOF files";
	dbamsg.ops[6].longdesc = NULL;
	dbamsg.ops[6].optable = dbamsg_fixaof_options;

	dbamsg.ops[7].func = NULL;
	dbamsg.ops[7].usage = NULL;
	dbamsg.ops[7].desc = NULL;
	dbamsg.ops[7].longdesc = NULL;
	dbamsg.ops[7].optable = NULL;
};

int main (int argc, const char* argv[])
{
	init();
	return dba_cmdline_dispatch_main(&dbamsg, argc, argv);
}

/* vim:set ts=4 sw=4: */
