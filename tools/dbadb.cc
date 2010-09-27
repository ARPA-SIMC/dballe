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

#include <dballe/cmdline/processor.h>
#include <dballe/cmdline/cmdline.h>
#include <dballe/core/record.h>
#include <dballe/core/file.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>
#include <dballe/db/db.h>
#include <dballe/db/cursor.h>
#include <wreport/error.h>

#include <cstdlib>
#include <cstring>

#if 0
#include <dballe/init.h>
#include <dballe/bufrex/msg.h>
#include <dballe/msg/file.h>
#include <dballe/msg/bufrex_codec.h>

#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace std;

struct cmdline::grep_t grepdata = { -1, -1, -1, 0, 0, "" };
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

static const char* op_dsn = "";
static const char* op_user = "";
static const char* op_pass = "";
static const char* op_input_type = "auto";
static const char* op_report = "";
static const char* op_output_type = "bufr";
static const char* op_output_template = "";
static int op_overwrite = 0;
static int op_fast = 0;
static int op_no_attrs = 0;
static int op_full_pseudoana = 0;
static int op_dump = 0;
int op_verbose = 0;

struct poptOption dbTable[] = {
	{ "dsn", 0, POPT_ARG_STRING, &op_dsn, 0,
		"DSN, or URL-like database definition, to use for connecting to the DB-All.e database (can also be specified in the environment as DBA_DB)" },
	{ "user", 0, POPT_ARG_STRING, &op_user, 0,
		"username to use for connecting to the DB-All.e database" },
	{ "pass", 0, POPT_ARG_STRING, &op_pass, 0,
		"password to use for connecting to the DB-All.e database" },
	POPT_TABLEEND
};

static void connect(DB& db)
{
	const char* chosen_dsn;

	/* If dsn is missing, look in the environment */
	if (op_dsn[0] == 0)
	{
		chosen_dsn = getenv("DBA_DB");
		if (chosen_dsn == NULL)
			throw error_consistency("no database specified");
	} else
		chosen_dsn = op_dsn;

	/* If dsn looks like a url, treat it accordingly */
	if (DB::is_url(chosen_dsn))
		return db.connect_from_url(chosen_dsn);
	else
		return db.connect(chosen_dsn, op_user, op_pass);
}

struct Importer : public cmdline::Action
{
	DB& db;
	bool overwrite;
	const char* forced_repmemo;

	Importer(DB& db) : db(db), overwrite(false), forced_repmemo(0) {}

	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		int import_flags = 0;;
		if (msgs == NULL)
		{
			fprintf(stderr, "Message #%d cannot be parsed: ignored\n", rmsg.index);
			return;
		}
		if (overwrite)
			import_flags |= DBA_IMPORT_OVERWRITE;
		if (op_fast)
			import_flags |= DBA_IMPORT_NO_TRANSACTIONS;
		if (!op_no_attrs)
			import_flags |= DBA_IMPORT_ATTRS;
		if (op_full_pseudoana)
			import_flags |= DBA_IMPORT_FULL_PSEUDOANA;

		for (size_t i = 0; i < msgs->size(); ++i)
		{
			Msg& msg = *(*msgs)[i];
			if (forced_repmemo == NULL && msg.type == MSG_GENERIC)
				/* Put generic messages in the generic rep_cod by default */
				db.import_msg(msg, NULL, import_flags);
			else
				db.import_msg(msg, forced_repmemo, import_flags);
		}
	}
};

int do_dump(poptContext optCon)
{
	const char* action;
	DB db;

	/* Throw away the command name */
	action = poptGetArg(optCon);

	/* Create the query */
	Record query;
	dba_cmdline_get_query(optCon, query);

	connect(db);
	auto_ptr<db::Cursor> cursor = db.query_data(query);

	Record res;
	for (int i = 0; cursor->next(); ++i)
	{
		cursor->to_record(res);
		printf("#%d: -----------------------\n", i);
		res.print(stdout);
	}

	return 0;
}

int do_stations(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	/* Create the query */
	Record query;
	dba_cmdline_get_query(optCon, query);

	DB db;
	connect(db);

	auto_ptr<db::Cursor> cursor = db.query_stations(query);

	Record result;
	for (size_t i = 0; cursor->next(); ++i)
	{
		// result.clear();
		cursor->to_record(result);
		printf("#%zd: -----------------------\n", i);
		result.print(stdout);
	}

	return 0;
}

int do_wipe(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	/* Get the optional name of the repinfo file */
	const char* table = poptGetArg(optCon);

	DB db;
	connect(db);
	db.reset(table);

	return 0;
}

int do_cleanup(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	DB db;
	connect(db);
	db.remove_orphans();

	return 0;
}

int do_repinfo(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	/* Get the optional name of the repinfo file.  If missing, the default will be used */
	const char* table = poptGetArg(optCon);

	int added, deleted, updated;

	DB db;
	connect(db);
	db.update_repinfo(table, &added, &deleted, &updated);
	printf("Update completed: %d added, %d deleted, %d updated.\n", added, deleted, updated);

	return 0;
}

const char* parse_op_report(DB& db)
{
	if (op_report[0] != 0)
	{
		const char* s;
		int is_cod = 1;
		for (s = op_report; *s && is_cod; s++)
			if (!isdigit(*s))
				is_cod = 0;
		
		if (is_cod)
			return db.rep_memo_from_cod(strtoul(op_report, NULL, 0)).c_str();
		else
			return op_report;
	} else
		return NULL;
}

int do_import(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	Encoding type = dba_cmdline_stringToMsgType(op_input_type, optCon);

	DB db;
	connect(db);

	Importer importer(db);
	importer.overwrite = op_overwrite;
	importer.forced_repmemo = parse_op_report(db);

	process_all(optCon, type, &grepdata, importer);

	return 0;
}

struct MsgWriter : public MsgConsumer
{
	File* file;
	msg::Exporter* exporter;
	const char* forced_rep_memo;

	MsgWriter() : file(0), exporter(0), forced_rep_memo(0) {}
	~MsgWriter()
	{
		if (file) delete file;
		if (exporter) delete exporter;
	}

	virtual void operator()(std::auto_ptr<Msg> msg)
	{
		/* Override the message type if the user asks for it */
		if (forced_rep_memo != NULL)
			msg->type = Msg::type_from_repmemo(forced_rep_memo);
		Rawmsg raw;
		Msgs msgs;
		msgs.acquire(msg);
		exporter->to_rawmsg(msgs, raw);
		file->write(raw);
	}
};

struct MsgDumper : public MsgConsumer
{
	FILE* out;
	MsgDumper() : out(stdout) {}

	virtual void operator()(std::auto_ptr<Msg> msg)
	{
		msg->print(out);
	}
};

int do_export(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	if (strcmp(op_output_template, "list") == 0)
	{
		list_templates();
		return 0;
	}

	/* Connect to the database */
	DB db;
	connect(db);

	/* Create the query */
	Record query;

	/* Add the query data from commandline */
	dba_cmdline_get_query(optCon, query);

	if (op_dump)
	{
		MsgDumper dumper;
		db.export_msgs(query, dumper);
	} else {
		msg::Exporter::Options opts;
		Encoding type = dba_cmdline_stringToMsgType(op_output_type, optCon);
		if (op_output_template[0] != 0)
			opts.template_name = op_output_template;

		MsgWriter writer;
		writer.forced_rep_memo = parse_op_report(db);
		writer.file = File::create(type, "(stdout)", "w").release();
		writer.exporter = msg::Exporter::create(type, opts).release();

		db.export_msgs(query, writer);
	}

	return 0;
}

int do_delete(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	if (poptPeekArg(optCon) == NULL)
		dba_cmdline_error(optCon, "you need to specify some query parameters");

	DB db;
	connect(db);

	/* Add the query data from commandline */
	Record query;
	dba_cmdline_get_query(optCon, query);

	// TODO: check that there is something

	db.remove(query);

	return 0;
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
		"template of the data in output (autoselect if not specified, 'list' gives a list)", "name" },
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
	init();
	res = dba_cmdline_dispatch_main(&proginfo, &dbadb, argc, argv);
	return res;
}

/* vim:set ts=4 sw=4: */
