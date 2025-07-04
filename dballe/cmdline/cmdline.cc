#include "cmdline.h"
#include "dballe/core/query.h"
#include "dballe/msg/wr_codec.h"
#include <cctype>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <popt.h>
#include <wreport/notes.h>

using namespace std;

namespace dballe {
namespace cmdline {

static int is_tableend(struct poptOption* op)
{
    return op->longName == NULL && op->shortName == '\0' && op->argInfo == 0 &&
           op->arg == 0 && op->val == 0 && op->descrip == NULL &&
           op->argDescrip == NULL;
}

static void manpage_print_options(const char* title, struct poptOption* optable,
                                  FILE* out)
{
    int i;

    fprintf(out, ".SS %s\n", title);

    for (i = 0; !is_tableend(&(optable[i])); i++)
    {
        int bol = 1;

        if (optable[i].argInfo == POPT_ARG_INCLUDE_TABLE)
            continue;
        /*
        .TP
        .B \-d, \-\-distance
        (only valid for the \fIrelated\fP command)
        Set the maximum distance to use for the "related" option.
        (see the \fI\-\-distance\fP option in \fBtagcoll\fP(1))
        */
        fprintf(out, ".TP\n.B");
        if (optable[i].argInfo == POPT_ARG_NONE)
        {
            if (optable[i].shortName != '\0')
            {
                fprintf(out, " \\-%c", optable[i].shortName);
                bol = 0;
            }
            if (optable[i].longName != nullptr)
            {
                fprintf(out, "%s \\-\\-%s", bol ? "" : ",",
                        optable[i].longName);
                bol = 0;
            }
        }
        else
        {
            if (optable[i].shortName != '\0')
            {
                fprintf(out, " \\-%c %s", optable[i].shortName,
                        optable[i].argDescrip);
                bol = 0;
            }
            if (optable[i].longName != nullptr)
            {
                fprintf(out, "%s \\-\\-%s=%s", bol ? "" : ",",
                        optable[i].longName, optable[i].argDescrip);
                bol = 0;
            }
        }
        fprintf(out, "\n%s\n", optable[i].descrip);
    }
}

void Subcommand::add_to_optable(std::vector<poptOption>& opts) const
{
    opts.push_back({"help", '?', 0, 0, 1, "print an help message", 0});
    opts.push_back({"verbose", 0, POPT_ARG_NONE, const_cast<int*>(&op_verbose),
                    0, "verbose output", 0});
}

void Subcommand::manpage_print_options(FILE* out)
{
    string title("Option for command ");
    title += names[0];
    vector<poptOption> opts;
    add_to_optable(opts);
    opts.push_back(POPT_TABLEEND);
    cmdline::manpage_print_options(title.c_str(), opts.data(), out);
}

poptContext Subcommand::make_popt_context(int argc, const char* argv[],
                                          vector<poptOption>& opts) const
{
    add_to_optable(opts);
    opts.push_back(POPT_TABLEEND);
    poptContext optCon = poptGetContext(NULL, argc, argv, opts.data(), 0);

    // Build the help information for this entry
    string help(usage + "\n\n" + desc + ".\n\n");
    if (!longdesc.empty())
        help += longdesc + ".\n\n";
    help += "Options are:";
    poptSetOtherOptionHelp(optCon, help.c_str());

    return optCon;
}

void Subcommand::init() {}

Command::~Command()
{
    for (auto& a : ops)
        delete a;
}

void Command::add_subcommand(Subcommand* action) { ops.push_back(action); }

void Command::add_subcommand(std::unique_ptr<Subcommand>&& action)
{
    add_subcommand(action.release());
}

Subcommand* Command::find_action(const std::string& name) const
{
    for (auto& a : ops)
        for (const auto& n : a->names)
            if (name == n)
                return a;
    return nullptr;
}

void Command::usage(const std::string& selfpath, FILE* out) const
{
    // Get the executable name
    size_t pos       = selfpath.rfind('/');
    const char* self = selfpath.c_str();
    if (pos != string::npos)
        self += pos + 1;

    fprintf(out, "Usage: %s <command> [options] [arguments]\n\n%s.\n%s.\n\n",
            self, desc.c_str(), longdesc.c_str());
    fprintf(out, "Available commands are:\n");
    fprintf(out, "\t%s help\n", self);
    fprintf(out, "\t\tdisplay this help message\n");
    fprintf(out, "\t%s help manpage\n", self);
    fprintf(out, "\t\tgenerate the manpage for %s\n", self);

    for (auto& a : ops)
    {
        bool first = true;
        for (auto& name : a->names)
        {
            if (first)
            {
                fprintf(out, "\t%s %s", self, name.c_str());
                first = false;
            }
            else
                fprintf(out, " or %s", name.c_str());
        }
        fprintf(out, "\n\t\t%s\n", a->desc.c_str());
    }

    fprintf(out,
            "\nCalling `%s <command> --help' will give help on the specific "
            "command\n",
            self);
}

#if 0
void dba_cmdline_print_dba_error()
{
    dba_error_print_to_stderr();
}
#endif

void error_cmdline::throwf(const char* fmt, ...)
{
    char buf[512];
    /* Format the arguments */
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);
    throw error_cmdline(buf);
}

void dba_cmdline_error(poptContext optCon, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    fputs("Error parsing commandline: ", stderr);

    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
    fputc('\n', stderr);

    poptPrintHelp(optCon, stderr, 0);

    exit(1);
}

Encoding string_to_encoding(const char* type)
{
    if (strlen(type) >= 1)
    {
        switch (type[0])
        {
            case 'b': return Encoding::BUFR;
            case 'c': return Encoding::CREX;
        }
    }
    try
    {
        return File::parse_encoding(type);
    }
    catch (wreport::error_notfound& e)
    {
        error_cmdline::throwf("%s", e.what());
    }
}

void Command::manpage(FILE* out) const
{
    static const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun",
                                   "jul", "aug", "sep", "oct", "nov", "dec"};
    const char* self            = name.c_str();
    char* uself;
    time_t curtime     = time(NULL);
    struct tm* loctime = localtime(&curtime);
    int i;

    /* Remove libtool cruft from program name if present */
    if (strncmp(self, "lt-", 3) == 0)
        self += 3;
    uself = strdup(self);
    for (i = 0; uself[i] != '\0'; i++)
        uself[i] = toupper(uself[i]);

    /*
    .\" First parameter, NAME, should be all caps
    .\" Second parameter, SECTION, should be 1-8, maybe w/ subsection
    .\" other parameters are allowed: see man(7), man(1)
    .TH APPNAME 1 "may 3, 1976"
    */
    fprintf(out, ".TH %s 1 \"%s %2d, %4d\n", uself, months[loctime->tm_mon],
            loctime->tm_mday, loctime->tm_year + 1900);
    /*
    .\" Some roff macros, for reference:
    .\" .nh        disable hyphenation
    .\" .hy        enable hyphenation
    .\" .ad l      left justify
    .\" .ad b      justify to both left and right margins
    .\" .nf        disable filling
    .\" .fi        enable filling
    .\" .br        insert line break
    .\" .sp <n>    insert n+1 empty lines
    .\" for manpage-specific macros, see man(7)
    */
    /*
    .SH NAME
    debtags \- Manage package tag data in a Debian system
    */
    fprintf(out,
            ".SH NAME\n"
            "%s \\- %s\n",
            self, desc.c_str());
    /*
    .SH SYNOPSIS
    .B debtags
    .RI [ options ]
    .RI [ command ]
    .RI [ args ... ]
    .br
    */
    fprintf(out,
            ".SH SYNOPSIS\n"
            ".B %s\n"
            ".RI [ command ]\n"
            ".RI [ options ]\n"
            ".RI [ args ... ]\n"
            ".br\n",
            self);

    /*
    .SH DESCRIPTION
    \fBdebtags\fP manages package tag data in a debian system and performs basic
    queries on it.
    .P
    Package data are activated or updated in the system using the \fBdebtags
    [...]
    */
    fprintf(out, ".SH DESCRIPTION\n");
    for (i = 0; longdesc[i] != '\0'; i++)
        switch (longdesc[i])
        {
            case '\n': fprintf(out, "\n.P\n"); break;
            case '-':  fprintf(out, "\\-"); break;
            default:   putc(longdesc[i], out); break;
        }
    fprintf(out,
            ".\n.P\n"
            "\\fB%s\\fP always requires a non-switch argument, that indicates "
            "what is the "
            "operation that should be performed:\n"
            ".TP\n"
            "\\fBhelp\\fP\n"
            ".br\n"
            "Print a help summary.\n"
            ".TP\n"
            "\\fBhelp manpage\\fP\n"
            ".br\n"
            "Print this manpage.\n",
            self);

    for (auto& op : ops)
    {
        /*
        .TP
        \fBupdate\fP
        .br
        Download the newest version of the package tag
        data and update the system package tag database.
        It needs to be run as root.
        */
        fprintf(out,
                ".TP\n"
                "\\fB%s\\fP\n"
                ".br\n"
                "%s.\n",
                op->usage.c_str(), op->desc.c_str());
        if (!op->longdesc.empty())
            fprintf(out, "%s.\n", op->longdesc.c_str());
    }

    /*
    .SH OPTIONS
    \fBdebtags\fP follow the usual GNU command line syntax, with long options
    starting with two dashes (`-').
    */
    fprintf(out,
            ".SH OPTIONS\n"
            "\\fB%s\\fP follows the usual GNU command line syntax, with long "
            "options "
            "starting with two dashes (`-').\n",
            self);

    /*
     * First trip on all the popt structures to see the subgroups, and document
     * the subgroups as common stuff.
     */
    {
        struct poptOption* seen[20];
        seen[0] = NULL;
        for (Subcommand* op : ops)
        {
            vector<poptOption> optable;
            op->add_to_optable(optable);
            for (auto& sw : optable)
                if (sw.argInfo == POPT_ARG_INCLUDE_TABLE)
                {
                    int is_seen = 0;
                    int j;
                    for (j = 0; seen[j] != NULL && j < 20; j++)
                        if (seen[j] == sw.arg)
                            is_seen = 1;
                    if (!is_seen && j < 20)
                    {
                        seen[j]     = (struct poptOption*)sw.arg;
                        seen[j + 1] = NULL;

                        manpage_print_options(sw.descrip,
                                              (struct poptOption*)sw.arg, out);
                    }
                }
        }
    }
    /*
     * Then document the rest, without
     * repeating the options in the subgroups.
     */
    for (auto& op : ops)
        op->manpage_print_options(out);

    /* .SH EXAMPLES */
    if (!manpage_examples_section.empty())
    {
        fprintf(out, ".SH EXAMPLES\n");
        fputs(manpage_examples_section.c_str(), out);
    }

    /* .SH FILES */
    if (!manpage_files_section.empty())
    {
        fprintf(out, ".SH FILES\n");
        fputs(manpage_files_section.c_str(), out);
    }

    /* .SH SEE ALSO */
    if (!manpage_seealso_section.empty())
    {
        fprintf(out, ".SH SEE ALSO\n");
        fputs(manpage_seealso_section.c_str(), out);
    }

    fprintf(
        out,
        ".SH AUTHOR\n"
        "\\fB%s\\fP has been written by Enrico Zini <enrico@enricozini.com> "
        "for ARPA Emilia Romagna, Servizio Idrometeorologico.\n",
        self);
}

int Command::main(int argc, const char* argv[])
{
    int i;

    /* Dispatch execution to the handler for the various commands */
    for (i = 1; i < argc; i++)
    {
        /* Check if the user asked for help */
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "help") == 0)
        {
            if (i + 1 < argc)
            {
                if (strcmp(argv[i + 1], "manpage") == 0)
                    manpage(stdout);
                else
                {
                    const Subcommand* action = find_action(argv[i + 1]);
                    if (action == nullptr)
                    {
                        fputs("Error parsing commandline: ", stderr);
                        fprintf(stderr,
                                "cannot get help on non-existing command '%s'",
                                argv[i + 1]);
                        fputc('\n', stderr);
                        fputc('\n', stderr);
                        usage(argv[0], stderr);
                        exit(1);
                    }
                    vector<poptOption> opts;
                    poptContext optCon =
                        action->make_popt_context(argc, argv, opts);
                    poptPrintHelp(optCon, stdout, 0);
                    poptFreeContext(optCon);
                }
            }
            else
                usage(argv[0], stdout);
            return 0;
        }

        /* Skip switches */
        if (argv[i][0] == '-')
            continue;

        /* Try the dispatch table */
        Subcommand* action = find_action(argv[i]);
        if (action == nullptr)
        {
            usage(argv[0], stderr);
            return 1;
        }
        else
        {
            vector<poptOption> opts;
            poptContext optCon = action->make_popt_context(argc, argv, opts);

            // Parse commandline
            int nextOp;
            while ((nextOp = poptGetNextOpt(optCon)) != -1)
            {
                if (nextOp == 1)
                {
                    poptPrintHelp(optCon, stdout, 0);
                    return 0;
                }
                else
                {
                    dba_cmdline_error(optCon,
                                      "invalid flag passed in the commandline");
                }
            }

            if (action->op_verbose)
                wreport::notes::set_target(cerr);

            int res = 0;
            try
            {
                action->init();
                res = action->main(optCon);
            }
            catch (error_cmdline& e)
            {
                fprintf(stderr, "Error parsing commandline: %s\n", e.what());
                fputc('\n', stderr);
                poptPrintHelp(optCon, stderr, 0);
                res = 1;
            }
            catch (std::exception& e)
            {
                fprintf(stderr, "%s\n", e.what());
                res = 1;
            }
            poptFreeContext(optCon);
            return res;
        }
    }

    /* Nothing found on the dispatch table */
    usage(argv[0], stderr);
    return 1;
}

unsigned dba_cmdline_get_query(poptContext optCon, Query& query)
{
    core::Query& q = core::Query::downcast(query);
    unsigned res;
    const char* queryparm;
    for (res = 0; (queryparm = poptPeekArg(optCon)) != NULL; ++res)
    {
        /* Split the input as name=val */
        if (strchr(queryparm, '=') == NULL)
            break;

        /* Mark as processed */
        poptGetArg(optCon);

        q.set_from_string(queryparm);
    }
    q.validate();
    return res;
}

void list_templates()
{
    const impl::msg::wr::TemplateRegistry& reg =
        impl::msg::wr::TemplateRegistry::get();
    for (impl::msg::wr::TemplateRegistry::const_iterator i = reg.begin();
         i != reg.end(); ++i)
        fprintf(stdout, "%s - %s\n", i->second.name.c_str(),
                i->second.description.c_str());
}

std::list<std::string> get_filenames(poptContext optCon)
{
    std::list<std::string> res;
    while (const char* name = poptGetArg(optCon))
        res.push_back(name);
    return res;
}

} // namespace cmdline
} // namespace dballe
