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

#include <config.h>

#include "aof_codec.h"
#include "msg.h"
#include <dballe/core/rawfile.h>

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

extern dba_err aof_read_synop(const uint32_t* obs, int obs_len, dba_msg msg);
extern dba_err aof_read_flight(const uint32_t* obs, int obs_len, dba_msg msg);
extern dba_err aof_read_satob(const uint32_t* obs, int obs_len, dba_msg msg);
extern dba_err aof_read_dribu(const uint32_t* obs, int obs_len, dba_msg msg);
extern dba_err aof_read_temp(const uint32_t* obs, int obs_len, dba_msg msg);
extern dba_err aof_read_pilot(const uint32_t* obs, int obs_len, dba_msg msg);
extern dba_err aof_read_satem(const uint32_t* obs, int obs_len, dba_msg msg);

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

dba_err aof_decoder_decode(dba_rawmsg msg, dba_msgs* msgs)
{
	/* char id[10]; */
	dba_err err = DBA_OK;
	const unsigned char* buf;
	const uint32_t* obs;
	int obs_len;
	dba_msgs outs = NULL;
	dba_msg out = NULL;

	assert(msgs != NULL);

	TRACE("aof_message_decode\n");

	/* Access the raw data in a more comfortable form */
	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(msg, &buf, &obs_len));
	obs = (const uint32_t*)buf;
	obs_len /= sizeof(uint32_t);

	TRACE("05 grid box number: %d\n", OBS(5));
	TRACE("obs type: %d, %d\n", OBS(6), OBS(7));

	DBA_RUN_OR_RETURN(dba_msgs_create(&outs));
	DBA_RUN_OR_RETURN(dba_msg_create(&out));

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
		case 1: DBA_RUN_OR_GOTO(cleanup, aof_read_synop(obs, obs_len, out)); break;
		case 2: DBA_RUN_OR_GOTO(cleanup, aof_read_flight(obs, obs_len, out)); break;
		case 3: DBA_RUN_OR_GOTO(cleanup, aof_read_satob(obs, obs_len, out)); break;
		case 4: DBA_RUN_OR_GOTO(cleanup, aof_read_dribu(obs, obs_len, out)); break;
		case 5: DBA_RUN_OR_GOTO(cleanup, aof_read_temp(obs, obs_len, out)); break;
		case 6: DBA_RUN_OR_GOTO(cleanup, aof_read_pilot(obs, obs_len, out)); break;
		case 7: DBA_RUN_OR_GOTO(cleanup, aof_read_satem(obs, obs_len, out)); break;
		default:
			return dba_error_parse(msg->file->name, msg->offset,
					"cannot handle AOF observation type %d subtype %d",
					OBS(5), OBS(6));
	}

	DBA_RUN_OR_GOTO(cleanup, dba_msgs_append_acquire(outs, out));
	out = NULL;
	*msgs = outs;
	outs = NULL;

cleanup:
	if (out) dba_msg_delete(out);
	if (outs) dba_msgs_delete(outs);
	return err == DBA_OK ? dba_error_ok() : err;
}

void aof_decoder_dump(dba_rawmsg msg, FILE* out)
{
	/* char id[10]; */
	const uint32_t* obs;
	int obs_len;
	int i;

	assert(msg != NULL);

	TRACE("aof_message_decode\n");

	/* Access the raw data in a more comfortable form */
	obs = (const uint32_t*)msg->buf;
	obs_len = msg->len / sizeof(uint32_t);

	for (i = 0; i < obs_len; i++)
		if (obs[i] == 0x7fffffff)
			fprintf(out, "%2d %10s\n", i+1, "missing");
		else
		{
			int j;
			uint32_t x = obs[i];
			fprintf(out, "%2d %10u %8x ", i+1, obs[i], obs[i]);
			for (j = 0; j < 32; j++)
			{
				fputc((x & 0x80000000) != 0 ? '1' : '0', out);
				x <<= 1;
				if ((j+1) % 8 == 0)
					fputc(' ', out);
			}
			fputc('\n', out);
		}
}

dba_err aof_read_satob(const uint32_t* obs, int obs_len, dba_msg msg)
{
	return dba_error_unimplemented("parsing AOF SATOB observations");
//	*out = NULL;
//	return dba_error_ok();
}

dba_err aof_read_satem(const uint32_t* obs, int obs_len, dba_msg msg)
{
	return dba_error_unimplemented("parsing AOF SATEM observations");
//	*out = NULL;
//	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
