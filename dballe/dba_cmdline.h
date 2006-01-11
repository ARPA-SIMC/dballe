#ifndef DBA_CMDLINE_H
#define DBA_CMDLINE_H

/** \file
 * Common functions for commandline tools
 */

#ifdef  __cplusplus
extern "C" {
#endif

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


void dba_cmdline_print_dba_error();
void dba_cmdline_error(poptContext optCon, const char* fmt, ...) __attribute__ ((noreturn));
dba_encoding dba_cmdline_stringToMsgType(const char* type, poptContext optCon);
int dba_cmdline_dispatch_main(const struct tool_desc* desc, int argc, const char* argv[]);

dba_err dba_cmdline_get_query(poptContext optCon, dba_record query);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
