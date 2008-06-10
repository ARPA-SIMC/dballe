/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

dba_err aof_read_flight(const uint32_t* obs, int obs_len, dba_msg msg)
{
	int ltype = -1, l1 = -1;
	int is_fixed = 0;
	int flags_start = 25;

	/* If the length is 28, then it's a new style message with dew point */
	is_fixed = obs_len == 28;
	if (is_fixed) flags_start = 26;

	/* 07 Code type */
	switch (OBS(7))
	{
		/* case 244: ACAR */
		case 141: msg->type = MSG_AIREP; break;
		case 144: msg->type = MSG_AMDAR; break;
		case 244: msg->type = MSG_ACARS; break;
		case 41: /* CODAR */
		case 241: /* COLBA */
		default: msg->type = MSG_AIREP; break;
	}

	//DBA_RUN_OR_RETURN(dba_var_seti(ship->var_st_dir, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(ship->var_st_speed, 0));

	/* 08 Latitude */
	/* 09 Longitude */
	/* 10 Observation date */
	/* 12 Exact time of observation */
	DBA_RUN_OR_RETURN(dba_aof_parse_lat_lon_datetime(msg, obs));

	/* 13,14 station ID */
	DBA_RUN_OR_RETURN(dba_aof_parse_st_ident(msg, obs));
	
	/* Aircraft roll angle */
	{
		int angle = (OBS(17) >> 18) & 0xf;
		if (angle != 0x0f)
			DBA_RUN_OR_RETURN(dba_msg_set_flight_phase(msg, angle, -1));
	}

	/* 
	 * If both pressure and height are present, use height, as makeaof makes
	 * pressure a function of height
	 */
	if (OBS(24) != AOF_UNDEF)
	{
		/* Save the height in an analogous height layer.
		 * We cannot save in the ana layer because a flight can pass twice in
		 * the same point, at two different heights */
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0,  7,  1), (double)OBS(24), get_conf6((OBS(flags_start + 1) >> 18) & 0x3f),
			102, OBS(24), 0, 0, 254, 0, 0));
		if (ltype == -1)
		{
			ltype = 102;
			l1 = OBS(24);
		}
	} else if (OBS(20) != AOF_UNDEF) {
		double press = OBS(20) * 10.0;

		/* Save the pressure in an analogous pressure layer.
		 * We cannot save in the ana layer because a flight can pass twice in
		 * the same point, at two different pressures */
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10,  4), press, get_conf6(OBS(flags_start) & 0x3f),
			100, press, 0, 0, 254, 0, 0));

		/* Default to height for AMDAR */
		if (msg->type != MSG_AMDAR || (OBS(24) == AOF_UNDEF))
		{
			ltype = 100;
			l1 = press;
			/* fprintf(stderr, "Press float: %f int: %d encoded: %s\n", press, l1, dba_var_value(dba_msg_get_flight_press_var(msg))); */
		}
	}
	if (ltype == -1)
		return dba_error_notfound("looking for pressure or height in an AOF flight message");

	/* File the measures at the right level */

	/* 21 Wind direction [degrees] */
	if (OBS(21) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11,  1), OBS(21), get_conf6(OBS(flags_start + 1) & 0x3f), ltype, l1, 0, 0, 254, 0, 0));
	/* 22 Wind speed [m/s] */
	if (OBS(22) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11,  2), OBS(22), get_conf6((OBS(flags_start + 1) >> 6) & 0x3f), ltype, l1, 0, 0, 254, 0, 0));
	/* 23 Air temperature [1/10 K] */
	if (OBS(23) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12,  1), totemp(OBS(23)), get_conf6((OBS(flags_start + 1) >> 12) & 0x3f), ltype, l1, 0, 0, 254, 0, 0));

	if (is_fixed)
	{
		if (OBS(25) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12,  3), totemp(OBS(23)), get_conf6((OBS(flags_start + 1) >> 24) & 0x3f), ltype, l1, 0, 0, 254, 0, 0));
		
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
