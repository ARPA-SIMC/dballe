/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

static unique_ptr<DB> connect()
{
    unique_ptr<DB> db;
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
        db = DB::connect_from_url(chosen_dsn);
    else
        db = DB::connect(chosen_dsn, op_user, op_pass);

    // Wipe database if requested
    if (op_wipe_first)
        db->reset();

    return db;
}


// Command line parsing wrappers for Dbadb methods

struct DatabaseCmd : public cmdline::Subcommand
{
    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &dbTable, 0,
            "Options used to connect to the database", 0 });
    }
};

struct DumpCmd : public DatabaseCmd
{
    DumpCmd()
    {
        names.push_back("dump");
        usage = "dump [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
        desc = "Dump data from the database";
        longdesc = "Query parameters are the same of the Fortran API. "
            "Please see the section \"Input and output parameters -- For data "
            "related action routines\" of the Fortran API documentation for a "
            "complete list.";
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        /* Create the query */
        Query query;
        dba_cmdline_get_query(optCon, query);

        unique_ptr<DB> db = connect();
        Dbadb dbadb(*db);

        return dbadb.do_dump(query, stdout);
    }
};

struct StationsCmd : public DatabaseCmd
{
    StationsCmd()
    {
        names.push_back("stations");
        usage = "stations [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
        desc = "List the stations present in the database";
        longdesc = "Query parameters are the same of the Fortran API. "
            "Please see the section \"Input and output parameters -- For data "
            "related action routines\" of the Fortran API documentation for a "
            "complete list.";
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        /* Create the query */
        Query query;
        dba_cmdline_get_query(optCon, query);

        unique_ptr<DB> db = connect();
        Dbadb dbadb(*db);

        return dbadb.do_stations(query, stdout);
    }
};

/// Create / empty the database
struct WipeCmd : public DatabaseCmd
{
    WipeCmd()
    {
        names.push_back("wipe");
        usage = "wipe [options] [optional rep_memo description file]";
        desc = "Reinitialise the database, removing all data";
        longdesc =
            "Reinitialisation is done using the given report code description file. "
            "If no file is provided, a default version is used";
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        /* Get the optional name of the repinfo file */
        const char* fname = poptGetArg(optCon);

        {
            // Connect first using the current format, and remove all tables.
            unique_ptr<DB> db = connect();
            db->disappear();
        }

        // Recreate tables
        unique_ptr<DB> db = connect();
        db->reset(fname);
        return 0;
    }
};

/// Perform database cleanup maintenance
struct CleanupCmd : public DatabaseCmd
{
    CleanupCmd()
    {
        names.push_back("cleanup");
        usage = "cleanup [options]";
        desc = "Perform database cleanup operations";
        longdesc =
            "The only operation currently performed by this command is "
            "deleting stations that have no values.  If more will be added in "
            "the future, they will be documented here.";
    }

    int main(poptContext optCon)
    {
        unique_ptr<DB> db = connect();
        db->vacuum();
        return 0;
    }
};

/// Update repinfo information in the database
struct RepinfoCmd : public DatabaseCmd
{
    RepinfoCmd()
    {
        names.push_back("repinfo");
        usage = "repinfo [options] [filename]";
        desc = "Update the report information table";
        longdesc =
            "Update the report information table with the data from the given "
            "report code description file.  "
            "If no file is provided, a default version is used";
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        unique_ptr<DB> db = connect();

        /* Get the optional name of the repinfo file.  If missing, the default will be used */
        const char* fname = poptGetArg(optCon);

        int added, deleted, updated;
        db->update_repinfo(fname, &added, &deleted, &updated);
        printf("Update completed: %d added, %d deleted, %d updated.\n", added, deleted, updated);
        return 0;
    }
};

struct ImportCmd : public DatabaseCmd
{
    ImportCmd()
    {
        names.push_back("import");
        usage = "import [options] [filter] filename [filename [ ... ] ]";
        desc = "Import data into the database";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        DatabaseCmd::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &reader.input_type, 0,
            "format of the input data ('bufr', 'crex', 'aof', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &reader.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ "overwrite", 'f', POPT_ARG_NONE, &op_overwrite, 0,
            "overwrite existing data", 0 });
        opts.push_back({ "report", 'r', POPT_ARG_STRING, &op_report, 0,
            "force data to be of this type of report", "rep" });
        opts.push_back({ "fast", 0, POPT_ARG_NONE, &op_fast, 0,
            "Prefer speed to transactional integrity: if the import is interrupted,"
            " the database needs to be wiped and recreated.", 0 });
        opts.push_back({ "no-attrs", 0, POPT_ARG_NONE, &op_no_attrs, 0,
            "do not import data attributes", 0 });
        opts.push_back({ "full-pseudoana", 0, POPT_ARG_NONE, &op_full_pseudoana, 0,
            "merge pseudoana extra values with the ones already existing in the database", 0 });
        opts.push_back({ "precise", 0, 0, &op_precise_import, 0,
            "import messages using precise contexts instead of standard ones", 0 });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        // Throw away the command name
        poptGetArg(optCon);

        // Configure the reader
        Query query;
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

        unique_ptr<DB> db = connect();

        const char* forced_repmemo = dbadb::parse_op_report(*db, op_report);

        Dbadb dbadb(*db);
        return dbadb.do_import(get_filenames(optCon), reader, import_flags, forced_repmemo);
    }
};

struct ExportCmd : public DatabaseCmd
{
    ExportCmd()
    {
        names.push_back("export");
        usage = "export [options] rep_memo [queryparm1=val1 [queryparm2=val2 [...]]]";
        desc = "Export data from the database";
        longdesc = "Query parameters are the same of the Fortran API. "
            "Please see the section \"Input and output parameters -- For data "
            "related action routines\" of the Fortran API documentation for a "
            "complete list.";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        DatabaseCmd::add_to_optable(opts);
        opts.push_back({ "report", 'r', POPT_ARG_STRING, &op_report, 0,
            "force exported data to be of this type of report", "rep" });
        opts.push_back({ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
            "format of the data in output ('bufr', 'crex', 'aof')", "type" });
        opts.push_back({ "template", 't', POPT_ARG_STRING, &op_output_template, 0,
            "template of the data in output (autoselect if not specified, 'list' gives a list)", "name" });
        opts.push_back({ "dump", 0, POPT_ARG_NONE, &op_dump, 0,
            "dump data to be encoded instead of encoding it", 0 });
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        if (strcmp(op_output_template, "list") == 0)
        {
            list_templates();
            return 0;
        }

        // Reat the query from command line
        Query query;
        dba_cmdline_get_query(optCon, query);

        unique_ptr<DB> db = connect();
        Dbadb dbadb(*db);

        if (op_dump)
        {
            return dbadb.do_export_dump(query, stdout);
        } else {
            Encoding type = dba_cmdline_stringToMsgType(op_output_type);
            unique_ptr<File> file = File::create(type, "(stdout)", "w");
            return dbadb.do_export(query, *file, op_output_template, op_report);
        }
    }
};

struct DeleteCmd : public DatabaseCmd
{
    DeleteCmd()
    {
        names.push_back("delete");
        usage = "delete [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
        desc = "Delete all the data matching the given query parameters";
        longdesc = "Query parameters are the same of the Fortran API. "
            "Please see the section \"Input and output parameters -- For data "
            "related action routines\" of the Fortran API documentation for a "
            "complete list.";
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        if (poptPeekArg(optCon) == NULL)
            dba_cmdline_error(optCon, "you need to specify some query parameters");

        /* Add the query data from commandline */
        Query query;
        dba_cmdline_get_query(optCon, query);

        unique_ptr<DB> db = connect();
        // TODO: check that there is something
        db->remove(query);
        return 0;
    }
};


int main (int argc, const char* argv[])
{
    Command dbadb;
    dbadb.name = "dbadb";
    dbadb.desc = "Manage the DB-ALLe database";
    dbadb.longdesc =
        "It allows to initialise the database, dump its contents and import and export data "
        "using BUFR, CREX or AOF encoding";

    dbadb.add_subcommand(new DumpCmd);
    dbadb.add_subcommand(new StationsCmd);
    dbadb.add_subcommand(new WipeCmd);
    dbadb.add_subcommand(new CleanupCmd);
    dbadb.add_subcommand(new RepinfoCmd);
    dbadb.add_subcommand(new ImportCmd);
    dbadb.add_subcommand(new ExportCmd);
    dbadb.add_subcommand(new DeleteCmd);

    return dbadb.main(argc, argv);
}

/* vim:set ts=4 sw=4: */
