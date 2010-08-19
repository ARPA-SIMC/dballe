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

#include "common.h"

dba_err aof_read_temp(const uint32_t* obs, int obs_len, dba_msg msg)
{
	int nlev = (obs_len - 20) / 8;
	int i;

	/* DBA_RUN_OR_RETURN(dba_msg_sounding_resize_obs(msg, nlev)); */

	/* 07 Code type */
	switch (OBS(7))
	{
		case 35:
			msg->type = MSG_TEMP;
			DBA_RUN_OR_RETURN(dba_aof_parse_st_block_station(msg, obs));
			DBA_RUN_OR_RETURN(dba_aof_parse_altitude(msg, obs));
			break;
		case 36:
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
		int os = 20 + i*8;
		// Pressure
		double press = (double)OBS(os + 0) * 10;
		int vss = (OBS(os + 6) >> 12) & 0x1ff;

		if (OBS(os + 0) == AOF_UNDEF)
			return dba_error_consistency("pressure indication not found for level %d", i);
		else
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 4),
						OBS(os + 0) * 10, get_conf6(OBS(os + 6) & 0x3f),
						100, press, 0, 0, 254, 0, 0));
					
		// Vertical sounding significance
		DBA_RUN_OR_RETURN(dba_convert_AOFVSS_to_BUFR08001(vss, &vss));
		DBA_RUN_OR_RETURN(dba_msg_seti(msg, DBA_VAR(0, 8, 1), vss, -1, 100, press, 0, 0, 254, 0, 0));

		// Wind direction
		if (OBS(os + 1) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 1),
						OBS(os + 1), get_conf6((OBS(os + 6) >> 6) & 0x3f),
						100, press, 0, 0, 254, 0, 0));
		// Wind speed
		if (OBS(os + 2) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 2),
						OBS(os + 2), get_conf6(OBS(os + 7) & 0x3f),
						100, press, 0, 0, 254, 0, 0));
		// Air temperature
		if (OBS(os + 3) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12, 101),
						totemp(OBS(os + 3)), get_conf6((OBS(os + 7) >> 6) & 0x3f),
						100, press, 0, 0, 254, 0, 0));
		// Dew point temperature
		if (OBS(os + 4) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12, 103),
						totemp(OBS(os + 4)), get_conf6((OBS(os + 7) >> 12) & 0x3f),
						100, press, 0, 0, 254, 0, 0));
		// Height + 1000
		if (OBS(os + 5) != AOF_UNDEF)
		{
			// Convert height to geopotential
#if 0
			dba_msg_datum d;
			double dval;
			fprintf(stderr, "HEIGHT: %d -> %f\n", OBS(os + 5) - 1000, ((double)OBS(os + 5) - 1000)*9.80665);
#endif
			// Rounding the converted height->geopotential to preserve the
			// correct amount of significant digits
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 3),
						round(((double)OBS(os + 5) - 1000)*9.80665/10)*10, get_conf6((OBS(os + 7) >> 18) & 0x3f),
						100, press, 0, 0, 254, 0, 0));
#if 0
			d = dba_msg_find(msg, DBA_VAR(0, 10, 3), 100, press, 0, 0, 0, 0);
			if (d == NULL)
				fprintf(stderr, "NO, QUESTO NO!\n");
			DBA_RUN_OR_RETURN(dba_var_enqd(d->var, &dval));
			fprintf(stderr, "HEIGHT: %d -> %f (was %f)\n", OBS(os + 5) - 1000, dval, round(((double)OBS(os + 5) - 1000)*9.80665/10)*10);
#endif
		}
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
