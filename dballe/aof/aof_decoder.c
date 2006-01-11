#include <config.h>

#include <dballe/aof/aof_decoder.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/io/dba_rawfile.h>

#include <stdint.h>

#include <assert.h>

// #define TRACE_DECODER

#ifdef TRACE_DECODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

#define AOF_UNDEF 0x7fffffff

extern dba_err aof_read_synop(const uint32_t* obs, int obs_len, dba_msg* out);
extern dba_err aof_read_flight(const uint32_t* obs, int obs_len, dba_msg* out);
extern dba_err aof_read_satob(const uint32_t* obs, int obs_len, dba_msg* out);
extern dba_err aof_read_dribu(const uint32_t* obs, int obs_len, dba_msg* out);
extern dba_err aof_read_temp(const uint32_t* obs, int obs_len, dba_msg* out);
extern dba_err aof_read_pilot(const uint32_t* obs, int obs_len, dba_msg* out);
extern dba_err aof_read_satem(const uint32_t* obs, int obs_len, dba_msg* out);

dba_err aof_decoder_get_category(dba_rawmsg msg, int* category, int* subcategory)
{
	const unsigned char* buf;
	const uint32_t* obs;
	int obs_len;

	/* Access the raw data in a more comfortable form */
	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(msg, &buf, &obs_len));
	obs = (const uint32_t*)buf;
	obs_len /= sizeof(uint32_t);

	if (obs_len < 7)
		return dba_error_parse(msg->file->name, msg->offset,
				"the buffer is too short to contain an AOF message");

	*category = obs[5];
	*subcategory = obs[6];

	return dba_error_ok();
}

#define OBS(n) (obs[n-1])

dba_err aof_decoder_decode(dba_rawmsg msg, dba_msg* out)
{
	/* char id[10]; */
	const unsigned char* buf;
	const uint32_t* obs;
	int obs_len;

	assert(msg != NULL);

	TRACE("aof_message_decode\n");

	/* Access the raw data in a more comfortable form */
	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(msg, &buf, &obs_len));
	obs = (const uint32_t*)buf;
	obs_len /= sizeof(uint32_t);

	TRACE("05 grid box number: %d\n", OBS(5));
	TRACE("obs type: %d, %d\n", OBS(6), OBS(7));

#if 0
	/* 13 Station ID (1:4) */
	/* 14 Station ID (5:8) */
	/* B01011 [CHARACTER] SHIP OR MOBILE LAND STATION IDENTIFIER */
	parse_station_id(msg, id);
	TRACE("ID: %s\n", id);
	for (i = 0; i < 8 && isspace(id[i]); i++)
		/* Skip leading spaces */ ;
	DBA_RUN_OR_RETURN(aof_message_store_variable_c(msg, DBA_VAR(0,  1,  11), id + i));
#endif

	/* 06 Observation type */
	/* 07 Code type */
	switch (OBS(6))
	{
		case 1: DBA_RUN_OR_RETURN(aof_read_synop(obs, obs_len, out)); break;
		case 2: DBA_RUN_OR_RETURN(aof_read_flight(obs, obs_len, out)); break;
		case 3: DBA_RUN_OR_RETURN(aof_read_satob(obs, obs_len, out)); break;
		case 4: DBA_RUN_OR_RETURN(aof_read_dribu(obs, obs_len, out)); break;
		case 5: DBA_RUN_OR_RETURN(aof_read_temp(obs, obs_len, out)); break;
		case 6: DBA_RUN_OR_RETURN(aof_read_pilot(obs, obs_len, out)); break;
		case 7: DBA_RUN_OR_RETURN(aof_read_satem(obs, obs_len, out)); break;
		default:
			return dba_error_parse(msg->file->name, msg->offset,
					"cannot handle AOF observation type %d subtype %d",
					OBS(5), OBS(6));
	}

	return dba_error_ok();
}


dba_err aof_read_satob(const uint32_t* obs, int obs_len, dba_msg* out)
{
	return dba_error_unimplemented("parsing AOF SATOB observations");
//	*out = NULL;
//	return dba_error_ok();
}

dba_err aof_read_satem(const uint32_t* obs, int obs_len, dba_msg* out)
{
	return dba_error_unimplemented("parsing AOF SATEM observations");
//	*out = NULL;
//	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
