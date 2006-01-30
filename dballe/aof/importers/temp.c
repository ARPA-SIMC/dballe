#include "common.h"

dba_err aof_read_temp(const uint32_t* obs, int obs_len, dba_msg* out)
{
	dba_msg msg;
	int nlev = (obs_len - 20) / 8;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&msg));
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
		double press = (double)OBS(os + 0) * 10;
		int vss = (OBS(os + 6) >> 12) & 0x1ff;

		if (OBS(os + 0) == AOF_UNDEF)
			return dba_error_consistency("pressure indication not found for level %d", i);
					
		DBA_RUN_OR_RETURN(dba_convert_AOFVSS_to_BUFR08001(vss, &vss));
		DBA_RUN_OR_RETURN(dba_msg_seti(msg, DBA_VAR(0, 8, 1), vss, -1, 100, press, 0, 0, 0, 0));
		/* DBA_RUN_OR_RETURN(dba_var_setd(msg->obs[i].var_press, (double)OBS(os + 0) * 10)); */

		if (OBS(os + 1) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 1), OBS(os + 1), -1, 100, press, 0, 0, 0, 0));
		if (OBS(os + 2) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 11, 2), OBS(os + 2), -1, 100, press, 0, 0, 0, 0));
		if (OBS(os + 3) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12, 1), totemp(OBS(os + 3)), -1, 100, press, 0, 0, 0, 0));
		if (OBS(os + 4) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 12, 2), totemp(OBS(os + 4)), -1, 100, press, 0, 0, 0, 0));
		if (OBS(os + 5) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_setd(msg, DBA_VAR(0, 10, 3), ((double)OBS(os + 5) - 1000)*9.8, -1, 100, press, 0, 0, 0, 0));
	}

	*out = msg;
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
