#ifndef DBA_CMDLINE_H
#define DBA_CMDLINE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * Common functions for commandline tools
 */

#include <stdio.h>
#include <dballe/err/dba_error.h>
#include <dballe/core/dba_record.h>
#include <dballe/dba_file.h>
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
