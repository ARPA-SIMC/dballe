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

dba_err bufrex_copy_to_metar(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	int i;
	int height_above_count = 0;
	int height = -1;

	msg->type = MSG_METAR;
	
	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
		{
			switch (dba_var_code(var))
			{
				case WR_VAR(0,  7,  6): height_above_count++; break;
			}
			continue;
		}
		switch (dba_var_code(var))
		{
			case WR_VAR(0,  7,  1): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			case WR_VAR(0,  7,  6):
				switch (height_above_count++)
				{
					case 0:
						DBA_RUN_OR_RETURN(dba_var_enqi(var, &height));
						if (height != 10)
							return dba_error_consistency("checking that wind level is 10M");
						break;
					case 1:
						DBA_RUN_OR_RETURN(dba_var_enqi(var, &height));
						if (height != 2)
							return dba_error_consistency("checking that temperature level is 2M");
						break;
				}
				break;
			case WR_VAR(0, 11,  1): DBA_RUN_OR_RETURN(dba_msg_set_wind_dir_var(msg, var)); break;
			case WR_VAR(0, 11, 16): DBA_RUN_OR_RETURN(dba_msg_set_ex_ccw_wind_var(msg, var)); break;
			case WR_VAR(0, 11, 17): DBA_RUN_OR_RETURN(dba_msg_set_ex_cw_wind_var(msg, var)); break;
			case WR_VAR(0, 11,  2): DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, var)); break;
			case WR_VAR(0, 11, 41): DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, var)); break;
			case WR_VAR(0, 12,  1): DBA_RUN_OR_RETURN(dba_msg_set_temp_2m_var(msg, var)); break;
			case WR_VAR(0, 12,  3): DBA_RUN_OR_RETURN(dba_msg_set_dewpoint_2m_var(msg, var)); break;
			case WR_VAR(0, 10, 52): DBA_RUN_OR_RETURN(dba_msg_set_qnh_var(msg, var)); break;
			case WR_VAR(0, 20,  9): DBA_RUN_OR_RETURN(dba_msg_set_metar_wtr_var(msg, var)); break;
		}
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
