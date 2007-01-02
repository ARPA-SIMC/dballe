/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * Common functions for commandline tools
 */

#include <dballe/core/error.h>
#include <dballe/core/record.h>
#include <dballe/msg/file.h>
#include <popt.h>

struct op_dispatch_table
{
	dba_err (*func)(poptContext);
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
dba_encoding dba_cmdline_stringToMsgType(const char* type, poptContext optCon);

/**
 * Process commandline arguments and perform the action requested
 */
int dba_cmdline_dispatch_main(const struct tool_desc* desc, int argc, const char* argv[]);

/**
 * Get a DB-ALLe query from commandline parameters in the form key=value
 */
dba_err dba_cmdline_get_query(poptContext optCon, dba_record query);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
