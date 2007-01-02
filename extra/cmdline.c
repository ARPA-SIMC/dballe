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

#define _GNU_SOURCE
/* _GNU_SOURCE is defined only to have strndup */

#include "cmdline.h"
#include <dballe/core/aliases.h>
#include <dballe/core/verbose.h>

#include <popt.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

static const struct op_dispatch_table* op_table_lookup(const struct tool_desc* desc, const char* name);
static void usage(const struct tool_desc* desc, const char* selfpath, FILE* out);

void dba_cmdline_print_dba_error()
{
	dba_error_print_to_stderr();
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

dba_encoding dba_cmdline_stringToMsgType(const char* type, poptContext optCon)
{
	if (strcmp(type, "bufr") == 0 || strcmp(type, "b") == 0)
	{
		return BUFR;
	}
	else if (strcmp(type, "crex") == 0 || strcmp(type, "c") == 0)
	{
		return CREX;
	}
	else if (strcmp(type, "aof") == 0 || strcmp(type, "a") == 0)
	{
		return AOF;
	}
	else if (strcmp(type, "auto") == 0)
	{
		return (dba_encoding)-1;
	}
	else
		dba_cmdline_error(optCon, "'%s' is not a valid format type", type);
}

static const struct op_dispatch_table* op_table_lookup(const struct tool_desc* desc, const char* name)
{
	const struct op_dispatch_table* op_table = desc->ops;
	int i, j;
	for (i = 0; op_table[i].desc != NULL; i++)
		for (j = 0; op_table[i].aliases[j] != NULL; j++)
			if (strcmp(name, op_table[i].aliases[j]) == 0)
				return &(op_table[i]);
	return NULL;
}

static void usage(const struct tool_desc* desc, const char* selfpath, FILE* out)
{
	const struct op_dispatch_table* op_table = desc->ops;
	int i, j;
	const char* self = strrchr(selfpath, '/');
	if (self == NULL) self = selfpath; else self++;

	fprintf(out, "Usage: %s <command> [options] [arguments]\n\n%s.\n%s.\n\n",
			self, desc->desc, desc->longdesc);
	fprintf(out, "Available commands are:\n");
	fprintf(out, "\t%s help\n", self);
	fprintf(out, "\t\tdisplay this help message\n");
	fprintf(out, "\t%s help manpage\n", self);
	fprintf(out, "\t\tgenerate the manpage for %s\n", self);

	for (i = 0; op_table[i].desc != NULL; i++)
	{
		for (j = 0; op_table[i].aliases[j] != NULL; j++)
		{
			if (j == 0)
				fprintf(out, "\t%s %s", self, op_table[i].aliases[j]);
			else
				fprintf(out, " or %s", op_table[i].aliases[j]);
		}
		fprintf(out, "\n\t\t%s\n", op_table[i].desc);
	}

	fprintf(out, "\nCalling `%s <command> --help' will give help on the specific command\n", self);
}

static int is_tableend(struct poptOption* op)
{
	return op->longName == NULL && \
		   op->shortName == '\0' && \
		   op->argInfo == 0 && \
		   op->arg == 0 && \
		   op->val == 0 && \
		   op->descrip == NULL && \
		   op->argDescrip == NULL;
}

static void manpage_print_options(const char* title, struct poptOption* optable, FILE* out)
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
			if (optable[i].longName != '\0')
			{
				fprintf(out, "%s \\-\\-%s", bol ? "" : ",", optable[i].longName);
				bol = 0;
			}
		} else {
			if (optable[i].shortName != '\0')
			{
				fprintf(out, " \\-%c %s", optable[i].shortName, optable[i].argDescrip);
				bol = 0;
			}
			if (optable[i].longName != '\0')
			{
				fprintf(out, "%s \\-\\-%s=%s", bol ? "" : ",", optable[i].longName, optable[i].argDescrip);
				bol = 0;
			}
		}
		fprintf(out, "\n%s\n", optable[i].descrip);
	}
}

void manpage(const struct tool_desc* desc, const char* selfpath, FILE* out)
{
	static const char* months[] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
	const struct op_dispatch_table* op_table = desc->ops;
	const char* self = strrchr(selfpath, '/');
	char* uself;
	time_t curtime = time(NULL);
	struct tm* loctime = localtime(&curtime);
	int i, op;
	
	if (self == NULL) self = selfpath; else self++;
	/* Remove libtool cruft from program name if present */
	if (strncmp(self, "lt-", 3) == 0) self += 3;
	uself = strdup(self);
	for (i = 0; uself[i] != '\0'; i++)
		uself[i] = toupper(uself[i]);

	/*
	.\" First parameter, NAME, should be all caps
	.\" Second parameter, SECTION, should be 1-8, maybe w/ subsection
	.\" other parameters are allowed: see man(7), man(1)
	.TH APPNAME 1 "may 3, 1976"
	*/
	fprintf(out, ".TH %s 1 \"%s %2d, %4d\n",
			uself, months[loctime->tm_mon], loctime->tm_mday, loctime->tm_year + 1900);
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
			"%s \\- %s\n", self, desc->desc);
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
			".br\n", self);

	/*
	.SH DESCRIPTION
	\fBdebtags\fP manages package tag data in a debian system and performs basic
	queries on it.
	.P
	Package data are activated or updated in the system using the \fBdebtags
	[...]
	*/
	fprintf(out, ".SH DESCRIPTION\n");
	for (i = 0; desc->longdesc[i] != '\0'; i++)
		switch (desc->longdesc[i])
		{
			case '\n': fprintf(out, "\n.P\n"); break;
			case '-': fprintf(out, "\\-"); break;
			default:
				putc(desc->longdesc[i], out);
				break;
		}
	fprintf(out, ".\n.P\n"
			"\\fB%s\\fP always requires a non-switch argument, that indicates what is the "
			"operation that should be performed:\n"
			".TP\n"
			"\\fBhelp\\fP\n"
			".br\n"
			"Print a help summary.\n"
			".TP\n"
			"\\fBhelp manpage\\fP\n"
			".br\n"
			"Print this manpage.\n", self);
	
	for (op = 0; op_table[op].desc != NULL; op++)
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
					op_table[op].usage,
					op_table[op].desc);
		if (op_table[op].longdesc != NULL)
			fprintf(out, "%s.\n", op_table[op].longdesc);
	}

	/*
	.SH OPTIONS
	\fBdebtags\fP follow the usual GNU command line syntax, with long options
	starting with two dashes (`-').
	*/
	fprintf(out,
			".SH OPTIONS\n"
			"\\fB%s\\fP follows the usual GNU command line syntax, with long options "
			"starting with two dashes (`-').\n", self);

	/*
	 * First trip on all the popt structures to see the subgroups, and document
	 * the subgroups as common stuff.
	 */
	{
		struct poptOption* seen[20];
		seen[0] = NULL;
		for (op = 0; op_table[op].desc != NULL; op++)
			for (i = 0; !is_tableend(&(op_table[op].optable[i])); i++)
				if (op_table[op].optable[i].argInfo == POPT_ARG_INCLUDE_TABLE)
				{
					int is_seen = 0;
					int j;
					for (j = 0; seen[j] != NULL && j < 20; j++)
						if (seen[j] == op_table[op].optable[i].arg)
							is_seen = 1;
					if (!is_seen && j < 20)
					{
						seen[j] = op_table[op].optable[i].arg;
						seen[j+1] = NULL;

						manpage_print_options(
								op_table[op].optable[i].descrip, 
								(struct poptOption*)op_table[op].optable[i].arg, 
								out);
					}
				}
	}
	/*
	 * Then document the rest, without
	 * repeating the options in the subgroups.
	 */
	for (op = 0; op_table[op].desc != NULL; op++)
	{
		char title[128];
		strcpy(title, "Option for command ");
		strcat(title, op_table[op].aliases[0]);
		manpage_print_options(
				title,
				(struct poptOption*)op_table[op].optable, 
				out);
	}
/*
.SH FILES
.TP
.B /var/lib/debtags/vocabulary
.br
The normative tag vocabulary
.TP
.B /etc/debtags/sources.list
.br
The list of sources to build the package tags database from
.TP
.B /var/lib/debtags/package\-tags
.br
The system package tags database, only kept as an easily parsable reference.
In the same directory there is a the binary index with the same content, used
by applications for fast access.

.SH SEE ALSO
.BR debtags-edit (1),
.BR tagcoll (1),
.BR tagcolledit (1),
.BR apt-cache (1),
.br
.BR http://debtags.alioth.debian.org/
.br
.BR http://debian.vitavonni.de/packagebrowser/
*/
	/*
	.SH AUTHOR
	\fBdebtags\fP has been written by Enrico Zini <enrico@debian.org>, with a great
	deal of ideas and feedback from many people around the debtags-devel,
	debian-devel and the previous deb-usability mailing lists.
	*/		
	fprintf(out,
			".SH AUTHOR\n"
			"\\fB%s\\fP has been written by Enrico Zini <enrico@enricozini.com> "
			"for ARPA Emilia Romagna, Servizio Idrometeorologico.\n", self);
}

int dba_cmdline_dispatch_main (const struct tool_desc* desc, int argc, const char* argv[])
{
	int i;

	dba_verbose_init();

	/* Dispatch execution to the handler for the various commands */
	for (i = 1; i < argc; i++)
	{
		const struct op_dispatch_table* action;

		/* Check if the user asked for help */
		if (strcmp(argv[i], "--help") == 0 ||
		    strcmp(argv[i], "help") == 0)
		{
			if (i+1 < argc)
			{
				if (strcmp(argv[i+1], "manpage") == 0)
					manpage(desc, argv[0], stdout);
				else
				{
					poptContext optCon;
					char help[1024];

					if ((action = op_table_lookup(desc, argv[i+1])) == NULL)
					{
						fputs("Error parsing commandline: ", stderr);
						fprintf(stderr, "cannot get help on non-existing command '%s'", argv[i+1]);
						fputc('\n', stderr);
						fputc('\n', stderr);
						usage(desc, argv[0], stderr);
						exit(1);
					}
					optCon = poptGetContext(NULL, argc, argv, action->optable, 0);

					/* Build the help information for this entry */
					strcpy(help, action->usage);
					strcat(help, "\n\n");
					strcat(help, action->desc);
					strcat(help, ".\n\n");
					if (action->longdesc != NULL)
					{
						strcat(help, action->longdesc);
						strcat(help, ".\n\n");
					}
					strcat(help, "Options are:");
					poptSetOtherOptionHelp(optCon, help);

					poptPrintHelp(optCon, stdout, 0);
				}
			}
			else
				usage(desc, argv[0], stdout);
			return 0;
		}
		
		/* Skip switches */
		if (argv[i][0] == '-')
			continue;

		/* Try the dispatch table */
		action = op_table_lookup(desc, argv[i]);
		if (action == NULL)
		{
			usage(desc, argv[0], stderr);
			return 1;
		} else {
			poptContext optCon = poptGetContext(NULL, argc, argv, action->optable, 0);
			char help[1024];
			int nextOp;
			dba_err err;

			/* Build the help information for this entry */
			strcpy(help, action->usage);
			strcat(help, "\n\n");
			strcat(help, action->desc);
			strcat(help, ".\n\n");
			if (action->longdesc != NULL)
			{
				strcat(help, action->longdesc);
				strcat(help, ".\n\n");
			}
			strcat(help, "Options are:");

			poptSetOtherOptionHelp(optCon, help);

			/* Parse commandline */
			while ((nextOp = poptGetNextOpt(optCon)) != -1)
			{
				if (nextOp == 1)
				{
					poptPrintHelp(optCon, stdout, 0);
					return 0;
				} else {
					dba_cmdline_error(optCon, "invalid flag passed in the commandline");
				}
			}

			err = action->func(optCon);
			poptFreeContext(optCon);

			if (err == DBA_OK)
				return 0;
			else {
				dba_cmdline_print_dba_error();
				return dba_error_get_code();
			}
		}
	}

	/* Nothing found on the dispatch table */
	usage(desc, argv[0], stderr);
	return 1;
}

dba_err dba_cmdline_get_query(poptContext optCon, dba_record query)
{
	const char* queryparm;
	while ((queryparm = poptPeekArg(optCon)) != NULL)
	{
		/* Split the input as name=val */
		if (strchr(queryparm, '=') == NULL)
			return dba_error_ok();

		/* Mark as processed */
		poptGetArg(optCon);

		DBA_RUN_OR_RETURN(dba_record_set_from_string(query, queryparm));
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
