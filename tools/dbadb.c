/*
 * DB-ALLe - Archive for punctual meteorological data
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

#include <dballe/init.h>
#include <dballe/bufrex/msg.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/file.h>
#include <dballe/msg/bufrex_codec.h>
#include <dballe/db/db.h>
#include <dballe/db/import.h>
#include <dballe/db/export.h>
#include <extra/cmdline.h>
#include <extra/processor.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

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

static char* op_db = "";
static char* op_dbfile = "";
static char* op_dsn = "";
static char* op_user = "";
static char* op_pass = "";
static char* op_input_type = "auto";
static char* op_report = "";
static char* op_output_type = "bufr";
static char* op_output_template = "";
static int op_overwrite = 0;
static int op_fast = 0;
static int op_no_attrs = 0;
static int op_full_pseudoana = 0;
static int op_dump = 0;
int op_verbose = 0;

struct poptOption dbTable[] = {
	{ "db", 0, POPT_ARG_STRING, &op_db, 0,
		"URL-like definition of DB-All.e database (can also be specified in the environment as DBA_DB)" },
	{ "dbfile", 0, POPT_ARG_STRING, &op_dbfile, 0,
		"SQLite DB-All.e database" },
	{ "dsn", 0, POPT_ARG_STRING, &op_dsn, 0,
		"DSN to use for connecting to the DB-All.e database" },
	{ "user", 0, POPT_ARG_STRING, &op_user, 0,
		"username to use for connecting to the DB-All.e database" },
	{ "pass", 0, POPT_ARG_STRING, &op_pass, 0,
		"password to use for connecting to the DB-All.e database" },
	POPT_TABLEEND
};

static dba_err create_dba_db(dba_db* db)
{
	const char* fromenv;
	if (op_db[0] != 0) return dba_db_create_from_url(op_db, db);
	if (op_dbfile[0] != 0) return dba_db_create_from_file(op_dbfile, db);
	if (op_dsn[0] != 0) return dba_db_create(op_dsn, op_user, op_pass, db);
	fromenv = getenv("DBA_DB"); 
	if (fromenv != NULL) return dba_db_create_from_url(fromenv, db);
	return dba_error_consistency("no database specified");
}

struct import_data
{
	dba_db db;
	int overwrite;
	const char* forced_repmemo;
};

static dba_err import_message(dba_rawmsg rmsg, bufrex_msg braw, dba_msgs msgs, void* data)
{
	struct import_data* d = (struct import_data*)data;
	int i, import_flags = 0;;
	if (msgs == NULL)
	{
		fprintf(stderr, "Message #%d cannot be parsed: ignored\n",
				rmsg->index);
		return dba_error_ok();
	}
	if (d->overwrite)
		import_flags |= DBA_IMPORT_OVERWRITE;
	if (op_fast)
		import_flags |= DBA_IMPORT_NO_TRANSACTIONS;
	if (!op_no_attrs)
		import_flags |= DBA_IMPORT_ATTRS;
	if (op_full_pseudoana)
		import_flags |= DBA_IMPORT_FULL_PSEUDOANA;

	for (i = 0; i < msgs->len; ++i)
	{
		dba_msg msg = msgs->msgs[i];
		if (d->forced_repmemo == NULL && msg->type == MSG_GENERIC)
		{
			/* Put generic messages in the generic rep_cod by default */
			DBA_RUN_OR_RETURN(dba_import_msg(d->db, msg, NULL, import_flags));
		}
		else
		{
			DBA_RUN_OR_RETURN(dba_import_msg(d->db, msg, d->forced_repmemo, import_flags));
		}
	}
	return dba_error_ok();
}

dba_err do_dump(poptContext optCon)
{
	const char* action;
	int count, i;
	dba_record query, result;
	dba_db_cursor cursor;
	dba_db db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	DBA_RUN_OR_RETURN(create_dba_db(&db));
	DBA_RUN_OR_RETURN(dba_db_query(db, query, &cursor, &count));
	DBA_RUN_OR_RETURN(dba_record_create(&result));

	for (i = 0; ; ++i)
	{
		int has_data;
		DBA_RUN_OR_RETURN(dba_db_cursor_next(cursor, &has_data));
		if (!has_data)
			break;
		dba_record_clear(result);
		DBA_RUN_OR_RETURN(dba_db_cursor_to_record(cursor, result));
		printf("#%d: -----------------------\n", i);
		dba_record_print(result, stdout);
	}

	//fprintf(stdout, "***\n");
	//dba_record_print(query, stdout);

	dba_db_delete(db);

	dba_record_delete(result);
	dba_record_delete(query);

	return dba_error_ok();
}

dba_err do_stations(poptContext optCon)
{
	const char* action;
	int count, i;
	dba_record query, result;
	dba_db_cursor cursor;
	dba_db db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	DBA_RUN_OR_RETURN(create_dba_db(&db));
	DBA_RUN_OR_RETURN(dba_db_ana_query(db, query, &cursor, &count));
	DBA_RUN_OR_RETURN(dba_record_create(&result));

	for (i = 0; ; ++i)
	{
		int has_data;
		DBA_RUN_OR_RETURN(dba_db_cursor_next(cursor, &has_data));
		if (!has_data)
			break;
		dba_record_clear(result);
		DBA_RUN_OR_RETURN(dba_db_cursor_to_record(cursor, result));
		printf("#%d: -----------------------\n", i);
		dba_record_print(result, stdout);
	}

	dba_db_delete(db);

	dba_record_delete(result);
	dba_record_delete(query);

	return dba_error_ok();
}

dba_err do_wipe(poptContext optCon)
{
	const char* action;
	const char* table;
	dba_db db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Get the optional name of the repinfo file */
	table = poptGetArg(optCon);

	DBA_RUN_OR_RETURN(create_dba_db(&db));
	DBA_RUN_OR_RETURN(dba_db_reset(db, table));
	dba_db_delete(db);

	return dba_error_ok();
}

dba_err do_cleanup(poptContext optCon)
{
	const char* action;
	const char* table;
	dba_db db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Get the optional name of the repinfo file */
	table = poptGetArg(optCon);

	DBA_RUN_OR_RETURN(create_dba_db(&db));
	DBA_RUN_OR_RETURN(dba_db_remove_orphans(db));
	dba_db_delete(db);

	return dba_error_ok();
}

dba_err do_repinfo(poptContext optCon)
{
	int added, deleted, updated;
	const char* action;
	const char* table;
	dba_db db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Get the optional name of the repinfo file.  If missing, the default will be used */
	table = poptGetArg(optCon);

	DBA_RUN_OR_RETURN(create_dba_db(&db));
	DBA_RUN_OR_RETURN(dba_db_update_repinfo(db, table, &added, &deleted, &updated));
	printf("Update completed: %d added, %d deleted, %d updated.\n", added, deleted, updated);
	dba_db_delete(db);

	return dba_error_ok();
}

dba_err parse_op_report(dba_db db, const char** res)
{
	if (op_report[0] != 0)
	{
		const char* s;
		int is_cod = 1;
		for (s = op_report; *s && is_cod; s++)
			if (!isdigit(*s))
				is_cod = 0;
		
		if (is_cod)
			return dba_db_rep_memo_from_cod(db, strtoul(op_report, NULL, 0), res);
		else
		{
			*res = op_report;
			return dba_error_ok();
		}
	} else {
		*res = NULL;
		return dba_error_ok();
	}
}

dba_err do_import(poptContext optCon)
{
	dba_encoding type;
	struct import_data data;

	/* Throw away the command name */
	poptGetArg(optCon);

	type = dba_cmdline_stringToMsgType(op_input_type, optCon);

	DBA_RUN_OR_RETURN(create_dba_db(&data.db));
	data.overwrite = op_overwrite;
	DBA_RUN_OR_RETURN(parse_op_report(data.db, &(data.forced_repmemo)));

	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, import_message, (void*)&data));

	dba_db_delete(data.db);

	return dba_error_ok();
}

struct export_data
{
	dba_file file;
	int cat;
	int subcat;
	int localsubcat;
	const char* forced_rep_memo;
};

static dba_err msg_writer(dba_msgs msgs, void* data)
{
	struct export_data* d = (struct export_data*)data;
	/* Override the message type if the user asks for it */
	if (d->forced_rep_memo != NULL)
	{
		int i;
		for (i = 0; i < msgs->len; ++i)
			msgs->msgs[i]->type = dba_msg_type_from_repmemo(d->forced_rep_memo);
	}
	DBA_RUN_OR_RETURN(dba_file_write_msgs(d->file, msgs, d->cat, d->subcat, d->localsubcat));
	dba_msgs_delete(msgs);
	return dba_error_ok();
}

static dba_err msg_dumper(dba_msgs msgs, void* data)
{
	FILE* out = (FILE*)data;
	dba_msgs_print(msgs, out);
	dba_msgs_delete(msgs);
	return dba_error_ok();
}

dba_err do_export(poptContext optCon)
{
	struct export_data d = { NULL, 0, 0, 0, NULL };
	dba_encoding type;
	dba_record query;
	dba_db db;

	/* Throw away the command name */
	poptGetArg(optCon);

	if (op_output_template[0] != 0)
		DBA_RUN_OR_RETURN(bufrex_msg_parse_template(op_output_template, &d.cat, &d.subcat, &d.localsubcat));

	/* Connect to the database */
	DBA_RUN_OR_RETURN(create_dba_db(&db));

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));

	/* Add the query data from commandline */
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	if (op_dump)
	{
		DBA_RUN_OR_RETURN(dba_db_export(db, query, msg_dumper, stdout));
	} else {
		DBA_RUN_OR_RETURN(parse_op_report(db, &(d.forced_rep_memo)));

		type = dba_cmdline_stringToMsgType(op_output_type, optCon);
		DBA_RUN_OR_RETURN(dba_file_create(type, "(stdout)", "w", &d.file));

		DBA_RUN_OR_RETURN(dba_db_export(db, query, msg_writer, &d));
		dba_file_delete(d.file);
	}

	dba_db_delete(db);

	dba_record_delete(query);

	return dba_error_ok();
}

dba_err do_delete(poptContext optCon)
{
	dba_record query;
	dba_db db;

	/* Throw away the command name */
	poptGetArg(optCon);

	if (poptPeekArg(optCon) == NULL)
		dba_cmdline_error(optCon, "you need to specify some query parameters");

	/* Connect to the database */
	DBA_RUN_OR_RETURN(create_dba_db(&db));

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));

	/* Add the query data from commandline */
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	// TODO: check that there is something

	DBA_RUN_OR_RETURN(dba_db_remove(db, query));

	dba_db_delete(db);
	dba_record_delete(query);

	return dba_error_ok();
}

static struct tool_desc dbadb;

struct poptOption dbadb_dump_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_wipe_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_import_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "overwrite", 'f', POPT_ARG_NONE, &op_overwrite, 0,
		"overwrite existing data" },
	{ "report", 'r', POPT_ARG_STRING, &op_report, 0,
		"force data to be of this type of report, specified with rep_cod or rep_memo values", "rep" },
	{ "fast", 0, POPT_ARG_NONE, &op_fast, 0,
		"Ignored.  This option is left here for compatibility with old versions of dbadb." },
	{ "no-attrs", 0, POPT_ARG_NONE, &op_no_attrs, 0,
		"do not import data attributes" },
	{ "full-pseudoana", 0, POPT_ARG_NONE, &op_full_pseudoana, 0,
		"merge pseudoana extra values with the ones already existing in the database" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbadb_export_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "report", 'r', POPT_ARG_STRING, &op_report, 0,
		"force exported data to be of this type of report, specified with rep_cod or rep_memo values", "rep" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ "template", 't', POPT_ARG_STRING, &op_output_template, 0,
		"template of the data in output (autoselect if not specified)", "type.sub.local" },
	{ "dump", 0, POPT_ARG_NONE, &op_dump, 0,
		"dump data to be encoded instead of encoding it" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_repinfo_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_cleanup_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_stations_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
		"Options used to connect to the database" },
	POPT_TABLEEND
};

struct poptOption dbadb_delete_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
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
	dbadb.ops = (struct op_dispatch_table*)calloc(9, sizeof(struct op_dispatch_table));

	dbadb.ops[0].func = do_dump;
	dbadb.ops[0].aliases[0] = "dump";
	dbadb.ops[0].usage = "dump [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbadb.ops[0].desc = "Dump data from the database";
	dbadb.ops[0].longdesc = "Query parameters are the same of the Fortran API. "
		"Please see the section \"Input and output parameters -- For data "
		"related action routines\" of the Fortran API documentation for a "
		"complete list.";
	dbadb.ops[0].optable = dbadb_dump_options;

	dbadb.ops[1].func = do_wipe;
	dbadb.ops[1].aliases[0] = "wipe";
	dbadb.ops[1].usage = "wipe [options] [optional rep_memo description file]";
	dbadb.ops[1].desc = "Reinitialise the database, removing all data";
	dbadb.ops[1].longdesc =
			"Reinitialisation is done using the given report code description file. "
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
	dbadb.ops[3].usage = "export [options] rep_memo [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbadb.ops[3].desc = "Export data from the database";
	dbadb.ops[3].longdesc = "Query parameters are the same of the Fortran API. "
		"Please see the section \"Input and output parameters -- For data "
		"related action routines\" of the Fortran API documentation for a "
		"complete list.";
	dbadb.ops[3].optable = dbadb_export_options;

	dbadb.ops[4].func = do_repinfo;
	dbadb.ops[4].aliases[0] = "repinfo";
	dbadb.ops[4].usage = "repinfo [options] [filename]";
	dbadb.ops[4].desc = "Update the report information table";
	dbadb.ops[4].longdesc =
			"Update the report information table with the data from the given "
			"report code description file.  "
			"If no file is provided, a default version is used";
	dbadb.ops[4].optable = dbadb_repinfo_options;

	dbadb.ops[5].func = do_cleanup;
	dbadb.ops[5].aliases[0] = "cleanup";
	dbadb.ops[5].usage = "cleanup [options]";
	dbadb.ops[5].desc = "Perform database cleanup operations";
	dbadb.ops[5].longdesc =
			"The only operation currently performed by this command is "
			"deleting stations that have no values.  If more will be added in "
			"the future, they will be documented here.";
	dbadb.ops[5].optable = dbadb_cleanup_options;

	dbadb.ops[6].func = do_stations;
	dbadb.ops[6].aliases[0] = "stations";
	dbadb.ops[6].usage = "stations [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbadb.ops[6].desc = "List the stations present in the database";
	dbadb.ops[6].longdesc = "Query parameters are the same of the Fortran API. "
		"Please see the section \"Input and output parameters -- For data "
		"related action routines\" of the Fortran API documentation for a "
		"complete list.";
	dbadb.ops[6].optable = dbadb_stations_options;

	dbadb.ops[7].func = do_delete;
	dbadb.ops[7].aliases[0] = "delete";
	dbadb.ops[7].usage = "delete [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
	dbadb.ops[7].desc = "Delete all the data matching the given query parameters";
	dbadb.ops[7].longdesc = "Query parameters are the same of the Fortran API. "
		"Please see the section \"Input and output parameters -- For data "
		"related action routines\" of the Fortran API documentation for a "
		"complete list.";
	dbadb.ops[7].optable = dbadb_delete_options;

	dbadb.ops[8].func = NULL;
	dbadb.ops[8].usage = NULL;
	dbadb.ops[8].desc = NULL;
	dbadb.ops[8].longdesc = NULL;
	dbadb.ops[8].optable = NULL;
};

static struct program_info proginfo = {
	"dbadb",
	NULL,
	NULL,
	NULL
};

int main (int argc, const char* argv[])
{
	int res;
	dba_init();
	init();
	res = dba_cmdline_dispatch_main(&proginfo, &dbadb, argc, argv);
	dba_shutdown();
	return res;
}

/* vim:set ts=4 sw=4: */
