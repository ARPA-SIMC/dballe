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

#include "config.h"

#include <dballe/core/verbose.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "??? ?? ????"
#endif

#ifdef __TIME__
#define TIME __TIME__
#else
#define TIME "??:??:??"
#endif

#define out DBA_VERBOSE_STREAM

static int level = DBA_VERB_NONE;

void dba_verbose_init()
{
	const char* verb = getenv("DBA_VERBOSE");
	if (verb != NULL)
		level = atoi(verb);
	if (level != DBA_VERB_NONE)
	{
		fprintf(out, "%s\n", PACKAGE_NAME " " PACKAGE_VERSION ", compiled on " DATE " " TIME
#ifdef USE_MYSQL4
			" using"
#else
			" avoiding"
#endif
			" MySQL4 features"
		);
		fprintf(out, "%s",
			"Copyright (C) 2005-2006 ARPA Emilia Romagna.\n"
			"DB-ALLe comes with ABSOLUTELY NO WARRANTY.\n"
			"This is free software, and you are welcome to redistribute it and/or modify it\n"
			"under the terms of the GNU General Public License as published by the Free\n"
			"Software Foundation; either version 2 of the License, or (at your option) any\n"
			"later version.\n");
	}
}

int dba_verbose_is_allowed(int lev)
{
	return (level & lev) != 0;
}

void dba_verbose(int lev, const char* fmt, ...)
{
	va_list ap;
	if (!dba_verbose_is_allowed(lev))
		return;

	va_start(ap, fmt);
	vfprintf(out, fmt, ap);
	va_end(ap);
}
