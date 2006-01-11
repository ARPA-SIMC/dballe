#include <dballe/msg/dba_msg.h>

#include <dballe/aof/aof_decoder.h>
#include <dballe/dba_cmdline.h>

#include "processor.h"
#include "conversion.h"

#include <stdlib.h>

static int op_use_local_vars = 0;
static int op_dump_interpreted = 0;
static char* op_input_type = "bufr";
static char* op_output_type = "bufr";

struct grep_t grepdata = { -1, -1, -1, 0, "" };
struct poptOption grepTable[] = {
	{ "category", 0, POPT_ARG_INT, &grepdata.category, 0,
		"match messages with the given data category", "num" },
	{ "subcategory", 0, POPT_ARG_INT, &grepdata.subcategory, 0,
		"match BUFR messages with the given data subcategory", "num" },
	{ "check-digit", 0, POPT_ARG_INT, &grepdata.checkdigit, 0,
		"match CREX messages with check digit (if 1) or without check digit (if 0)", "num" },
	{ "unparsable", 0, 0, &grepdata.unparsable, 0,
		"match only messages that cannot be parsed" },
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

static dba_err print_aof_header(dba_rawmsg rmsg, dba_msg msg)
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

static dba_err summarise_bufr_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	DBA_RUN_OR_RETURN(print_bufr_header(rmsg, braw)); puts(".");
	return dba_error_ok();
}

static dba_err summarise_crex_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	DBA_RUN_OR_RETURN(print_crex_header(rmsg, braw)); puts(".");
	return dba_error_ok();
}

static dba_err summarise_aof_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	DBA_RUN_OR_RETURN(print_aof_header(rmsg, msg)); puts(".");
	return dba_error_ok();
}

static dba_err dump_dba_vars(bufrex_raw msg)
{
	int i;

	for (i = 0; i < msg->vars_count; i++)
	{
		dba_var var;
		dba_varcode code;
		dba_varinfo info;

		if (op_use_local_vars)
			DBA_RUN_OR_RETURN(dba_var_to_local(msg->vars[i], &var));
		else
			var = msg->vars[i];
		code = dba_var_code(var);
		
		info = dba_var_info(var);

		dba_var_print(var, stdout);

		if (op_use_local_vars)
			dba_var_delete(var);
	}

	return dba_error_ok();
}

static dba_err dump_bufr_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	DBA_RUN_OR_RETURN(print_bufr_header(rmsg, braw)); puts(":");
	DBA_RUN_OR_RETURN(dump_dba_vars(braw));
	return dba_error_ok();
}

static dba_err dump_crex_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	DBA_RUN_OR_RETURN(print_crex_header(rmsg, braw)); puts(":");
	DBA_RUN_OR_RETURN(dump_dba_vars(braw));
	return dba_error_ok();
}

static dba_err dump_aof_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	DBA_RUN_OR_RETURN(print_aof_header(rmsg, msg)); puts(":");
	dba_msg_print(msg, stdout);
	/* DBA_RUN_OR_RETURN(dump_dba_vars((dba_message)msg)); */
	return dba_error_ok();
}

static dba_err dump_cooked_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	dba_msg_print(msg, stdout);
	return dba_error_ok();
}

dba_err write_raw_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	dba_file file = (dba_file)data;
	DBA_RUN_OR_RETURN(dba_file_write_raw(file, rmsg));
	return dba_error_ok();
}

dba_err do_scan(poptContext optCon)
{
	action bufraction;
	action crexaction;
	action aofaction;

	/* Throw away the command name */
	poptGetArg(optCon);
	bufraction = summarise_bufr_message;
	crexaction = summarise_crex_message;
	aofaction = summarise_aof_message;

	switch (dba_cmdline_stringToMsgType(op_input_type, optCon))
	{
		case BUFR: DBA_RUN_OR_RETURN(process_all(optCon, BUFR, &grepdata, bufraction, 0)); break;
		case CREX: DBA_RUN_OR_RETURN(process_all(optCon, CREX, &grepdata, crexaction, 0)); break;
		case AOF:  DBA_RUN_OR_RETURN(process_all(optCon, AOF, &grepdata, aofaction, 0)); break;
	}

	return dba_error_ok();
}

dba_err do_dump(poptContext optCon)
{
	action crexaction;
	action bufraction;
	action aofaction;

	/* Throw away the command name */
	poptGetArg(optCon);
	if (op_dump_interpreted)
	{
		bufraction = dump_cooked_message;
		crexaction = dump_cooked_message;
		aofaction = dump_cooked_message;
	} else {
		bufraction = dump_bufr_message;
		crexaction = dump_crex_message;
		aofaction = dump_aof_message;
	}

	switch (dba_cmdline_stringToMsgType(op_input_type, optCon))
	{
		case BUFR: DBA_RUN_OR_RETURN(process_all(optCon, BUFR, &grepdata, bufraction, 0)); break;
		case CREX: DBA_RUN_OR_RETURN(process_all(optCon, CREX, &grepdata, crexaction, 0)); break;
		case AOF:  DBA_RUN_OR_RETURN(process_all(optCon, AOF, &grepdata, aofaction, 0)); break;
	}

	return dba_error_ok();
}

dba_err do_cat(poptContext optCon)
{
	/* Throw away the command name */
	dba_encoding type;
	dba_file file;

	poptGetArg(optCon);

	type = dba_cmdline_stringToMsgType(op_input_type, optCon);

	DBA_RUN_OR_RETURN(dba_file_create(&file, type, "(stdout)", "w"));
	/*DBA_RUN_OR_RETURN(aof_file_write_header(file, 0, 0)); */
	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, write_raw_message, file));
	dba_file_delete(file);

	return dba_error_ok();
}

dba_err do_convert(poptContext optCon)
{
	dba_file file;
	dba_encoding type;

	/* Throw away the command name */
	poptGetArg(optCon);

	type = dba_cmdline_stringToMsgType(op_output_type, optCon);

	DBA_RUN_OR_RETURN(dba_file_create(&file, type, "(stdout)", "w"));
	/* DBA_RUN_OR_RETURN(dba_file_write_header(file, 0, 0)); */

	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, convert_message, (void*)file));

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
		int diffs;
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

dba_err do_filter(poptContext optCon)
{
	dba_encoding type;

	/* Throw away the command name */
	poptGetArg(optCon);

	type = dba_cmdline_stringToMsgType(op_input_type, optCon);

	dba_file file;
	DBA_RUN_OR_RETURN(dba_file_create(&file, type, "(stdout)", "w"));
	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, write_raw_message, file));
	/*DBA_RUN_OR_RETURN(aof_file_write_header(file, 0, 0)); */
	dba_file_delete(file);

	return dba_error_ok();
}

static struct tool_desc dbamsg;

struct poptOption dbamsg_scan_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "local", 0, 0, &op_use_local_vars, 0,
		"convert variables to the DBALLE standard value before dumping their values" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_dump_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the unput data ('bufr', 'crex', 'aof')", "type" },
	{ "local", 0, 0, &op_use_local_vars, 0,
		"convert variables to the DBALLE standard value before dumping their values" },
	{ "interpreted", 0, 0, &op_dump_interpreted, 0,
		"dump the message as understood by the importer" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_cat_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_convert_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ "local", 0, 0, &op_use_local_vars, 0,
		"convert variables to the DBALLE standard value before dumping their values" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_compare_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
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
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

static void init()
{
	dbamsg.desc = "Work with encoded metereological data";
	dbamsg.longdesc =
		"Examine, dump and convert files containing metereological data. "
		"It supports observations encoded in BUFR, CREX and AOF formats";
	dbamsg.ops = (struct op_dispatch_table*)calloc(7, sizeof(struct op_dispatch_table));

	dbamsg.ops[0].func = do_scan;
	dbamsg.ops[0].aliases[0] = "scan";
	dbamsg.ops[0].usage = "scan [options] filename [filename [...]]";
	dbamsg.ops[0].desc = "Summarise the contents of a file with metereological data";
	dbamsg.ops[0].longdesc = NULL;
	dbamsg.ops[0].optable = dbamsg_scan_options;

	dbamsg.ops[1].func = do_dump;
	dbamsg.ops[1].aliases[0] = "dump";
	dbamsg.ops[1].usage = "dump [options] filename [filename [...]]";
	dbamsg.ops[1].desc = "Dump the contents of a file with metereological data";
	dbamsg.ops[1].longdesc = NULL;
	dbamsg.ops[1].optable = dbamsg_dump_options;

	dbamsg.ops[2].func = do_cat;
	dbamsg.ops[2].aliases[0] = "cat";
	dbamsg.ops[2].usage = "cat [options] filename [filename [...]]";
	dbamsg.ops[2].desc = "Dump the raw data of a file with metereological data";
	dbamsg.ops[2].longdesc = NULL;
	dbamsg.ops[2].optable = dbamsg_cat_options;

	dbamsg.ops[3].func = do_convert;
	dbamsg.ops[3].aliases[0] = "convert";
	dbamsg.ops[3].aliases[1] = "conv";
	dbamsg.ops[3].usage = "convert [options] filename [filename [...]]";
	dbamsg.ops[3].desc = "Convert metereological data between different formats";
	dbamsg.ops[3].longdesc = NULL;
	dbamsg.ops[3].optable = dbamsg_convert_options;

	dbamsg.ops[4].func = do_compare;
	dbamsg.ops[4].aliases[0] = "compare";
	dbamsg.ops[4].aliases[1] = "cmp";
	dbamsg.ops[4].usage = "compare [options] filename1 [filename2]";
	dbamsg.ops[4].desc = "Compare two files with metereological data";
	dbamsg.ops[4].longdesc = NULL;
	dbamsg.ops[4].optable = dbamsg_compare_options;

	dbamsg.ops[5].func = do_filter;
	dbamsg.ops[5].aliases[0] = "filter";
	dbamsg.ops[5].usage = "filter [options] filename [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbamsg.ops[5].desc = "Copy only those messages whose contents match the given query parameters";
	dbamsg.ops[5].longdesc = NULL;
	dbamsg.ops[5].optable = dbamsg_filter_options;

	dbamsg.ops[6].func = NULL;
	dbamsg.ops[6].usage = NULL;
	dbamsg.ops[6].desc = NULL;
	dbamsg.ops[6].longdesc = NULL;
	dbamsg.ops[6].optable = NULL;
};

int main (int argc, const char* argv[])
{
	init();
	return dba_cmdline_dispatch_main(&dbamsg, argc, argv);
}

/* vim:set ts=4 sw=4: */
