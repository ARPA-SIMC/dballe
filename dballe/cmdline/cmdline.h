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

#ifndef DBA_CMDLINE_H
#define DBA_CMDLINE_H

/** @file
 * @ingroup dballe
 * Common functions for commandline tools
 */

#include <dballe/core/rawmsg.h>
#include <popt.h>

namespace dballe {
struct Record;

namespace cmdline {

struct op_dispatch_table
{
	int (*func)(poptContext);
	const char* aliases[3];
	const char* usage;
	const char* desc;
	const char* longdesc;
	struct poptOption* optable;
};

#define ODT_END { NULL, NULL, NULL, NULL, NULL, NULL }

struct tool_desc
{
	const char* desc;
	const char* longdesc;
	struct op_dispatch_table* ops;	
};

struct program_info
{
	const char* name;
	const char* manpage_examples_section;
	const char* manpage_files_section;
	const char* manpage_seealso_section;
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
Encoding dba_cmdline_stringToMsgType(const char* type, poptContext optCon);

/**
 * Process commandline arguments and perform the action requested
 */
int dba_cmdline_dispatch_main(const struct program_info* pinfo, const struct tool_desc* desc, int argc, const char* argv[]);

/**
 * Get a DB-ALLe query from commandline parameters in the form key=value
 */
void dba_cmdline_get_query(poptContext optCon, Record& query);

/**
 * List available output templates
 */
void list_templates();

} // namespace cmdline
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
