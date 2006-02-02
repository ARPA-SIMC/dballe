#include "common.h"

#include <time.h>		/* struct tm */
#include <stdint.h>		/* uint32_t */
#include <strings.h>	/* bzero */
#include <string.h>		/* strncpy */
#include <stdlib.h>		/* strtol */
#include <ctype.h>		/* isspace */

void dba_aof_dump_word(const char* prefix, uint32_t x)
{
	int i;
	fprintf(stderr, "%s", prefix);
	for (i = 0; i < 32; i++)
	{
		fprintf(stderr, "%c", (x & 0x80000000) != 0 ? '1' : '0');
		x <<= 1;
		if ((i+1) % 8 == 0)
			fprintf(stderr, " ");
	}
}

/* Set the confidence attribute of the given variable from the given 2-bit AOF
 * quality flag */
static inline dba_err set_confidence2(dba_var var, int conf)
{
	dba_err err = DBA_OK;
	dba_var attr = NULL;

	DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
	DBA_RUN_OR_GOTO(cleanup, dba_var_seti(attr, get_conf2(conf)));
	DBA_RUN_OR_GOTO(cleanup, dba_var_seta_nocopy(var, attr));
	attr = NULL;

cleanup:
	if (attr != NULL)
		dba_var_delete(attr);
	return err == DBA_OK ? dba_error_ok() : err;
}

/* Set the confidence attribute of the given variable from the given 6-bit AOF
 * quality flag */
static inline dba_err set_confidence6(dba_var var, int conf)
{
	return set_confidence2(var, conf >> 3);
}

static inline void parse_date(uint32_t val, struct tm* tm)
{
	tm->tm_mday = val % 100;
	tm->tm_mon = ((val / 100) % 100) - 1;
	tm->tm_year = (val / 10000) - 1900;
}

static inline const char* stationID(const uint32_t* obs)
{
	static char id[9];
	id[0] = (OBS(13) >> 21) & 0x7f;
	id[1] = (OBS(13) >> 14) & 0x7f;
	id[2] = (OBS(13) >> 7) & 0x7f;
	id[3] = OBS(13) & 0x7f;
	id[4] = (OBS(14) >> 21) & 0x7f;
	id[5] = (OBS(14) >> 14) & 0x7f;
	id[6] = (OBS(14) >> 7) & 0x7f;
	id[7] = OBS(14) & 0x7f;
	id[8] = 0;
	return id;
}

static inline void parse_obs_datetime(const uint32_t* obs, struct tm* t)
{
	bzero(t, sizeof (struct tm));
	/* 10 Observation date */
	parse_date(obs[9], t);
	/* 12 Exact time of observation */
	t->tm_hour = obs[11] / 100;
	t->tm_min = obs[11] % 100;
}

dba_err dba_aof_parse_lat_lon_datetime(dba_msg msg, const uint32_t* obs)
{
	struct tm datetime;

	/* 08 Latitude */
	DBA_RUN_OR_RETURN(dba_msg_set_latitude(msg, ((double)OBS(8) - 9000.0)/100.0, get_conf6(OBS(19) & 0x3f)));
	/* 09 Longitude */
	DBA_RUN_OR_RETURN(dba_msg_set_longitude(msg, ((double)OBS(9) - 18000.0)/100.0, get_conf6((OBS(19) >> 6) & 0x3f)));

	TRACE("11 Synoptic time: %d\n", OBS(11));
	/* 10 Observation date */
	/* 12 Exact time of observation */
	parse_obs_datetime(obs, &datetime);

	DBA_RUN_OR_RETURN(dba_msg_set_year(msg, datetime.tm_year + 1900, get_conf6((OBS(19) >> 12) & 0x3f)));
	DBA_RUN_OR_RETURN(dba_msg_set_month(msg, datetime.tm_mon + 1, get_conf6((OBS(19) >> 12) & 0x3f)));
	DBA_RUN_OR_RETURN(dba_msg_set_day(msg, datetime.tm_mday, get_conf6((OBS(19) >> 12) & 0x3f)));
	DBA_RUN_OR_RETURN(dba_msg_set_hour(msg, datetime.tm_hour, get_conf6((OBS(19) >> 18) & 0x3f)));
	DBA_RUN_OR_RETURN(dba_msg_set_minute(msg, datetime.tm_min, get_conf6((OBS(19) >> 18) & 0x3f)));

	return dba_error_ok();
}

dba_err dba_aof_parse_st_block_station(dba_msg msg, const uint32_t* obs)
{
	const char* id = stationID(obs);
	char block[4];

	strncpy(block, id + 3, 2);
	block[2] = 0;

	DBA_RUN_OR_RETURN(dba_msg_set_block(msg, strtol(block, 0, 10), -1));
	DBA_RUN_OR_RETURN(dba_msg_set_station(msg, strtol(id + 5, 0, 10), -1));

	return dba_error_ok();
}

dba_err dba_aof_parse_st_ident(dba_msg msg, const uint32_t* obs)
{
	/* 13,14 station ID */
	const char* id = stationID(obs);
	/* Trim leading spaces */
	while (*id && isspace(*id))
		id++;
	DBA_RUN_OR_RETURN(dba_msg_set_ident(msg, id, -1));
	return dba_error_ok();
}

dba_err dba_aof_parse_altitude(dba_msg msg, const uint32_t* obs)
{
	/* 16 station altitude */
	if (OBS(16) != AOF_UNDEF)
		DBA_RUN_OR_RETURN(dba_msg_set_height(msg, OBS(16) - 1000, get_conf6((OBS(19) >> 24) & 0x3f)));
	return dba_error_ok();
}

/* 27 Weather group word */
dba_err dba_aof_parse_weather_group(dba_msg msg, const uint32_t* obs)
{
	int _pastw = OBS(27) & 0x7f;
	int _presw = (OBS(27) >> 7) & 0x7f;
	int _visib = OBS(27) >> 14;

	/* dump_word("Weather: ", OBS(27)); fputc('\n', stderr); */

	/* B20001 HORIZONTAL VISIBILITY: 5000.000000 M */
	if (_visib != 0x1ffff)
		DBA_RUN_OR_RETURN(dba_msg_set_visibility(msg, (double)_visib, get_conf2(OBS(31) >> 16)));

	/* B20003 PRESENT WEATHER (SEE NOTE 1): 10.000000 CODE TABLE 20003 */
	if (_presw != 0x7f)
	{
		int val;
		DBA_RUN_OR_RETURN(dba_convert_WMO4677_to_BUFR20003(_presw, &val));
		DBA_RUN_OR_RETURN(dba_msg_set_pres_wtr(msg, val, get_conf2(OBS(31) >> 14)));
	}

	/* B20004 PAST WEATHER (1) (SEE NOTE 2): 2.000000 CODE TABLE 20004 */
	if (_pastw != 0x7f)
	{
		int val;
		DBA_RUN_OR_RETURN(dba_convert_WMO4561_to_BUFR20004(_pastw, &val));
		DBA_RUN_OR_RETURN(dba_msg_set_past_wtr1(msg, val, get_conf2(OBS(31) >> 12)));
	}
	
	return dba_error_ok();
}

/* 28 General cloud group */
dba_err dba_aof_parse_general_cloud_group(dba_msg msg, const uint32_t* obs)
{
	/* uint32_t v = OBS(28); */
	int ch = OBS(28) & 0xf;
	int cm = (OBS(28) >>  4) & 0xf;
	int h  = (OBS(28) >>  8) & 0x7ff;
	int cl = (OBS(28) >> 19) & 0xf;
	int nh = (OBS(28) >> 23) & 0xf;
	int n  = (OBS(28) >> 27) & 0xf;
	/*{
		int x = v, i;
		fprintf(stderr, "Clouds %x: ", v);
		for (i = 0; i < 32; i++)
		{
			fprintf(stderr, "%c", (x & 0x80000000) != 0 ? '1' : '0');
			x <<= 1;
		}
		fprintf(stderr, "\n");
	}*/

	/* B20012 CLOUD TYPE: 35.000000 CODE TABLE 20012 */
	if (ch != 0xf)
	{
		/* DBA_RUN_OR_RETURN(dba_convert_WMO0513_to_BUFR20012(ch, &val)); */
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_ch(msg, ch + 10, get_conf2(OBS(31) >> 18)));
	}

	/* B20012 CLOUD TYPE: 35.000000 CODE TABLE 20012 */
	if (cm != 0xf)
	{
		/* DBA_RUN_OR_RETURN(dba_convert_WMO0513_to_BUFR20012(cm, &val)) */;
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_cm(msg, cm + 20, get_conf2(OBS(31) >> 20)));
	}

	/* B20013 HEIGHT OF BASE OF CLOUD in  M */
	if (h != 0x7ff)
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_hh(msg, h * 10.0, get_conf2(OBS(31) >> 22)));

	/* B20012 CLOUD TYPE: 35.000000 CODE TABLE 20012 */
	if (cl != 0xf)
	{
		/* DBA_RUN_OR_RETURN(dba_convert_WMO0513_to_BUFR20012(cl, &val)) */;
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_cl(msg, cl + 30, get_conf2(OBS(31) >> 24)));
	}

	/* B20011 CLOUD AMOUNT in CODE TABLE 20011 */
	if (nh != 0xf)
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_nh(msg, nh, get_conf2(OBS(31) >> 26)));

	/* B20010 CLOUD COVER (TOTAL) in % */
	if (n != 0xf)
		DBA_RUN_OR_RETURN(dba_msg_set_cloud_n(msg, n * 100 / 8, get_conf2(OBS(31) >> 28)));

	return dba_error_ok();
}

/* Decode a bit-packed cloud group */
dba_err dba_aof_parse_cloud_group(uint32_t val, int* ns, int* c, int* h)
{
	*ns = (val >> 19) & 0xf;
	DBA_RUN_OR_RETURN(dba_convert_WMO0500_to_BUFR20012((val >> 15) & 0xf, c));
	*h = val & 0x7fff;
	return dba_error_ok();
}

uint32_t dba_aof_get_extra_conf(const uint32_t* obs, int idx)
{
	int count = count_bits(OBS(32));
	uint32_t w = OBS(33+count+idx/4);
	return (w >> ((idx % 4) * 6)) & 0x3f;

}

/* vim:set ts=4 sw=4: */
