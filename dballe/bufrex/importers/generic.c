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

#include <dballe/msg/dba_msg.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/core/dba_record.h>

dba_err bufrex_copy_to_generic(dba_msg msg, bufrex_raw raw)
{
	dba_err err = DBA_OK;
	dba_var copy = NULL;
	int i;
	int ltype = -1, l1 = -1, l2 = -1, pind = -1, p1 = -1, p2 = -1;

	msg->type = MSG_GENERIC;

	for (i = 0; i < raw->vars_count; i++)
	{
		dba_var var = raw->vars[i];

		if (dba_var_value(var) == NULL)
		{
			/* Also skip attributes if there are some following */
			for ( ; i + 1 < raw->vars_count &&
					DBA_VAR_X(dba_var_code(raw->vars[i + 1])) == 33; i++)
				;
			continue;
		}

		switch (dba_var_code(var))
		{
			case DBA_VAR(0, 4, 192): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &pind)); break;
			case DBA_VAR(0, 4, 193): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &p1)); break;
			case DBA_VAR(0, 4, 194): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &p2)); break;
			case DBA_VAR(0, 7, 192): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &ltype)); break;
			case DBA_VAR(0, 7, 193): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &l1)); break;
			case DBA_VAR(0, 7, 194): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &l2)); break;
			default:
				if (ltype == -1 || l1 == -1 || l2 == -1 || pind == -1 || p1 == -1 || p2 == -1)
					DBA_FAIL_GOTO(cleanup, dba_error_consistency(
							"Incomplete context informations l(%d,%d,%d),p(%d,%d,%d) for variable %d%02d%03d",
							ltype, l1, l2, pind, p1, p2, dba_var_code(var)));

				DBA_RUN_OR_GOTO(cleanup, dba_var_copy(var, &copy));

				/* Add attributes if there are some following */
				for ( ; i + 1 < raw->vars_count &&
						DBA_VAR_X(dba_var_code(raw->vars[i + 1])) == 33; i++)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, raw->vars[i + 1]));
				
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, copy, ltype, l1, l2, pind, p1, p2));
				copy = NULL;
				break;
		}
	}

cleanup:
	if (copy != NULL)
		dba_var_delete(copy);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* vim:set ts=4 sw=4: */
