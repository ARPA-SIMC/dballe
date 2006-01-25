#include "common.h"

dba_err aof_read_pilot(const uint32_t* obs, int obs_len, dba_msg* out)
{
	dba_msg msg;
	int nlev = (obs_len - 20) / 7;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&msg));
	/* DBA_RUN_OR_RETURN(dba_msg_sounding_resize_obs(msg, nlev)); */

	/* 07 Code type */
	switch (OBS(7))
	{
		case 32:
			msg->type = MSG_TEMP;
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
		int ltype, l1;
		DBA_RUN_OR_RETURN(dba_convert_AOFVSS_to_BUFR08001(vss, &vss));

		/* Use the pressure if defined, else use the geopotential */
		if (OBS(os + 0) != AOF_UNDEF)
		{
			ltype = 100;
			l1 = OBS(os + 0) * 10;
			/* Pressure */
			/*	DBA_RUN_OR_RETURN(dba_msg_set_flight_press(msg, ((double)OBS(os + 0) * 10), get_conf6(OBS(os+5) & 0x3f))); */
		} else if (OBS(os + 4) != AOF_UNDEF) {
			ltype = 103;
			l1 = OBS(os + 4) - 1000;

			/* Height */
			/*	DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 3), ((double)OBS(os + 4) - 1000)*9.8, get_conf6((OBS(os+6) >> 12) & 0x3f), ltype, l1, 0, 0, 0, 0)); */
		} else {
			return dba_error_notfound("looking for pressure or height in an AOF PILOT message");
		}

		DBA_RUN_OR_RETURN(dba_msg_seti(msg, DBA_VAR(0, 8, 1), vss, -1, ltype, l1, 0, 0, 0, 0));
		/* DBA_RUN_OR_RETURN(dba_var_setd(msg->obs[i].var_press, (double)OBS(os + 0) * 10)); */

		/* Wind direction */
		if (OBS(os + 1) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 1),
						OBS(os + 1), get_conf6((OBS(os+5) >> 6) & 0x3f),
						ltype, l1, 0, 0, 0, 0));
		/* Wind speed */
		if (OBS(os + 2) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 2),
						OBS(os + 2), get_conf6(OBS(os+6) & 0x3f),
						ltype, l1, 0, 0, 0, 0));
		/* Air temperature */
		if (OBS(os + 3) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12, 1),
						k2c(OBS(os + 3)), get_conf6((OBS(os+6) >> 6) & 0x3f),
						ltype, l1, 0, 0, 0, 0));
	}

	*out = msg;
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
