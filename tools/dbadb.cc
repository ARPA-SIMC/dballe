/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/cmdline/dbadb.h>
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

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace std;

// Command line parser variables
struct cmdline::Reader reader;
static const char* op_output_template = "";
const char* op_output_type = "bufr";
const char* op_report = "";
const char* op_dsn = "";
const char* op_user = "";
const char* op_pass = "";
int op_wipe_first = 0;
int op_dump = 0;
int op_overwrite = 0;
int op_fast = 0;
int op_no_attrs = 0;
int op_full_pseudoana = 0;
int op_verbose = 0;
int op_precise_import = 0;

struct poptOption grepTable[] = {
    { "category", 0, POPT_ARG_INT, &reader.filter.category, 0,
        "match messages with the given data category", "num" },
    { "subcategory", 0, POPT_ARG_INT, &reader.filter.subcategory, 0,
        "match BUFR messages with the given data subcategory", "num" },
    { "check-digit", 0, POPT_ARG_INT, &reader.filter.checkdigit, 0,
        "match CREX messages with check digit (if 1) or without check digit (if 0)", "num" },
    { "unparsable", 0, 0, &reader.filter.unparsable, 0,
        "match only messages that cannot be parsed", 0 },
    { "parsable", 0, 0, &reader.filter.parsable, 0,
        "match only messages that can be parsed", 0 },
    { "index", 0, POPT_ARG_STRING, &reader.filter.index, 0,
        "match messages with the index in the given range (ex.: 1-5,9,22-30)", "expr" },
    POPT_TABLEEND
};

struct poptOption dbTable[] = {
    { "dsn", 0, POPT_ARG_STRING, &op_dsn, 0,
        "DSN, or URL-like database definition, to use for connecting to the DB-All.e database (can also be specified in the environment as DBA_DB)", "dsn" },
    { "user", 0, POPT_ARG_STRING, &op_user, 0,
        "username to use for connecting to the DB-All.e database", "user" },
    { "pass", 0, POPT_ARG_STRING, &op_pass, 0,
        "password to use for connecting to the DB-All.e database", "pass" },
    { "wipe-first", 0, POPT_ARG_NONE, &op_wipe_first, 0,
        "wipe database before any other action" },
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
        db.connect_from_url(chosen_dsn);
    else
        db.connect(chosen_dsn, op_user, op_pass);

    // Wipe database if requested
    if (op_wipe_first)
        db.reset();
}


// Command line parsing wrappers for Dbadb methods

int do_dump(poptContext optCon)
{
    /* Throw away the command name */
    poptGetArg(optCon);

    /* Create the query */
    Record query;
    dba_cmdline_get_query(optCon, query);

    DB db;
    connect(db);
    Dbadb dbadb(db);

    return dbadb.do_dump(query, stdout);
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
    Dbadb dbadb(db);

    return dbadb.do_stations(query, stdout);
}

/// Create / empty the database
int do_wipe(poptContext optCon)
{
    /* Throw away the command name */
    poptGetArg(optCon);

    /* Get the optional name of the repinfo file */
    const char* fname = poptGetArg(optCon);

    DB db;
    connect(db);

    db.reset(fname);
    return 0;
}

/// Perform database cleanup maintenance
int do_cleanup(poptContext optCon)
{
    DB db;
    connect(db);
    db.remove_orphans();
    return 0;
}

/// Update repinfo information in the database
int do_repinfo(poptContext optCon)
{
    /* Throw away the command name */
    poptGetArg(optCon);

    DB db;
    connect(db);

    /* Get the optional name of the repinfo file.  If missing, the default will be used */
    const char* fname = poptGetArg(optCon);

    int added, deleted, updated;
    db.update_repinfo(fname, &added, &deleted, &updated);
    printf("Update completed: %d added, %d deleted, %d updated.\n", added, deleted, updated);
    return 0;
}


int do_import(poptContext optCon)
{
    // Throw away the command name
    poptGetArg(optCon);

    // Configure the reader
    Record query;
    if (dba_cmdline_get_query(optCon, query) > 0)
        reader.filter.matcher_from_record(query);
    reader.import_opts.simplified = !op_precise_import;

    // Configure the importer
    int import_flags = 0;
    if (op_overwrite)
        import_flags |= DBA_IMPORT_OVERWRITE;
    if (op_fast)
        setenv("DBA_INSECURE_SQLITE", "true", true);
    if (!op_no_attrs)
        import_flags |= DBA_IMPORT_ATTRS;
    if (op_full_pseudoana)
        import_flags |= DBA_IMPORT_FULL_PSEUDOANA;

    DB db;
    connect(db);

    const char* forced_repmemo = dbadb::parse_op_report(db, op_report);

    Dbadb dbadb(db);
    return dbadb.do_import(get_filenames(optCon), reader, import_flags, forced_repmemo);
}

int do_export(poptContext optCon)
{
    /* Throw away the command name */
    poptGetArg(optCon);

    if (strcmp(op_output_template, "list") == 0)
    {
        list_templates();
        return 0;
    }

    // Reat the query from command line
    Record query;
    dba_cmdline_get_query(optCon, query);

    DB db;
    connect(db);
    Dbadb dbadb(db);

    if (op_dump)
    {
        return dbadb.do_export_dump(query, stdout);
    } else {
        Encoding type = dba_cmdline_stringToMsgType(op_output_type);
        auto_ptr<File> file = File::create(type, "(stdout)", "w");
        return dbadb.do_export(query, *file, op_output_template, op_report);
    }
}

int do_delete(poptContext optCon)
{
    /* Throw away the command name */
    poptGetArg(optCon);

    if (poptPeekArg(optCon) == NULL)
        dba_cmdline_error(optCon, "you need to specify some query parameters");

    /* Add the query data from commandline */
    Record query;
    dba_cmdline_get_query(optCon, query);

    DB db;
    connect(db);

    // TODO: check that there is something

    db.remove(query);
    return 0;
}


struct poptOption dbadb_dump_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_wipe_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_import_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output", 0 },
    { "type", 't', POPT_ARG_STRING, &reader.input_type, 0,
        "format of the input data ('bufr', 'crex', 'aof', 'csv')", "type" },
    { "overwrite", 'f', POPT_ARG_NONE, &op_overwrite, 0,
        "overwrite existing data", 0 },
    { "report", 'r', POPT_ARG_STRING, &op_report, 0,
        "force data to be of this type of report, specified with rep_cod or rep_memo values", "rep" },
    { "fast", 0, POPT_ARG_NONE, &op_fast, 0,
        "Prefer speed to transactional integrity: if the import is interrupted,"
        " the database needs to be wiped and recreated.", 0 },
    { "no-attrs", 0, POPT_ARG_NONE, &op_no_attrs, 0,
        "do not import data attributes", 0 },
    { "full-pseudoana", 0, POPT_ARG_NONE, &op_full_pseudoana, 0,
        "merge pseudoana extra values with the ones already existing in the database", 0 },
    { "precise", 0, 0, &op_precise_import, 0,
        "import messages using precise contexts instead of standard ones", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
        "Options used to filter messages", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_export_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output", 0 },
    { "report", 'r', POPT_ARG_STRING, &op_report, 0,
        "force exported data to be of this type of report, specified with rep_cod or rep_memo values", "rep" },
    { "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
        "format of the data in output ('bufr', 'crex', 'aof')", "type" },
    { "template", 't', POPT_ARG_STRING, &op_output_template, 0,
        "template of the data in output (autoselect if not specified, 'list' gives a list)", "name" },
    { "dump", 0, POPT_ARG_NONE, &op_dump, 0,
        "dump data to be encoded instead of encoding it", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_repinfo_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_cleanup_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_stations_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

struct poptOption dbadb_delete_options[] = {
    { "help", '?', 0, 0, 1, "print an help message", 0 },
    { NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
        "Options used to connect to the database", 0 },
    POPT_TABLEEND
};

static struct tool_desc dbadb_tooldesc;

static void init()
{
    dbadb_tooldesc.desc = "Manage the DB-ALLe database";
    dbadb_tooldesc.longdesc =
        "It allows to initialise the database, dump its contents and import and export data "
        "using BUFR, CREX or AOF encoding";
    dbadb_tooldesc.ops = (struct op_dispatch_table*)calloc(9, sizeof(struct op_dispatch_table));

    dbadb_tooldesc.ops[0].func = do_dump;
    dbadb_tooldesc.ops[0].aliases[0] = "dump";
    dbadb_tooldesc.ops[0].usage = "dump [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
    dbadb_tooldesc.ops[0].desc = "Dump data from the database";
    dbadb_tooldesc.ops[0].longdesc = "Query parameters are the same of the Fortran API. "
        "Please see the section \"Input and output parameters -- For data "
        "related action routines\" of the Fortran API documentation for a "
        "complete list.";
    dbadb_tooldesc.ops[0].optable = dbadb_dump_options;

    dbadb_tooldesc.ops[1].func = do_wipe;
    dbadb_tooldesc.ops[1].aliases[0] = "wipe";
    dbadb_tooldesc.ops[1].usage = "wipe [options] [optional rep_memo description file]";
    dbadb_tooldesc.ops[1].desc = "Reinitialise the database, removing all data";
    dbadb_tooldesc.ops[1].longdesc =
            "Reinitialisation is done using the given report code description file. "
            "If no file is provided, a default version is used";
    dbadb_tooldesc.ops[1].optable = dbadb_wipe_options;

    dbadb_tooldesc.ops[2].func = do_import;
    dbadb_tooldesc.ops[2].aliases[0] = "import";
    dbadb_tooldesc.ops[2].usage = "import [options] [filter] filename [filename [ ... ] ]";
    dbadb_tooldesc.ops[2].desc = "Import data into the database";
    dbadb_tooldesc.ops[2].longdesc = NULL;
    dbadb_tooldesc.ops[2].optable = dbadb_import_options;

    dbadb_tooldesc.ops[3].func = do_export;
    dbadb_tooldesc.ops[3].aliases[0] = "export";
    dbadb_tooldesc.ops[3].usage = "export [options] rep_memo [queryparm1=val1 [queryparm2=val2 [...]]]";
    dbadb_tooldesc.ops[3].desc = "Export data from the database";
    dbadb_tooldesc.ops[3].longdesc = "Query parameters are the same of the Fortran API. "
        "Please see the section \"Input and output parameters -- For data "
        "related action routines\" of the Fortran API documentation for a "
        "complete list.";
    dbadb_tooldesc.ops[3].optable = dbadb_export_options;

    dbadb_tooldesc.ops[4].func = do_repinfo;
    dbadb_tooldesc.ops[4].aliases[0] = "repinfo";
    dbadb_tooldesc.ops[4].usage = "repinfo [options] [filename]";
    dbadb_tooldesc.ops[4].desc = "Update the report information table";
    dbadb_tooldesc.ops[4].longdesc =
            "Update the report information table with the data from the given "
            "report code description file.  "
            "If no file is provided, a default version is used";
    dbadb_tooldesc.ops[4].optable = dbadb_repinfo_options;

    dbadb_tooldesc.ops[5].func = do_cleanup;
    dbadb_tooldesc.ops[5].aliases[0] = "cleanup";
    dbadb_tooldesc.ops[5].usage = "cleanup [options]";
    dbadb_tooldesc.ops[5].desc = "Perform database cleanup operations";
    dbadb_tooldesc.ops[5].longdesc =
            "The only operation currently performed by this command is "
            "deleting stations that have no values.  If more will be added in "
            "the future, they will be documented here.";
    dbadb_tooldesc.ops[5].optable = dbadb_cleanup_options;

    dbadb_tooldesc.ops[6].func = do_stations;
    dbadb_tooldesc.ops[6].aliases[0] = "stations";
    dbadb_tooldesc.ops[6].usage = "stations [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
    dbadb_tooldesc.ops[6].desc = "List the stations present in the database";
    dbadb_tooldesc.ops[6].longdesc = "Query parameters are the same of the Fortran API. "
        "Please see the section \"Input and output parameters -- For data "
        "related action routines\" of the Fortran API documentation for a "
        "complete list.";
    dbadb_tooldesc.ops[6].optable = dbadb_stations_options;

    dbadb_tooldesc.ops[7].func = do_delete;
    dbadb_tooldesc.ops[7].aliases[0] = "delete";
    dbadb_tooldesc.ops[7].usage = "delete [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
    dbadb_tooldesc.ops[7].desc = "Delete all the data matching the given query parameters";
    dbadb_tooldesc.ops[7].longdesc = "Query parameters are the same of the Fortran API. "
        "Please see the section \"Input and output parameters -- For data "
        "related action routines\" of the Fortran API documentation for a "
        "complete list.";
    dbadb_tooldesc.ops[7].optable = dbadb_delete_options;

    dbadb_tooldesc.ops[8].func = NULL;
    dbadb_tooldesc.ops[8].usage = NULL;
    dbadb_tooldesc.ops[8].desc = NULL;
    dbadb_tooldesc.ops[8].longdesc = NULL;
    dbadb_tooldesc.ops[8].optable = NULL;
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
    res = dba_cmdline_dispatch_main(&proginfo, &dbadb_tooldesc, argc, argv);
    return res;
}

/* vim:set ts=4 sw=4: */
