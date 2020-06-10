#include <dballe/cmdline/dbadb.h>
#include <dballe/cmdline/processor.h>
#include <dballe/cmdline/cmdline.h>
#include <dballe/file.h>
#include <dballe/message.h>
#include <dballe/msg/msg.h>
#include <dballe/db/db.h>
#include <wreport/error.h>
#include <wreport/utils/string.h>

#include <cstdlib>
#include <cstring>

using namespace dballe;
using namespace dballe::cmdline;
using namespace wreport;
using namespace std;

// Command line parser variables
struct cmdline::ReaderOptions readeropts;
static const char* op_output_template = "";
const char* op_output_type = "bufr";
const char* op_report = "";
const char* op_url = "";
const char* op_user = "";
const char* op_pass = "";
const char* op_varlist = "";
int op_wipe_first = 0;
int op_dump = 0;
int op_overwrite = 0;
int op_fast = 0;
int op_no_attrs = 0;
int op_full_pseudoana = 0;
int op_verbose = 0;
int op_precise_import = 0;
int op_wipe_disappear = 0;


struct poptOption grepTable[] = {
    { "category", 0, POPT_ARG_INT, &readeropts.category, 0,
        "match messages with the given data category", "num" },
    { "subcategory", 0, POPT_ARG_INT, &readeropts.subcategory, 0,
        "match BUFR messages with the given data subcategory", "num" },
    { "check-digit", 0, POPT_ARG_INT, &readeropts.checkdigit, 0,
        "match CREX messages with check digit (if 1) or without check digit (if 0)", "num" },
    { "parsable", 0, 0, &readeropts.parsable, 0,
        "match only messages that can be parsed", 0 },
    { "index", 0, POPT_ARG_STRING, &readeropts.index_filter, 0,
        "match messages with the index in the given range (ex.: 1-5,9,22-30)", "expr" },
    POPT_TABLEEND
};

struct poptOption dbTable[] = {
    { "dsn", 0, POPT_ARG_STRING, &op_url, 0,
        "alias of --url, used for historical compatibility", "url" },
    { "url", 0, POPT_ARG_STRING, &op_url, 0,
        "DSN, or URL-like database definition, to use for connecting to the DB-All.e database (can also be specified in the environment as DBA_DB)", "url" },
    { "wipe-first", 0, POPT_ARG_NONE, &op_wipe_first, 0,
        "wipe database before any other action", 0 },
    POPT_TABLEEND
};

static std::shared_ptr<db::DB> connect()
{
    const char* chosen_url;

    /* If url is missing, look in the environment */
    if (op_url[0] == 0)
    {
        chosen_url = getenv("DBA_DB");
        if (chosen_url == NULL)
            throw error_consistency("no database specified");
    } else
        chosen_url = op_url;

    /* If url looks like a url, treat it accordingly */
    auto options = DBConnectOptions::create(chosen_url);
    auto db = dynamic_pointer_cast<db::DB>(DB::connect(*options));

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
        core::Query query;
        dba_cmdline_get_query(optCon, query);

        auto db = connect();
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
        core::Query query;
        dba_cmdline_get_query(optCon, query);

        auto db = connect();
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

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        DatabaseCmd::add_to_optable(opts);
        opts.push_back({ "disappear", 0, POPT_ARG_NONE, &op_wipe_disappear, 0,
            "just remove the DB-All.e data and tables", 0 });
    }


    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        /* Get the optional name of the repinfo file */
        const char* fname = poptGetArg(optCon);

        {
            // Connect first using the current format, and remove all tables.
            auto db = connect();
            db->disappear();
        }

        if (!op_wipe_disappear)
        {
            // Recreate tables
            auto db = connect();
            db->reset(fname);
        }
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
        auto db = connect();
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

        auto db = connect();

        /* Get the optional name of the repinfo file.  If missing, the default will be used */
        const char* fname = poptGetArg(optCon);

        auto tr = dynamic_pointer_cast<db::Transaction>(db->transaction());
        int added, deleted, updated;
        tr->update_repinfo(fname, &added, &deleted, &updated);
        tr->commit();

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
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'csv', 'json')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
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
        opts.push_back({ "varlist", 0, POPT_ARG_STRING, &op_varlist, 0,
            "only import variables with the given varcode(s)", "varlist" });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        // Throw away the command name
        poptGetArg(optCon);
        cmdline::Reader reader(readeropts);
        reader.verbose = op_verbose;

        // Configure the reader
        core::Query query;
        if (dba_cmdline_get_query(optCon, query) > 0)
            reader.filter.matcher_from_record(query);
        reader.import_opts.simplified = !op_precise_import;

        // Configure the importer
        auto opts = DBImportOptions::create();
        if (op_overwrite)
            opts->overwrite = true;
        if (op_fast)
            setenv("DBA_INSECURE_SQLITE", "true", true);
        if (!op_no_attrs)
            opts->import_attributes = true;
        if (op_full_pseudoana)
            opts->update_station = true;
        if (op_varlist[0])
            resolve_varlist(op_varlist, [&](wreport::Varcode code) { opts->varlist.push_back(code); });

        auto db = connect();

        if (strcmp(op_report, "") != 0)
            opts->report = op_report;

        Dbadb dbadb(*db);
        return dbadb.do_import(get_filenames(optCon), reader, *opts);
    }
};

struct ExportCmd : public DatabaseCmd
{
    ExportCmd()
    {
        names.push_back("export");
        usage = "export [options] [queryparm1=val1 [queryparm2=val2 [...]]]";
        desc = "Export data from the database";
        longdesc = "Query parameters are the same of the Fortran API. "
            "Please see the section \"Input and output parameters -- For data "
            "related action routines\" of the Fortran API documentation for a "
            "complete list.\n\n"
            "The database is specified with --url or with the DBA_DB"
            " environment variable";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        DatabaseCmd::add_to_optable(opts);
        opts.push_back({ "report", 'r', POPT_ARG_STRING, &op_report, 0,
            "force exported data to be of this type of report", "rep" });
        opts.push_back({ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
            "format of the data in output ('bufr', 'crex')", "type" });
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
        core::Query query;
        dba_cmdline_get_query(optCon, query);

        auto db = connect();
        Dbadb dbadb(*db);

        const char* forced_repmemo = NULL;
        if (strcmp(op_report, "") != 0)
            forced_repmemo = op_report;

        if (op_dump)
        {
            return dbadb.do_export_dump(query, stdout);
        } else {
            Encoding type = File::parse_encoding(op_output_type);
            auto file = File::create(type, stdout, false, "w");
            return dbadb.do_export(query, *file, op_output_template, forced_repmemo);
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
        core::Query query;
        dba_cmdline_get_query(optCon, query);

        auto db = connect();
        // TODO: check that there is something
        db->remove_data(query);
        return 0;
    }
};

struct InfoCmd : public DatabaseCmd
{
    InfoCmd()
    {
        names.push_back("info");
        usage = "info";
        desc = "Print information about the database";
    }

    int main(poptContext optCon) override
    {
        // Throw away the command name
        poptGetArg(optCon);

        auto db = connect();

        string default_format = db::format_format(db::DB::get_default_format());
        fprintf(stdout, "Default format for new DBs: %s\n", default_format.c_str());
        db->print_info(stdout);

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
        "using BUFR, or CREX encoding";

    dbadb.add_subcommand(new DumpCmd);
    dbadb.add_subcommand(new StationsCmd);
    dbadb.add_subcommand(new WipeCmd);
    dbadb.add_subcommand(new CleanupCmd);
    dbadb.add_subcommand(new RepinfoCmd);
    dbadb.add_subcommand(new ImportCmd);
    dbadb.add_subcommand(new ExportCmd);
    dbadb.add_subcommand(new DeleteCmd);
    dbadb.add_subcommand(new InfoCmd);

    return dbadb.main(argc, argv);
}

/* vim:set ts=4 sw=4: */
