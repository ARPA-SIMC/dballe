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
#include "dballe/msg/msg.h"
#include "dballe/core/conv.h"
#include "dballe/bufrex/msg.h"
#include <math.h>

dba_err bufrex_copy_to_pollution(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	int i;
	// Above ground is ltype = 105, l1 = metri (se undef, cosa metto? Chiedo a
	// Stortini per un default?)
	int ltype = -1, l1 = -1;
	int ptype = -1, p1 = -1, p2 = -1;
	int valtype = 0;
	int decscale = 0;
	double value = 0;
	int conf = 0;

	msg->type = MSG_POLLUTION;
	
	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
			continue;
		switch (dba_var_code(var))
		{
			case DBA_VAR(0,  1,  19): DBA_RUN_OR_RETURN(dba_msg_set_st_name_var(msg, var)); break;
			case DBA_VAR(0,  1, 212): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 1, 212), 257, 0, 0, 0, 0, 0)); break;
			case DBA_VAR(0,  1, 213): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 1, 213), 257, 0, 0, 0, 0, 0)); break;
			case DBA_VAR(0,  1, 214): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 1, 214), 257, 0, 0, 0, 0, 0)); break;
			case DBA_VAR(0,  1, 215): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 1, 215), 257, 0, 0, 0, 0, 0)); break;
			case DBA_VAR(0,  1, 216): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 1, 216), 257, 0, 0, 0, 0, 0)); break;
			case DBA_VAR(0,  1, 217): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 1, 217), 257, 0, 0, 0, 0, 0)); break;
			case DBA_VAR(0,  4,   1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,   2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,   3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,   4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,   5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  4,   6): DBA_RUN_OR_RETURN(dba_msg_set_second_var(msg, var)); break;
			case DBA_VAR(0,  5,   1):
			case DBA_VAR(0,  5,   2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  6,   1):
			case DBA_VAR(0,  6,   2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			// TODO: discuss with Paolo
			case DBA_VAR(0,  7,  30): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			// TODO: discuss with Paolo
			case DBA_VAR(0,  7,  31): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			case DBA_VAR(0,  8,  21): {
				// TODO: map
				int val;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &val));
				switch (val)
				{
					case 1: ptype = 0; p1 = 0; p2 = 0; break;
					case 2: ptype = 0; p1 = 0; p2 = 0; break;
					case 3: ptype = 0; p1 = 0; p2 = 0; break;
					default:
						// TODO, all of these
						break;
				}
				break;
			}
			case DBA_VAR(0,  4,  25): DBA_RUN_OR_RETURN(dba_var_enqi(var, &p1)); break; // FIXME
			case DBA_VAR(0,  8,  43): {
				int val;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &val));
				switch (val)
				{
					case 1: valtype = DBA_VAR(0, 1, 2); break;
					case 2: valtype = DBA_VAR(0, 2, 3); break;
					case 3: valtype = DBA_VAR(0, 3, 4); break;
					default:
						// FIXME, all of these
						break;
				}

				break;
			}
			case DBA_VAR(0,  8,  44): {
				// 008044 (VAL) CAS REGISTRY NUMBER(CCITTIA5): 10102-44-0
				// (as attribute?)
				break;
			}
			case DBA_VAR(0,  8,  45): {
				// 008045 (VAL) PARTICULATE MATTER CHARACTERIZATION(CODE TABLE 008045): (undef)
				// (as attribute?)
				break;
			}
			case DBA_VAR(0,  8,  90): DBA_RUN_OR_RETURN(dba_var_enqi(var, &decscale)); break;
			case DBA_VAR(0, 15,  23): DBA_RUN_OR_RETURN(dba_var_enqd(var, &value)); break;
			case DBA_VAR(0, 33,   3): {
				int val;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &val));
				switch (val)
				{
					case 1: conf = 10; break;
					case 2: conf = 40; break;
					case 3: conf = 80; break;
					case 4: conf = 100; break;
					default:
						// FIXME, all of these
						break;
				}
				break;
			}
		}
	}
	value = value * exp10(decscale);
	DBA_RUN_OR_RETURN(dba_msg_setd(msg, valtype, value, conf, 1, 0, 0, ptype, p1, p2));
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
