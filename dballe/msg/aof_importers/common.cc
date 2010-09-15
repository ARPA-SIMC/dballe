/*
 * DB-ALLe - Archive for punctual meteorological data
 *
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

#include "common.h"

#include <time.h>		/* struct tm */
#include <stdint.h>		/* uint32_t */
#include <strings.h>	/* bzero */
#include <string.h>		/* strncpy */
#include <stdlib.h>		/* strtol */
#include <ctype.h>		/* isspace */

namespace dballe {
namespace msg {

#if 0
void dba_aof_dump_word(const char* prefix, uint32_t x)
{
	int i;
	fprintf(stderr, "%s", prefix);
	for (i = 0; i < 32; i++)
	{
		fprintf(stderr, "%c", (x & 0x80000000) != 0 ? '1' : '0');
		x <<= 1;
		if ((i+1) % 8 == 0)
			fprintf(stderr, " ");
	}
}

/* Set the confidence attribute of the given variable from the given 2-bit AOF
 * quality flag */
static inline dba_err set_confidence2(dba_var var, int conf)
{
	dba_err err = DBA_OK;
	dba_var attr = NULL;

	DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
	DBA_RUN_OR_GOTO(cleanup, dba_var_seti(attr, get_conf2(conf)));
	DBA_RUN_OR_GOTO(cleanup, dba_var_seta_nocopy(var, attr));
	attr = NULL;

cleanup:
	if (attr != NULL)
		dba_var_delete(attr);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* Set the confidence attribute of the given variable from the given 6-bit AOF
 * quality flag */
static inline dba_err set_confidence6(dba_var var, int conf)
{
	return set_confidence2(var, conf >> 3);
}
#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
