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

#include "bench/Benchmark.h"
#include <dballe/core/verbose.h>

#include <cstdio>
#include <cstdlib>
#include <sys/times.h>
#include <unistd.h>
#include <iostream>

#define timing(fmt, ...) do { \
	struct tms curtms; \
	if (times(&curtms) == -1) \
		return dba_error_system("reading timing informations"); \
	fprintf(stderr, fmt, __VA_ARGS__); \
	fprintf(stderr, ": %.3f user, %.3f system, %.3f total\n", \
			(curtms.tms_utime - lasttms.tms_utime)/tps,\
			(curtms.tms_stime - lasttms.tms_stime)/tps,\
			((curtms.tms_utime - lasttms.tms_utime) + (curtms.tms_stime - lasttms.tms_stime))/tps); \
	lasttms = curtms; \
} while (0)

using namespace std;

static void print_dba_error()
{
	const char* details = dba_error_get_details();
	fprintf(stderr, "Error %d (%s) while %s",
			dba_error_get_code(),
			dba_error_get_message(),
			dba_error_get_context());
	if (details == NULL)
		fputc('\n', stderr);
	else
		fprintf(stderr, ".  Details:\n%s\n", details);
}

int main(int argc, const char* argv[])
{
	dba_err err = DBA_OK;

	// We want predictable results
	srand(1);

	dba_verbose_init();

	if (argc == 1)
		DBA_RUN_OR_GOTO(fail, Benchmark::root()->run());
	else if (argv[1] == string("--list"))
	{
		Benchmark::root()->list(cout);
	}
	else
	{
		for (int i = 1; i < argc; i++)
			if (argv[i][0] == '/')
				DBA_RUN_OR_GOTO(fail, Benchmark::root()->run(argv[i]+1));
			else
				DBA_RUN_OR_GOTO(fail, Benchmark::root()->run(argv[i]));
	}

	return 0;

fail:
	print_dba_error();
	return 1;
}

/* vim:set ts=4 sw=4: */
