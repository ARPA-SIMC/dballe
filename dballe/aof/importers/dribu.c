#include "common.h"

dba_err aof_read_dribu(const uint32_t* obs, int obs_len, dba_msg* out)
{
	dba_msg msg;

	DBA_RUN_OR_RETURN(dba_msg_create(&msg));
	msg->type = MSG_BUOY;

	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_dir, 0));
	//DBA_RUN_OR_RETURN(dba_var_seti(msg->var_st_speed, 0));
	DBA_RUN_OR_RETURN(dba_msg_set_st_type(msg, 0, -1)); 

	/* 08 Latitude */
	/* 09 Longitude */
	/* 10 Observation date */
	/* 12 Exact time of observation */
	DBA_RUN_OR_RETURN(dba_aof_parse_lat_lon_datetime(msg, obs));

	/* 13,14 station ID */
	DBA_RUN_OR_RETURN(dba_aof_parse_st_ident(msg, obs));

	if (OBS(20) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_press_msl(msg, (double)OBS(20) * 10, get_conf6(OBS(25))));

	if (OBS(21) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_wind_dir(msg, OBS(21), get_conf6(OBS(26) & 0x3f)));
	if (OBS(22) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_wind_speed(msg, OBS(22), get_conf6((OBS(26) >> 6) & 0x3f)));

	if (OBS(23) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_temp_2m(msg, k2c(OBS(23)), get_conf6((OBS(26) >> 12) & 0x3f)));

	if (OBS(24) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_water_temp(msg, (double)OBS(24) / 10.0, get_conf6((OBS(26) >> 18) & 0x3f)));

	*out = (dba_msg)msg;
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
