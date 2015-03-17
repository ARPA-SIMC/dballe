/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_CMDLINE_H
#define DBA_CMDLINE_H

/** @file
 * @ingroup dballe
 * Common functions for commandline tools
 */

#include <wreport/error.h>
#include <dballe/core/rawmsg.h>
#include <popt.h>
#include <memory>
#include <vector>
#include <list>
#include <string>

namespace dballe {
struct Record;

namespace cmdline {

struct Subcommand
{
    std::vector<std::string> names;
    std::string usage;
    std::string desc;
    std::string longdesc;
    int op_verbose;

    virtual ~Subcommand() {}

    virtual void add_to_optable(std::vector<poptOption>& opts) const;
    virtual int main(poptContext) = 0;

    /**
     * Create a popt context for this subcommand.
     *
     * Options are appended to opts, which is generally passed empty. Its
     * memory needs to be owned by the caller, because the resulting
     * poptContext will refer to data inside it, so the lifetime of the vector
     * should be at least as long as the lifetime of the resulting poptContext.
     */
    poptContext make_popt_context(int argc, const char* argv[], std::vector<poptOption>& opts) const;
    void manpage_print_options(FILE* out);
};

#define ODT_END { NULL, NULL, NULL, NULL, NULL, NULL }

struct Command
{
    std::string name;
    std::string desc;
    std::string longdesc;
    std::string manpage_examples_section;
    std::string manpage_files_section;
    std::string manpage_seealso_section;

    std::vector<Subcommand*> ops;

    ~Command();

    /// Add an action to this tool, taking ownership of its memory management
    void add_subcommand(Subcommand* action);
    void add_subcommand(std::unique_ptr<Subcommand>&& action);

    Subcommand* find_action(const std::string& name) const;

    void usage(const std::string& selfpath, FILE* out) const;
    void manpage(FILE* out) const;

    /// Process commandline arguments and perform the action requested
    int main(int argc, const char* argv[]);
};

/// Report an error with command line options
struct error_cmdline : public std::exception
{
    std::string msg; ///< error message returned by what()

    /// @param msg error message
    error_cmdline(const std::string& msg) : msg(msg) {}
    ~error_cmdline() throw () {}

    virtual const char* what() const throw () { return msg.c_str(); }

    /// Throw the exception, building the message printf-style
    static void throwf(const char* fmt, ...) WREPORT_THROWF_ATTRS(1, 2);
};


/**
 * Print informations about the last error to stderr
 */
void dba_cmdline_print_dba_error();

/**
 * Print an error that happened when parsing commandline arguments, then add
 * usage informations and exit
 */
void dba_cmdline_error(poptContext optCon, const char* fmt, ...) __attribute__ ((noreturn));

/**
 * Return the ::dba_encoding that corresponds to the name in the string
 */
Encoding dba_cmdline_stringToMsgType(const char* type);

/**
 * Get a DB-ALLe query from commandline parameters in the form key=value
 *
 * @return the number of key=value pairs found
 */
unsigned dba_cmdline_get_query(poptContext optCon, Record& query);

/**
 * List available output templates
 */
void list_templates();

/// Read all the command line arguments and return them as a list
std::list<std::string> get_filenames(poptContext optCon);

} // namespace cmdline
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
