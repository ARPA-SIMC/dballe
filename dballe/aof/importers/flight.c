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

#include "common.h"

dba_err aof_read_flight(const uint32_t* obs, int obs_len, dba_msg* out)
{
	dba_msg msg;
	int ltype = -1, l1 = -1;

	DBA_RUN_OR_RETURN(dba_msg_create(&msg));

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

	if (OBS(20) != AOF_UNDEF)
	{
		double press = OBS(20) * 10.0;

		/* Save the pressure in the anagraphical layer only */
		/* FIXME: vedere se va memorizzato o no nell'anagrafica */
		DBA_RUN_OR_RETURN(dba_msg_set_flight_press(msg, press, get_conf6(OBS(25) & 0x3f)));

		/* Default to height for AMDAR */
		if (msg->type != MSG_AMDAR || (OBS(24) == AOF_UNDEF))
		{
			ltype = 100;
			l1 = press / 100;
			/* fprintf(stderr, "Press float: %f int: %d encoded: %s\n", press, l1, dba_var_value(dba_msg_get_flight_press_var(msg))); */
		}
	}
	if (OBS(24) != AOF_UNDEF)
	{
		/* FIXME: vedere se va memorizzato o no nell'anagrafica (può non aver
		 * senso se è calcolata dalla pressione) */
		DBA_RUN_OR_RETURN(dba_msg_set_height(msg, (double)OBS(24), get_conf6((OBS(26) >> 18) & 0x3f)));
		if (ltype == -1)
		{
			ltype = 103;
			l1 = OBS(24);
		}
	}
	if (ltype == -1)
		return dba_error_notfound("looking for pressure or height in an AOF flight message");

	/* File the measures at the right level */

	/* 21 Wind direction [degrees] */
	if (OBS(21) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11,  1), OBS(21), get_conf6(OBS(26) & 0x3f), ltype, l1, 0, 0, 0, 0));
	/* 22 Wind speed [m/s] */
	if (OBS(22) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11,  2), OBS(22), get_conf6((OBS(26) >> 6) & 0x3f), ltype, l1, 0, 0, 0, 0));
	/* 23 Air temperature [1/10 K] */
	if (OBS(23) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12,  1), totemp(OBS(23)), get_conf6((OBS(26) >> 12) & 0x3f), ltype, l1, 0, 0, 0, 0));

	*out = (dba_msg)msg;
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
