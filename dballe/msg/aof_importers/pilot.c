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

dba_err aof_read_pilot(const uint32_t* obs, int obs_len, dba_msg msg)
{
	int nlev = (obs_len - 20) / 7;
	int i;

	/* DBA_RUN_OR_RETURN(dba_msg_sounding_resize_obs(msg, nlev)); */

	/* 07 Code type */
	switch (OBS(7))
	{
		case 32:
			msg->type = MSG_PILOT;
			DBA_RUN_OR_RETURN(dba_aof_parse_st_block_station(msg, obs));
			DBA_RUN_OR_RETURN(dba_aof_parse_altitude(msg, obs));
			break;
		case 33:
			msg->type = MSG_TEMP_SHIP;
			DBA_RUN_OR_RETURN(dba_aof_parse_st_ident(msg, obs));
			break;
		default:
			msg->type = MSG_GENERIC;
			DBA_RUN_OR_RETURN(dba_aof_parse_st_ident(msg, obs));
			break;
	}

	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_dir, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_speed, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_type, 0));

	/* 08 Latitude */
	/* 09 Longitude */
	/* 10 Observation date */
	/* 12 Exact time of observation */
	DBA_RUN_OR_RETURN(dba_aof_parse_lat_lon_datetime(msg, obs));
	
	for (i = 0; i < nlev; i++)
	{
		int os = 20 + i*7;
		int vss = (OBS(os + 5) >> 12) & 0x1ff;
		int ltype = -1, l1 = -1;
		DBA_RUN_OR_RETURN(dba_convert_AOFVSS_to_BUFR08001(vss, &vss));

		/* Use the pressure if defined, else use the geopotential */
		if (OBS(os + 0) != AOF_UNDEF)
		{
			ltype = 100;
			l1 = OBS(os + 0) * 10;
			/* Pressure */
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 4),
						((double)OBS(os + 0) * 10), get_conf6(OBS(os+5) & 0x3f),
						ltype, l1, 0, 0, 254, 0, 0));
		}
		if (OBS(os + 4) != AOF_UNDEF) {
			if (ltype == -1)
			{
				ltype = 102;
				l1 = OBS(os + 4) - 1000;
#if 0
				/* Save ICAO pressure instead of geopotential.  Tried but
				 * doesn't work (BUFR will reload with pressure levels instead
				 * of height levels */
				DBA_RUN_OR_RETURN(dba_convert_icao_to_press(l1, &press));
				DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 4),
							press, get_conf6(OBS(os+5) & 0x3f),
							ltype, l1, 0, 0, 0, 0));
#endif
			}

			/* Geopotential */
			// Rounding the converted height->geopotential to preserve the
			// correct amount of significant digits
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 3),
						round(((double)OBS(os + 4) - 1000.0) * 9.80665 / 10) * 10, get_conf6((OBS(os+6) >> 12) & 0x3f),
						ltype, l1, 0, 0, 254, 0, 0));
		}

		if (ltype == -1)
			return dba_error_notfound("looking for pressure or height in an AOF PILOT message");

		DBA_RUN_OR_RETURN(dba_msg_seti(msg, DBA_VAR(0, 8, 1), vss, -1, ltype, l1, 0, 0, 254, 0, 0));
		/* DBA_RUN_OR_RETURN(dba_var_setd(msg->obs[i].var_press, (double)OBS(os + 0) * 10)); */

		/* Wind direction */
		if (OBS(os + 1) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 1),
						OBS(os + 1), get_conf6((OBS(os+5) >> 6) & 0x3f),
						ltype, l1, 0, 0, 254, 0, 0));
		/* Wind speed */
		if (OBS(os + 2) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 2),
						OBS(os + 2), get_conf6(OBS(os+6) & 0x3f),
						ltype, l1, 0, 0, 254, 0, 0));
		/* Air temperature */
		if (OBS(os + 3) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12, 1),
						totemp(OBS(os + 3)), get_conf6((OBS(os+6) >> 6) & 0x3f),
						ltype, l1, 0, 0, 254, 0, 0));
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
