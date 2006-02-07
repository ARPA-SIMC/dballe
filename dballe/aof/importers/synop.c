#include "common.h"

dba_err aof_read_synop(const uint32_t* obs, int obs_len, dba_msg* out)
{
	dba_msg msg;
	int prcode;
	int i;

	DBA_RUN_OR_RETURN(dba_msg_create(&msg));

	/* 07 Code type */
	if (OBS(7) < 20)
	{
		msg->type = MSG_SYNOP;

		/* 13,14 station ID */
		DBA_RUN_OR_RETURN(dba_aof_parse_st_block_station(msg, obs));

		/* 16 station altitude */
		DBA_RUN_OR_RETURN(dba_aof_parse_altitude(msg, obs));
	} else {
		msg->type = MSG_SHIP;

		/*
		DBA_RUN_OR_RETURN(dba_var_seti(ship->var_st_dir, 0));
		DBA_RUN_OR_RETURN(dba_var_seti(ship->var_st_speed, 0));
		*/

		/* 13,14 station ID */
		DBA_RUN_OR_RETURN(dba_aof_parse_st_ident(msg, obs));
	}

	/* 07 Code type */
	switch (OBS(7))
	{
		case 11: DBA_RUN_OR_RETURN(dba_msg_set_st_type(msg, 1, -1)); break;
		case 14: DBA_RUN_OR_RETURN(dba_msg_set_st_type(msg, 0, -1)); break;
		case 21:
		case 22:
		case 23: DBA_RUN_OR_RETURN(dba_msg_set_st_type(msg, 1, -1)); break;
		case 24: DBA_RUN_OR_RETURN(dba_msg_set_st_type(msg, 0, -1)); break;
	}

	/* 08 Latitude */
	/* 09 Longitude */
	/* 10 Observation date */
	/* 12 Exact time of observation */
	DBA_RUN_OR_RETURN(dba_aof_parse_lat_lon_datetime(msg, obs));

	/* 20 Pressure [1/10 hPa] or geopotential [m] depending on field #29
	 *
	 * Values of the pressure code in word 29:
	 *   0 - sea level
	 *   1 - station level pressure
	 *   2 - 850mb level geopotential
	 *   3 - 700mb level geopotential
	 *   4 - 500gpm  level pressure
	 *   5 - 1000gpm level pressure
	 *   6 - 2000gpm level pressure
	 *   7 - 3000gpm level pressure
	 *   8 - 4000gpm level pressure
	 *   9 - 900mb level geopotential
	 *  10 - 1000mb level geopotential
	 *  11 - 500mb level geopotential
	 */
	prcode = (OBS(29) >> 6) & 0xf;
	if (prcode == 0)
	{
		if (OBS(20) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_set_press_msl(msg, (double)OBS(20)*10.0, get_conf6(OBS(29) & 0x3f)));
	} else if (prcode == 1) {
		if (OBS(20) != AOF_UNDEF)
			DBA_RUN_OR_RETURN(dba_msg_set_press(msg, (double)OBS(20)*10.0, get_conf6(OBS(29) & 0x3f)));
	} else if (prcode >= 2) {
		/* TODO: synops don't really measure geopotential? */
		/* DBA_RUN_OR_RETURN(dba_var_setd(ship->var_(0, 10,  3), OBS(20))); */
	}

	if (OBS(21) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_wind_dir(msg, OBS(21), get_conf6(OBS(30) & 0x3f)));
	if (OBS(22) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_wind_speed(msg, OBS(22), get_conf6((OBS(30) >> 6) & 0x3f)));

	if (OBS(23) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_temp_2m(msg, totemp(OBS(23)), get_conf6((OBS(30) >> 12) & 0x3f)));
	if (OBS(24) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_dewpoint_2m(msg, totemp(OBS(24)), get_conf6((OBS(30) >> 18) & 0x3f)));

	/* Caracteristic of pressure tendency is not encoded in AOF 
	if (OBS(25) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_press_tend(msg, (double)((OBS(25) - 500) * 10), get_conf6(OBS(31) & 0x3f)));
	*/

	if (OBS(25) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_press_3h(msg, (((double)OBS(25) - 500.0) * 10.0), get_conf6(OBS(31) & 0x3f)));

	if (OBS(26) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_water_temp(msg, (double)OBS(26) / 10.0, get_conf6((OBS(31) >> 6) & 0x3f)));

	/* 27 Weather group word */
	DBA_RUN_OR_RETURN(dba_aof_parse_weather_group(msg, obs));

	/* 28 General cloud group */
	DBA_RUN_OR_RETURN(dba_aof_parse_general_cloud_group(msg, obs));

	/* Iterate among the optional groups */

	/* fprintf(stderr, "FLAGS: %x", OBS(32));
	dump_word(": ", OBS(32)); fprintf(stderr, "\n"); */

	i = 33;

	/* Iterate among the optional groups */
	if (OBS(32) & 0x1)
	{
		int n, c, h;
		uint32_t conf = dba_aof_get_extra_conf(obs, i - 33);
		DBA_RUN_OR_RETURN(dba_aof_parse_cloud_group(OBS(i), &n, &c, &h));

		DBA_RUN_OR_RETURN(dba_msg_set_cloud_n1(msg, n, get_conf2((conf >> 4) & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_c1(msg, c, get_conf2((conf >> 2) & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_h1(msg, h, get_conf2(conf & 0x3)));
		++i;
	}
	if (OBS(32) & 0x2)
	{
		int n, c, h;
		uint32_t conf = dba_aof_get_extra_conf(obs, i - 33);
		DBA_RUN_OR_RETURN(dba_aof_parse_cloud_group(OBS(i), &n, &c, &h));

		DBA_RUN_OR_RETURN(dba_msg_set_cloud_n2(msg, n, get_conf2(conf & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_c2(msg, c, get_conf2((conf >> 2) & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_h2(msg, h, get_conf2((conf >> 4) & 0x3)));
		++i;
	}
	if (OBS(32) & 0x4)
	{
		int n, c, h;
		uint32_t conf = dba_aof_get_extra_conf(obs, i - 33);
		DBA_RUN_OR_RETURN(dba_aof_parse_cloud_group(OBS(i), &n, &c, &h));

		DBA_RUN_OR_RETURN(dba_msg_set_cloud_n3(msg, n, get_conf2(conf & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_c3(msg, c, get_conf2((conf >> 2) & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_h3(msg, h, get_conf2((conf >> 4) & 0x3)));
		++i;
	}
	if (OBS(32) & 0x8)
	{
		int n, c, h;
		uint32_t conf = dba_aof_get_extra_conf(obs, i - 33);
		DBA_RUN_OR_RETURN(dba_aof_parse_cloud_group(OBS(i), &n, &c, &h));

		DBA_RUN_OR_RETURN(dba_msg_set_cloud_n4(msg, n, get_conf2(conf & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_c4(msg, c, get_conf2((conf >> 2) & 0x3)));
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_h4(msg, h, get_conf2((conf >> 4) & 0x3)));
		++i;
	}
	if (OBS(32) & 0x10) /* Ground group */
	{
		/* dba_aof_dump_word("Ground group: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x20) /* Special group 1 */
	{
		/* dba_aof_dump_word("Special group 1: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x40) /* Special group 2 */
	{
		/* dba_aof_dump_word("Special group 2: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x80) /* Ice group */
	{
		/* dba_aof_dump_word("Ice group: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x100) /* Rain group */
	{
		/* dba_aof_dump_word("Rain group: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x200) /* Ship group */
	{
		int speed = OBS(i) & 0xff;
		int dir = (OBS(i) >> 8) & 0x3ff;
		uint32_t conf = dba_aof_get_extra_conf(obs, i - 33);

		/* dump_word("Ship group: ", OBS(i)); fprintf(stderr, "\n"); */

		if (dir != 0x3ff)
			DBA_RUN_OR_RETURN(dba_msg_set_st_dir(msg, dir, get_conf2(conf & 0x3)));
		if (speed != 0xff)
			DBA_RUN_OR_RETURN(dba_msg_set_st_speed(msg, speed, get_conf2((conf >> 2) & 0x3)));

		i++;
	}
	if (OBS(32) & 0x400) /* Waves group */
	{
		/* dba_aof_dump_word("Waves group 1: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x800) /* Waves group */
	{
		/* dba_aof_dump_word("Waves group 2: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}
	if (OBS(32) & 0x1000) /* Waves group */
	{
		/* dba_aof_dump_word("Waves group 3: ", OBS(i)); fprintf(stderr, "\n"); */
		i++;
	}

#if 0
	*** Still unconverted
	DBA_RUN_OR_RETURN(dba_var_seti(synop->var_humidity, obs[xx]));
	//DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_ident, vars[8]));
	DBA_RUN_OR_RETURN(dba_var_copy_val(synop->var_press, vars[9]));
#endif

	*out = msg;

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
