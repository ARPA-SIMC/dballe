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

#include "dballe/msg/msg.h"
#include "dballe/bufrex/msg.h"

#define MISSING_BARO -10000
#define MISSING_PRESS_STD 0.0
#define MISSING_SENSOR_H -10000
#define MISSING_VSS 63
#define MISSING_TIME_PERIOD -10000

dba_err bufrex_copy_to_synop(dba_msg msg, bufrex_msg raw, bufrex_subset sset)
{
	int i, j;
	double height_baro = MISSING_BARO;
	double press_std = MISSING_PRESS_STD;
	double height_sensor = MISSING_SENSOR_H;
	int vs = MISSING_VSS;
	int time_period = MISSING_TIME_PERIOD;
	dba_varcode last_var1 = 0;
	dba_varcode last_var2 = 0;
	int cloudleveltype = 0;
	int cloudl1 = -1;

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
		int processed = 1;
		dba_var var = sset->vars[i];

		switch (dba_var_code(var))
		{
/* Context items */
			case DBA_VAR(0,  7, 32):
				/* Height to use later as layer for what needs it */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqd(var, &height_sensor));
				else
					height_sensor = MISSING_SENSOR_H;
				break;
			case DBA_VAR(0,  8,  2): {
				/* Vertical significance */
				dba_varcode prev, next;
				if (i == 0) return dba_error_consistency("B08002 found at beginning of message");
				if (i == sset->vars_count-1) return dba_error_consistency("B08002 found at end of message");
				prev = dba_var_code(sset->vars[i-1]);
				next = dba_var_code(sset->vars[i+1]);

				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqi(var, &vs));
				else
					vs = MISSING_VSS;

				if (prev == DBA_VAR(0, 20, 10))
				{
					/* Cloud Data */
					cloudleveltype = 258;
					cloudl1 = 0;
				} else if (next == DBA_VAR(0, 20, 11)) {
					/* Individual cloud group and clouds with bases below */
					if (cloudleveltype != 259)
					{
						cloudleveltype = 259;
						cloudl1 = 1;
					} else {
						++cloudl1;
					}
				} else if (next == DBA_VAR(0, 20, 54)) {
					/* Direction of cloud drift */
					if (cloudleveltype != 260)
					{
						cloudleveltype = 260;
						cloudl1 = 1;
					} else {
						++cloudl1;
					}
				} else if (dba_var_value(var) == NULL) {
					cloudleveltype = 0;
				} else
					return dba_error_consistency("Vertical significance %d found in unrecognised context", vs);

				/* Store original VS value as a measured value */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 4),
								256, 0, cloudleveltype, cloudl1,
								254, 0, 0));
				break;
			}
			case DBA_VAR(0,  4, 24):
				/* Time period in hours */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqi(var, &time_period));
				else
					vs = MISSING_TIME_PERIOD;
				break;
			default:
				processed = 0;
				break;
		}

		if (processed || dba_var_value(var) == NULL)
			continue;

		switch (dba_var_code(var))
		{
/* Context items */
			case DBA_VAR(0,  7, 32):
				/* Remember the height to use later as layer for what needs it */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqd(var, &height_sensor));
				else
					height_sensor = MISSING_SENSOR_H;
				break;
			case DBA_VAR(0,  8, 2):
				/* Remember the vertical significance */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqi(var, &vs));
				else
					vs = MISSING_VSS;
				break;


/* Fixed surface station identification, time, horizontal and vertical
 * coordinates (complete) */
			case DBA_VAR(0,  1,  1): DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg, var)); break;
			case DBA_VAR(0,  1,  2): DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg, var)); break;
			case DBA_VAR(0,  1,  5):
			case DBA_VAR(0,  1, 11): DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg, var)); break;
			case DBA_VAR(0,  1, 12): DBA_RUN_OR_RETURN(dba_msg_set_st_dir_var(msg, var)); break;
			case DBA_VAR(0,  1, 13): DBA_RUN_OR_RETURN(dba_msg_set_st_speed_var(msg, var)); break;
			case DBA_VAR(0,  2,  1): DBA_RUN_OR_RETURN(dba_msg_set_st_type_var(msg, var)); break;
			case DBA_VAR(0,  1, 15): DBA_RUN_OR_RETURN(dba_msg_set_st_name_var(msg, var)); break;
			case DBA_VAR(0,  4,  1): DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg, var)); break;
			case DBA_VAR(0,  4,  2): DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg, var)); break;
			case DBA_VAR(0,  4,  3): DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg, var)); break;
			case DBA_VAR(0,  4,  4): DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg, var)); break;
			case DBA_VAR(0,  4,  5): DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg, var)); break;
			case DBA_VAR(0,  5,  1):
			case DBA_VAR(0,  5,  2): DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg, var)); break;
			case DBA_VAR(0,  6,  1):
			case DBA_VAR(0,  6,  2): DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg, var)); break;
			case DBA_VAR(0,  7,  1):
			case DBA_VAR(0,  7, 30): DBA_RUN_OR_RETURN(dba_msg_set_height_var(msg, var)); break;
			/* case DBA_VAR(0,  7,  4): DBA_RUN_OR_RETURN(dba_msg_set_isobaric_surface_var(msg, var)); break; */
			case DBA_VAR(0,  7, 31):
				/* Remember the height to use later as layer for pressure */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqd(var, &height_baro));
				DBA_RUN_OR_RETURN(dba_msg_set_height_baro_var(msg, var));
				break;

/* Pressure data (complete) */
			case DBA_VAR(0, 10,  4):
				if (height_baro == MISSING_BARO)
					DBA_RUN_OR_RETURN(dba_msg_set_press_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 4),
								102, height_baro*1000, 0, 0,
								254, 0, 0));
				break;
			case DBA_VAR(0, 10, 51):
				if (height_baro == MISSING_BARO)
					DBA_RUN_OR_RETURN(dba_msg_set_press_msl_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 51),
								102, height_baro*1000, 0, 0,
								254, 0, 0));
				break;
			case DBA_VAR(0, 10, 61):
				if (height_baro == MISSING_BARO)
					DBA_RUN_OR_RETURN(dba_msg_set_press_3h_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 60),
								102, height_baro*1000, 0, 0,
								4, 0, 10800));
				break;
			case DBA_VAR(0, 10, 62):
				if (height_baro == MISSING_BARO)
					DBA_RUN_OR_RETURN(dba_msg_set_press_24h_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 60),
								102, height_baro*1000, 0, 0,
								4, 0, 86400));
				break;
			case DBA_VAR(0, 10, 63):
				if (height_baro == MISSING_BARO)
					DBA_RUN_OR_RETURN(dba_msg_set_press_tend_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10, 63),
								102, height_baro*1000, 0, 0,
								205, 0, 10800));
				break;	
			case DBA_VAR(0,  7,  4):
				/* Remember the standard level pressure to use later as layer for geopotential */
				if (dba_var_value(var) != NULL)
					DBA_RUN_OR_RETURN(dba_var_enqd(var, &press_std));
				/* DBA_RUN_OR_RETURN(dba_msg_set_height_baro_var(msg, var)); */
				break;
			case DBA_VAR(0, 10,  9):
				if (press_std == MISSING_PRESS_STD)
					return dba_error_consistency("B10009 given without pressure of standard level");
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 10,  9),
							100, press_std, 0, 0,
							254, 0, 0));
				break;

			/* Legacy bits */
			case DBA_VAR(0, 10,  3): DBA_RUN_OR_RETURN(dba_msg_set_geopotential_var(msg, var)); break;

/* Basic synoptic "instantaneous" data */

/* Temperature and humidity data (complete) */
			case DBA_VAR(0, 12,   4):
			case DBA_VAR(0, 12, 101):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_temp_2m_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 101),
								103, height_sensor * 1000, 0, 0,
								254, 0, 0));
				break;
			case DBA_VAR(0, 12,  6):
			case DBA_VAR(0, 12, 103):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_dewpoint_2m_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 103),
								103, height_sensor * 1000, 0, 0,
								254, 0, 0));
				break;
			case DBA_VAR(0, 13,  3):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_humidity_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 3),
								103, height_sensor * 1000, 0, 0,
								254, 0, 0));
				break;

/* Visibility data (complete) */
			case DBA_VAR(0, 20,  1):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_visibility_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 20, 1),
								103, height_sensor * 1000, 0, 0,
								254, 0, 0));
				break;

/* Precipitation past 24h (complete) */
			case DBA_VAR(0, 13, 19):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_tot_prec1_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
								103, height_sensor * 1000, 0, 0,
								1, 0, 3600));
				break;
			case DBA_VAR(0, 13, 20):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_tot_prec3_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
								103, height_sensor * 1000, 0, 0,
								1, 0, 10800));
				break;
			case DBA_VAR(0, 13, 21):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_tot_prec6_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
								103, height_sensor * 1000, 0, 0,
								1, 0, 21600));
				break;
			case DBA_VAR(0, 13, 22):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_tot_prec12_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
								103, height_sensor * 1000, 0, 0,
								1, 0, 43200));
				break;
			case DBA_VAR(0, 13, 23):
				if (height_sensor == MISSING_SENSOR_H)
					DBA_RUN_OR_RETURN(dba_msg_set_tot_prec24_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
								103, height_sensor * 1000, 0, 0,
								1, 0, 86400));
				break;

/* Cloud data */
			case DBA_VAR(0, 20, 10): DBA_RUN_OR_RETURN(dba_msg_set_cloud_n_var(msg, var)); break;

/* Individual cloud layers or masses */
/* Clouds with bases below station level */
/* Direction of cloud drift */
			case DBA_VAR(0, 20, 11):
			case DBA_VAR(0, 20, 13):
			case DBA_VAR(0, 20, 17):
			case DBA_VAR(0, 20, 54):
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, dba_var_code(var),
							256, 0, cloudleveltype, cloudl1,
							254, 0, 0));
				break;
			case DBA_VAR(0, 20, 12): {
				int lt2 = cloudleveltype, l2=cloudl1;
				if (lt2 == 258)
				{
					l2 = 1;
					if (i > 0 && dba_var_code(sset->vars[i-1]) == DBA_VAR(0, 20, 12))
					{
						++l2;
						if (i > 1 && dba_var_code(sset->vars[i-2]) == DBA_VAR(0, 20, 12))
							++l2;
					}
				}
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, dba_var_code(var),
							256, 0, lt2, l2,
							254, 0, 0));
				break;
			}

/* Direction and elevation of cloud */
/* TODO: beware it contains another B20012 */
/* TODO: what level? */

/* State of ground, snow depth, ground minimum temperature (complete) */
			case DBA_VAR(0, 20,  62): DBA_RUN_OR_RETURN(dba_msg_set_state_ground_var(msg, var)); break;
			case DBA_VAR(0, 13,  13): DBA_RUN_OR_RETURN(dba_msg_set_tot_snow_var(msg, var)); break;
			case DBA_VAR(0, 12, 113):
				DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 12, 121),
							1, 0, 0, 0,
							3, 0, 43200));
						  break;

/* Basic synoptic "period" data */

/* Present and past weather (complete) */
			case DBA_VAR(0, 20,  3): DBA_RUN_OR_RETURN(dba_msg_set_pres_wtr_var(msg, var)); break;

			case DBA_VAR(0, 20,  4):
				if (time_period == MISSING_TIME_PERIOD)
					DBA_RUN_OR_RETURN(dba_msg_set_past_wtr1_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 20, 4),
								1, 0, 0, 0,
								205, 0, time_period * 3600));
				break;
			case DBA_VAR(0, 20,  5):
				if (time_period == MISSING_TIME_PERIOD)
					DBA_RUN_OR_RETURN(dba_msg_set_past_wtr2_var(msg, var));
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 20, 5),
								1, 0, 0, 0,
								205, 0, time_period * 3600));
				break;

/* Sunshine data (complete) */
			case DBA_VAR(0, 14, 31):
				if (time_period == MISSING_TIME_PERIOD)
					return dba_error_consistency("total sunshine B14031 given without time period indication");
				else
					DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 14, 31),
								1, 0, 0, 0,
								1, 0, time_period * 3600));
				break;

/* Precipitation measurement (complete) */
			case DBA_VAR(0, 13, 11):
				if (time_period == MISSING_TIME_PERIOD)
					return dba_error_consistency("total precipitation B13011 given without time period indication");
				else {
					if (height_sensor == MISSING_SENSOR_H)
						DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
									1, 0, 0, 0,
									1, 0, time_period * 3600));
					else
						DBA_RUN_OR_RETURN(dba_msg_set(msg, var, DBA_VAR(0, 13, 11),
									103, height_sensor * 1000, 0, 0,
									1, 0, time_period * 3600));
				}
				break;

/* Extreme temperature data */

/* Wind data */

/* Evaporation data */

/* Radiation data */

/* Temperature change */

			case DBA_VAR(0, 11, 11): DBA_RUN_OR_RETURN(dba_msg_set_wind_dir_var(msg, var)); break;
			case DBA_VAR(0, 11, 12): DBA_RUN_OR_RETURN(dba_msg_set_wind_speed_var(msg, var)); break;
			case DBA_VAR(0, 22, 42): DBA_RUN_OR_RETURN(dba_msg_set_water_temp_var(msg, var)); break;
			case DBA_VAR(0, 12,  5): DBA_RUN_OR_RETURN(dba_msg_set_wet_temp_2m_var(msg, var)); break;
			case DBA_VAR(0, 10,197): DBA_RUN_OR_RETURN(dba_msg_set_height_anem_var(msg, var)); break;
		}
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
