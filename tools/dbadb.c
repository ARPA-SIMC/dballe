#include <dballe/msg/dba_msg.h>

#include <dballe/dba_init.h>
#include <dballe/dba_file.h>
/*
#include <dballe/bufrex/bufr_file.h>
#include <dballe/bufrex/crex_file.h>
#include <dballe/aof/aof_file.h>

#include <dballe/bufrex/bufrex_conv.h>
#include <dballe/aof/aof_conv.h>
*/

#include <dballe/db/dballe.h>
#include <dballe/db/dba_import.h>
#include <dballe/db/dba_export.h>

#include <dballe/bufrex/bufrex.h>

#include <dballe/dba_cmdline.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "processor.h"

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

static char* op_dsn = "test";
static char* op_user = "test";
static char* op_pass = "";
static char* op_input_type = "bufr";
static char* op_output_type = "bufr";
static char* op_output_template = "";
static char* op_overwrite = 0;

struct poptOption dbTable[] = {
	{ "dsn", 0, POPT_ARG_STRING, &op_dsn, 0,
		"DSN to use for connecting to the DBALLE database" },
	{ "user", 0, POPT_ARG_STRING, &op_user, 0,
		"username to use for connecting to the DBALLE database" },
	{ "pass", 0, POPT_ARG_STRING, &op_pass, 0,
		"password to use for connecting to the DBALLE database" },
	POPT_TABLEEND
};

struct import_data
{
	dba db;
	int overwrite;
};

static dba_err import_message(dba_rawmsg rmsg, bufrex_raw braw, dba_msg msg, void* data)
{
	struct import_data* d = (struct import_data*)data;
	DBA_RUN_OR_RETURN(dba_import_msg(d->db, msg, d->overwrite));
	return dba_error_ok();
}

dba_err do_dump(poptContext optCon)
{
	const char* action;
	int count, i;
	dba_record query, result;
	dba_cursor cursor;
	dba db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	DBA_RUN_OR_RETURN(dba_init());
	DBA_RUN_OR_RETURN(dba_open(op_dsn, op_user, op_pass, &db));
	DBA_RUN_OR_RETURN(dba_query(db, query, &cursor, &count));
	DBA_RUN_OR_RETURN(dba_record_create(&result));

	for (i = 0; i < count; i++)
	{
		dba_varcode var;
		int is_last;
		DBA_RUN_OR_RETURN(dba_cursor_next(cursor, result, &var, &is_last));
		printf("#%d: -----------------------\n", i);
		dba_record_print(result, stdout);
	}

	dba_close(db);
	dba_shutdown();

	dba_record_delete(result);
	dba_record_delete(query);

	return dba_error_ok();
}

dba_err do_wipe(poptContext optCon)
{
	const char* action;
	const char* table;
	dba db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Get the optional name of the */
	table = poptGetArg(optCon);

	DBA_RUN_OR_RETURN(dba_init());
	DBA_RUN_OR_RETURN(dba_open(op_dsn, op_user, op_pass, &db));
	DBA_RUN_OR_RETURN(dba_reset(db, table));
	dba_close(db);
	dba_shutdown();

	return dba_error_ok();
}

dba_err do_import(poptContext optCon)
{
	dba_encoding type;
	struct import_data data;

	/* Throw away the command name */
	poptGetArg(optCon);

	type = dba_cmdline_stringToMsgType(op_input_type, optCon);

	DBA_RUN_OR_RETURN(dba_init());
	DBA_RUN_OR_RETURN(dba_open(op_dsn, op_user, op_pass, &data.db));
	data.overwrite = op_overwrite;

	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, import_message, (void*)&data));

	dba_close(data.db);
	dba_shutdown();

	return dba_error_ok();
}

dba_err do_export(poptContext optCon)
{
	const char* action;
	const char* datatype;
	int exp_type = 0, exp_subtype = 0, i, rep_cod;
	dba_file file;
	dba_encoding type;
	dba_msg* msgs = NULL;
	dba_record query;
	dba db;

	/* Throw away the command name */
	action = poptGetArg(optCon);
	datatype = poptGetArg(optCon);

	if (op_output_template[0] != 0)
		if (sscanf(op_output_template, "%d.%d", &exp_type, &exp_subtype) != 2)
			dba_cmdline_error(optCon, "output template must be specified as 'type.subtype' (type number, then dot, then subtype number)");

	/* Connect to the database */
	DBA_RUN_OR_RETURN(dba_init());
	DBA_RUN_OR_RETURN(dba_open(op_dsn, op_user, op_pass, &db));

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));

	/* Obtain the report code to query */
	if (isdigit(datatype[0]))
		rep_cod = atoi(datatype);
	else
		DBA_RUN_OR_RETURN(dba_rep_cod_from_memo(db, datatype, &rep_cod));

	/* Query the wanted report code */
	DBA_RUN_OR_RETURN(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod));

	/* Add the rest of the query */
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	switch (rep_cod)
	{
		case 1:		DBA_RUN_OR_RETURN(dba_db_export(db, MSG_SYNOP, &msgs, query)); break;
		case 2:		DBA_RUN_OR_RETURN(dba_db_export(db, MSG_GENERIC, &msgs, query)); break;
		case 3:		DBA_RUN_OR_RETURN(dba_db_export(db, MSG_TEMP, &msgs, query)); break;
		case 9:		DBA_RUN_OR_RETURN(dba_db_export(db, MSG_BUOY, &msgs, query)); break;
		case 10:	DBA_RUN_OR_RETURN(dba_db_export(db, MSG_SHIP, &msgs, query)); break;
		case 11:	DBA_RUN_OR_RETURN(dba_db_export(db, MSG_TEMP_SHIP, &msgs, query)); break;
		case 12:	DBA_RUN_OR_RETURN(dba_db_export(db, MSG_AIREP, &msgs, query)); break;
		case 13:	DBA_RUN_OR_RETURN(dba_db_export(db, MSG_AMDAR, &msgs, query)); break;
		case 14:	DBA_RUN_OR_RETURN(dba_db_export(db, MSG_ACARS, &msgs, query)); break;
		default:	DBA_RUN_OR_RETURN(dba_db_export(db, MSG_GENERIC, &msgs, query)); break;
	}

	if (msgs == NULL)
		return dba_error_ok();

	type = dba_cmdline_stringToMsgType(op_output_type, optCon);
	DBA_RUN_OR_RETURN(dba_file_create(&file, type, "(stdout)", "w"));
	switch (type)
	{
		case BUFR:
			for (i = 0; msgs[i] != NULL; i++)
			{
				dba_rawmsg raw;
				DBA_RUN_OR_RETURN(bufrex_encode_bufr(msgs[i], exp_type, exp_subtype, &raw));
				DBA_RUN_OR_RETURN(dba_file_write_raw(file, raw));
				dba_rawmsg_delete(raw);
				dba_msg_delete(msgs[i]);
			}
			break;
		case CREX:
			for (i = 0; msgs[i] != NULL; i++)
			{
				dba_rawmsg raw;
				DBA_RUN_OR_RETURN(bufrex_encode_crex(msgs[i], exp_type, exp_subtype, &raw));
				DBA_RUN_OR_RETURN(dba_file_write_raw(file, raw));
				dba_rawmsg_delete(raw);
				dba_msg_delete(msgs[i]);
			}
			
			break;
		case AOF: 
			return dba_error_unimplemented("export to AOF format");
	}
	dba_file_delete(file);
	free(msgs);
	dba_close(db);
	dba_shutdown();

	dba_record_delete(query);

	return dba_error_ok();
}

static struct tool_desc dbadb;

struct poptOption dbadb_dump_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_wipe_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_import_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "overwrite", 'f', POPT_ARG_NONE, &op_overwrite, 0,
		"overwrite existing data" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbadb_export_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ "template", 't', POPT_ARG_STRING, &op_output_template, 0,
		"template of the data in output (autoselect if not specified)", "type.sub" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

static void init()
{
	dbadb.desc = "Manage the DB-ALLe database";
	dbadb.longdesc =
		"It allows to initialise the database, dump its contents and import and export data "
		"using BUFR, CREX or AOF encoding";
	dbadb.ops = (struct op_dispatch_table*)calloc(5, sizeof(struct op_dispatch_table));

	dbadb.ops[0].func = do_dump;
	dbadb.ops[0].aliases[0] = "dump";
	dbadb.ops[0].usage = "dump [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbadb.ops[0].desc = "Dump data from the database";
	dbadb.ops[0].longdesc = NULL;
	dbadb.ops[0].optable = dbadb_dump_options;

	dbadb.ops[1].func = do_wipe;
	dbadb.ops[1].aliases[0] = "wipe";
	dbadb.ops[1].usage = "wipe [options] [optional rep_type description file]";
	dbadb.ops[1].desc = "Reinitialise the database, removing all data";
	dbadb.ops[1].longdesc =
			"Reinitialisation is done using the given report type description file. "
			"If no file is provided, a default version is used";
	dbadb.ops[1].optable = dbadb_wipe_options;

	dbadb.ops[2].func = do_import;
	dbadb.ops[2].aliases[0] = "import";
	dbadb.ops[2].usage = "import [options] filename [filename [ ... ] ]";
	dbadb.ops[2].desc = "Import data into the database";
	dbadb.ops[2].longdesc = NULL;
	dbadb.ops[2].optable = dbadb_import_options;

	dbadb.ops[3].func = do_export;
	dbadb.ops[3].aliases[0] = "export";
	dbadb.ops[3].usage = "export [options] type [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbadb.ops[3].desc = "Export data from the database";
	dbadb.ops[3].longdesc = NULL;
	dbadb.ops[3].optable = dbadb_export_options;

	dbadb.ops[4].func = NULL;
	dbadb.ops[4].usage = NULL;
	dbadb.ops[4].desc = NULL;
	dbadb.ops[4].longdesc = NULL;
	dbadb.ops[4].optable = NULL;
};

int main (int argc, const char* argv[])
{
	init();
	return dba_cmdline_dispatch_main(&dbadb, argc, argv);
}

/* vim:set ts=4 sw=4: */
