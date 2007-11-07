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

#include "dballe/msg/msg.h"
#include "dballe/bufrex/msg.h"

dba_err bufrex_copy_to_synop(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	int i;
	int cloud_type_count = 0;
	int cloud_amt_count = 0;
	int cloud_height_count = 0;

	switch (raw->type)
	{
		case 0:
			msg->type = MSG_SYNOP;
			break;
		case 1:
			switch (raw->localsubtype)
			{
				case 21: msg->type = MSG_BUOY; break;
				case 9:
				case 11:
				case 13:
				case 19: msg->type = MSG_SHIP; break;
				case 0:
					/* Guess looking at the variables */
					if (sset->vars_count > 1 && dba_var_code(sset->vars[0]) == DBA_VAR(0, 1, 5))
						msg->type = MSG_BUOY;
					else
						msg->type = MSG_SHIP;
					break;
				default: msg->type = MSG_GENERIC; break;
			}
			break;
		default: msg->type = MSG_GENERIC; break;
	}
	
	for (i = 0; i < sset->vars_count; i++)
	{
		dba_var var = sset->vars[i];

		if (dba_var_value(var) == NULL)
		{
			switch (dba_var_code(var))
			{
				case DBA_VAR(0, 20, 11): cloud_amt_count++; break;
				case DBA_VAR(0, 20, 13): cloud_height_count++; break;
				case DBA_VAR(0, 20, 12): cloud_type_count++; break;
			}
			continue;
		}
		switch (dba_var_code(var))
		{
			case DBA_VAR(0,  1,  1): DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg, var)); break;
			case DBA_VAR(0,  1,  2): DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg, var)); break;
			case DBA_VAR(0,  1,  5):
			case DBA_VAR(0,  1, 11): DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg, var)); break;
			case DBA_VAR(0,  1, 12): DBA_RUN_OR_RETURN(dba_msg_set_st_dir_var(msg, var)); break;
			case DBA_VAR(0,  1, 13): DBA_RUN_OR_RETURN(dba_msg_set_st_speed_var(msg, var)); break;
			case DBA_VAR(0,  2,  1): DBA_RUN_OR_RETURN(dba_msg_set_st_type_var(msg, var)); break;
			case DBA_VAR(0,  4,  1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,  2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,  3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,  4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,  5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  5,  1):
			case DBA_VAR(0,  5,  2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  1):
			case DBA_VAR(0,  6,  2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  7,  1): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			case DBA_VAR(0,  7,  4): DBA_RUN_OR_RETURN(dba_msg_set_isobaric_surface_var(msg, var)); break;
			case DBA_VAR(0, 10,  3): DBA_RUN_OR_RETURN(dba_msg_set_geopotential_var(msg, var)); break;
			case DBA_VAR(0, 10,  4): DBA_RUN_OR_RETURN(dba_msg_set_press_var(msg, var)); break;
			case DBA_VAR(0, 10, 51): DBA_RUN_OR_RETURN(dba_msg_set_press_msl_var(msg, var)); break;
			case DBA_VAR(0, 10, 61): DBA_RUN_OR_RETURN(dba_msg_set_press_3h_var(msg, var)); break;
			case DBA_VAR(0, 10, 63): DBA_RUN_OR_RETURN(dba_msg_set_press_tend_var(msg, var)); break;
			case DBA_VAR(0, 11, 11): DBA_RUN_OR_RETURN(dba_msg_set_wind_dir_var(msg, var)); break;
			case DBA_VAR(0, 11, 12): DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, var)); break;
			case DBA_VAR(0, 12,  4): DBA_RUN_OR_RETURN(dba_msg_set_temp_2m_var(msg, var)); break;
			case DBA_VAR(0, 12,  6): DBA_RUN_OR_RETURN(dba_msg_set_dewpoint_2m_var(msg, var)); break;
			case DBA_VAR(0, 13,  3): DBA_RUN_OR_RETURN(dba_msg_set_humidity_var(msg, var)); break;
			case DBA_VAR(0, 20,  1): DBA_RUN_OR_RETURN(dba_msg_set_visibility_var(msg, var)); break;
			case DBA_VAR(0, 20,  3): DBA_RUN_OR_RETURN(dba_msg_set_pres_wtr_var(msg, var)); break;
			case DBA_VAR(0, 20,  4): DBA_RUN_OR_RETURN(dba_msg_set_past_wtr1_var(msg, var)); break;
			case DBA_VAR(0, 20,  5): DBA_RUN_OR_RETURN(dba_msg_set_past_wtr2_var(msg, var)); break;
			case DBA_VAR(0, 20, 10): DBA_RUN_OR_RETURN(dba_msg_set_cloud_n_var(msg, var)); break;
			case DBA_VAR(0, 20, 11):
				switch (cloud_amt_count++)
				{
					case 0: DBA_RUN_OR_RETURN(dba_msg_set_cloud_nh_var(msg, var)); break;
					case 1: DBA_RUN_OR_RETURN(dba_msg_set_cloud_n1_var(msg, var)); break;
					case 2: DBA_RUN_OR_RETURN(dba_msg_set_cloud_n2_var(msg, var)); break;
					case 3: DBA_RUN_OR_RETURN(dba_msg_set_cloud_n3_var(msg, var)); break;
					case 4: DBA_RUN_OR_RETURN(dba_msg_set_cloud_n4_var(msg, var)); break;
				}
				break;
			case DBA_VAR(0, 20, 13):
				switch (cloud_height_count++)
				{
					 case 0: DBA_RUN_OR_RETURN(dba_msg_set_cloud_hh_var(msg, var)); break;
					 case 1: DBA_RUN_OR_RETURN(dba_msg_set_cloud_h1_var(msg, var)); break;
					 case 2: DBA_RUN_OR_RETURN(dba_msg_set_cloud_h2_var(msg, var)); break;
					 case 3: DBA_RUN_OR_RETURN(dba_msg_set_cloud_h3_var(msg, var)); break;
					 case 4: DBA_RUN_OR_RETURN(dba_msg_set_cloud_h4_var(msg, var)); break;
				}
				break;
			case DBA_VAR(0, 20, 12):
			   switch (cloud_type_count++)
			   {
					case 0: DBA_RUN_OR_RETURN(dba_msg_set_cloud_cl_var(msg, var)); break;
					case 1: DBA_RUN_OR_RETURN(dba_msg_set_cloud_cm_var(msg, var)); break;
					case 2: DBA_RUN_OR_RETURN(dba_msg_set_cloud_ch_var(msg, var)); break;
					case 3: DBA_RUN_OR_RETURN(dba_msg_set_cloud_c1_var(msg, var)); break;
					case 4: DBA_RUN_OR_RETURN(dba_msg_set_cloud_c2_var(msg, var)); break;
					case 5: DBA_RUN_OR_RETURN(dba_msg_set_cloud_c3_var(msg, var)); break;
					case 6: DBA_RUN_OR_RETURN(dba_msg_set_cloud_c4_var(msg, var)); break;
			   }
			   break;
			case DBA_VAR(0, 13, 19): DBA_RUN_OR_RETURN(dba_msg_set_tot_prec1_var(msg, var)); break;
			case DBA_VAR(0, 13, 20): DBA_RUN_OR_RETURN(dba_msg_set_tot_prec3_var(msg, var)); break;
			case DBA_VAR(0, 13, 21): DBA_RUN_OR_RETURN(dba_msg_set_tot_prec6_var(msg, var)); break;
			case DBA_VAR(0, 13, 22): DBA_RUN_OR_RETURN(dba_msg_set_tot_prec12_var(msg, var)); break;
			case DBA_VAR(0, 13, 23): DBA_RUN_OR_RETURN(dba_msg_set_tot_prec24_var(msg, var)); break;
			case DBA_VAR(0, 13, 13): DBA_RUN_OR_RETURN(dba_msg_set_tot_snow_var(msg, var)); break;
			case DBA_VAR(0, 22, 42): DBA_RUN_OR_RETURN(dba_msg_set_water_temp_var(msg, var)); break;
			case DBA_VAR(0, 12,  5): DBA_RUN_OR_RETURN(dba_msg_set_wet_temp_2m_var(msg, var)); break;
			case DBA_VAR(0, 10,197): DBA_RUN_OR_RETURN(dba_msg_set_height_anem_var(msg, var)); break;
		}
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
