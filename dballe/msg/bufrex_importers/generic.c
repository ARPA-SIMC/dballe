/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "dballe/msg/msg.h"
#include "dballe/bufrex/msg.h"

dba_err bufrex_copy_to_generic(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	dba_err err = DBA_OK;
	dba_var copy = NULL;
	int i;
	int ltype1 = -1, ltype2 = -1, l1 = -1, l2 = -1, pind = -1, p1 = -1, p2 = -1;

	msg->type = MSG_GENERIC;

	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
		{
			/* Also skip attributes if there are some following */
			for ( ; i + 1 < sset->vars_count &&
					DBA_VAR_X(dba_var_code(sset->vars[i + 1])) == 33; i++)
				;
			continue;
		}

		switch (dba_var_code(var))
		{
			case DBA_VAR(0, 4, 192): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &pind)); break;
			case DBA_VAR(0, 4, 193): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &p1)); break;
			case DBA_VAR(0, 4, 194): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &p2)); break;
			case DBA_VAR(0, 7, 192): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &ltype1)); break;
			case DBA_VAR(0, 7, 193): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &l1)); break;
			case DBA_VAR(0, 7, 194): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &l2)); break;
			case DBA_VAR(0, 7, 195): DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(var, &ltype2)); break;
			case DBA_VAR(0, 1, 194):
			{
				// Set the rep memo if we found it
				const char* val = dba_var_value(var);
				if (val)
				{
					msg->type = dba_msg_type_from_repmemo(val);
					DBA_RUN_OR_GOTO(cleanup, dba_msg_set_rep_memo(msg, val, -1));
				}
				break;
			}
			default:
				if (ltype1 == -1 || l1 == -1 || ltype2 == -1 || l2 == -1 || pind == -1 || p1 == -1 || p2 == -1)
					DBA_FAIL_GOTO(cleanup, dba_error_consistency(
							"Incomplete context informations l(%d,%d, %d,%d),p(%d,%d,%d) for variable %d%02d%03d",
							ltype1, l1, ltype2, l2, pind, p1, p2,
							DBA_VAR_F(dba_var_code(var)),
							DBA_VAR_X(dba_var_code(var)),
							DBA_VAR_Y(dba_var_code(var))));

				DBA_RUN_OR_GOTO(cleanup, dba_var_copy(var, &copy));

				/* Add attributes if there are some following */
				for ( ; i + 1 < sset->vars_count &&
						DBA_VAR_X(dba_var_code(sset->vars[i + 1])) == 33; i++)
					DBA_RUN_OR_GOTO(cleanup, dba_var_seta(copy, sset->vars[i + 1]));
				
				DBA_RUN_OR_GOTO(cleanup, dba_msg_set_nocopy(msg, copy, ltype1, l1, ltype2, l2, pind, p1, p2));
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
