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

#include "dballe/msg/dba_msg.h"
#include "dballe/bufrex/bufrex_msg.h"

dba_err bufrex_copy_to_temp(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	int i;
	int cloud_type_count = 0;
	double press = -1;
	dba_var press_var = NULL;
	double surface_press = -1;
	dba_var surface_press_var = NULL;

	switch (raw->subtype)
	{
		case 0:
			/* Guess looking at the variables */
			if (sset->vars_count > 1 && dba_var_code(sset->vars[0]) == DBA_VAR(0, 1, 11))
				msg->type = MSG_TEMP_SHIP;
			else
				msg->type = MSG_TEMP;
			break;
		case 101: msg->type = MSG_TEMP; break;
		case 92:
		case 102: msg->type = MSG_TEMP_SHIP; break;
		default: msg->type = MSG_GENERIC; break;
	}

	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
		{
			switch (dba_var_code(var))
			{
				case DBA_VAR(0, 20, 12): cloud_type_count++; break;
			}
			continue;
		}

		switch (dba_var_code(var))
		{
			case DBA_VAR(0,  1,  1): DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg, var)); break;
			case DBA_VAR(0,  1,  2): DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg, var)); break;
			case DBA_VAR(0,  1, 11): DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg, var)); break;
			case DBA_VAR(0,  1, 12): DBA_RUN_OR_RETURN(dba_msg_set_st_dir_var(msg, var)); break;
			case DBA_VAR(0,  1, 13): DBA_RUN_OR_RETURN(dba_msg_set_st_speed_var(msg, var)); break;
			case DBA_VAR(0,  2, 11): DBA_RUN_OR_RETURN(dba_msg_set_sonde_type_var(msg, var)); break;
			case DBA_VAR(0,  2, 12): DBA_RUN_OR_RETURN(dba_msg_set_sonde_method_var(msg, var)); break;
			case DBA_VAR(0,  4,  1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,  2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,  3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,  4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,  5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  5,  1): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  5,  2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  1): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  7,  1): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;

			case DBA_VAR(0, 20, 10): DBA_RUN_OR_RETURN(dba_msg_set_cloud_n_var(msg, var)); break;
			case DBA_VAR(0, 20, 11): DBA_RUN_OR_RETURN(dba_msg_set_cloud_nh_var(msg, var)); break;
			case DBA_VAR(0, 20, 13): DBA_RUN_OR_RETURN(dba_msg_set_cloud_hh_var(msg, var)); break;
			case DBA_VAR(0, 20, 12):
				switch (cloud_type_count++)
				{
					case 0: DBA_RUN_OR_RETURN(dba_msg_set_cloud_cl_var(msg, var)); break;
					case 1: DBA_RUN_OR_RETURN(dba_msg_set_cloud_cm_var(msg, var)); break;
					case 2: DBA_RUN_OR_RETURN(dba_msg_set_cloud_ch_var(msg, var)); break;
				}
				break;
/*
			case DBA_VAR(0, 31,  1):
		    {
				int size;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &size));
				DBA_RUN_OR_RETURN(dba_msg_sounding_resize_obs(so, size));
				obs = -1;
				break;
			}
*/
			case DBA_VAR(0,  7,  4):
				DBA_RUN_OR_RETURN(dba_var_enqd(var, &press));
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 4), 100, press, 0, 0, 0, 0));
				press_var = var;
				break;
			case DBA_VAR(0,  8,  1): {
				int vss;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &vss));
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0,  8, 1), 100, press, 0, 0, 0, 0));
				if (vss & 64)
				{
					surface_press = press;
					surface_press_var = press_var;
				}
				break;
			}
			case DBA_VAR(0, 10,  3): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 3), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 12,  1): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 1), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 12,  2): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 2), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 12,  3): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 3), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 11,  1): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 1), 100, press, 0, 0, 0, 0)); break;
			case DBA_VAR(0, 11,  2): DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 11, 2), 100, press, 0, 0, 0, 0)); break;
			/*
			default:
				fprintf(stderr, "Unhandled variable: "); dba_var_print(var, stderr);
				break;
			*/
		}
	}

	/* Extract surface data from the surface level */
	if (surface_press != -1)
	{
		dba_msg_datum d;
		dba_var v;
		
		/* Pressure is taken from a saved variable referencing to the original
		 * pressure data in the message, to preserve data attributes
		 */
		if (surface_press_var != NULL && dba_var_value(surface_press_var) != NULL
		    && (v = dba_msg_get_press_var(msg)) == NULL)
			DBA_RUN_OR_RETURN(dba_msg_set_press_var(msg, surface_press_var));

		if ((d = dba_msg_find(msg, DBA_VAR(0, 12, 1), 100, surface_press, 0, 0, 0, 0)) != NULL
		    && (v = dba_msg_get_temp_2m_var(msg)) == NULL)
			DBA_RUN_OR_RETURN(dba_msg_set_temp_2m_var(msg, d->var));
		if ((d = dba_msg_find(msg, DBA_VAR(0, 12, 3), 100, surface_press, 0, 0, 0, 0)) != NULL
		    && (v = dba_msg_get_dewpoint_2m_var(msg)) == NULL)
			DBA_RUN_OR_RETURN(dba_msg_set_dewpoint_2m_var(msg, d->var));
		if ((d = dba_msg_find(msg, DBA_VAR(0, 11, 1), 100, surface_press, 0, 0, 0, 0)) != NULL
		    && (v = dba_msg_get_wind_dir_var(msg)) == NULL)
			DBA_RUN_OR_RETURN(dba_msg_set_wind_dir_var(msg, d->var));
		if ((d = dba_msg_find(msg, DBA_VAR(0, 11, 2), 100, surface_press, 0, 0, 0, 0)) != NULL
		    && (v = dba_msg_get_wind_speed_var(msg)) == NULL)
			DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, d->var));
	}

	return dba_error_ok();
}
/* vim:set ts=4 sw=4: */
