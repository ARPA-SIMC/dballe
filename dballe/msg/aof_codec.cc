/*
 * DB-ALLe - Archive for point-based meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "aof_importers/common.h"
#include "msg.h"
#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
//#include <dballe/core/file_internals.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>
#include <time.h>
#include <errno.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

AOFImporter::AOFImporter(const import::Options& opts)
    : Importer(opts) {}
AOFImporter::~AOFImporter() {}

void AOFImporter::import(const Rawmsg& msg, Msgs& msgs) const
{
	/* char id[10]; */
	TRACE("aof_message_decode\n");

	/* Access the raw data in a more comfortable form */
	const uint32_t* obs = (const uint32_t*)msg.data();
	int obs_len = msg.size() / sizeof(uint32_t);

	TRACE("05 grid box number: %d\n", OBS(5));
	TRACE("obs type: %d, %d\n", OBS(6), OBS(7));

    auto_ptr<Msg> out(new Msg);

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
		case 1: read_synop(obs, obs_len, *out); break;
		case 2: read_flight(obs, obs_len, *out); break;
		case 3: read_satob(obs, obs_len, *out); break;
		case 4: read_dribu(obs, obs_len, *out); break;
		case 5: read_temp(obs, obs_len, *out); break;
		case 6: read_pilot(obs, obs_len, *out); break;
		case 7: read_satem(obs, obs_len, *out); break;
		default:
                error_parse::throwf(msg.filename().c_str(), msg.offset,
					"cannot handle AOF observation type %d subtype %d",
					OBS(5), OBS(6));
	}

    msgs.acquire(out);
}

void AOFImporter::import(const bufrex::Msg& msg, Msgs& msgs) const
{
    throw error_unimplemented("AOF importer cannot import from bufrex::Msg");
}

void AOFImporter::get_category(const Rawmsg& msg, int* category, int* subcategory)
{
	/* Access the raw data in a more comfortable form */
	const uint32_t* obs = (const uint32_t*)msg.data();
	int obs_len = msg.size() / sizeof(uint32_t);

	if (obs_len < 7)
		throw error_parse(msg.filename().c_str(), msg.offset,
				"the buffer is too short to contain an AOF message");

	*category = obs[5];
	*subcategory = obs[6];
}

void AOFImporter::dump(const Rawmsg& msg, FILE* out)
{
	/* Access the raw data in a more comfortable form */
	const uint32_t* obs = (const uint32_t*)msg.data();
	int obs_len = msg.size() / sizeof(uint32_t);

	for (int i = 0; i < obs_len; i++)
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


void AOFImporter::read_satob(const uint32_t* obs, int obs_len, Msg& msg)
{
	throw error_unimplemented("parsing AOF SATOB observations");
}

void AOFImporter::read_satem(const uint32_t* obs, int obs_len, Msg& msg)
{
	throw error_unimplemented("parsing AOF SATEM observations");
}


#if 0

/*
 * AOF I/O utilities
 */

dba_err aof_codec_read_record(dba_file file, uint32_t** rec, int* len)
{
	uint32_t len_word, len_word1;
	FILE* in = file->fd;
	int swapwords = 0;

	/* Read the first Fortran length of record word */
	if (fread(&len_word, 4, 1, in) == 0)
	{
		if (feof(in))
		{
			*rec = NULL;
			*len = 0;
			return dba_error_ok();
		}
		return dba_error_system("reading a record-length first word in %s during AOF decoding", file->name);
	}

	if ((len_word & 0xFF000000) != 0)
	{
		swapwords = 1;
		len_word = bswap_32(len_word);
	}

	if (len_word % 4 != 0)
		return dba_error_parse(file->name, ftell(in), "length of record (%d) is not a multiple of 4", len_word);

	if ((*rec = (uint32_t*)malloc(len_word)) == NULL)
		return dba_error_alloc("allocating space for an AOF record");
	
	/* Read the record */
	if (fread(*rec, len_word, 1, in) == 0)
	{
		free(*rec);
		*rec = NULL;
		return dba_error_system("reading a %d-bytes record from %s during AOF decoding", len_word, file->name);
	}

	/* Read the last Fortran length of record word */
	if (fread(&len_word1, 4, 1, in) == 0)
	{
		free(*rec);
		*rec = NULL;
		return dba_error_system("reading a record-length last word in %s during AOF decoding", file->name);
	}

	/* Swap words if needed */
	if (swapwords)
	{
		int i;
		for (i = 0; i < len_word / 4; i++)
			(*rec)[i] = bswap_32((*rec)[i]);
		len_word1 = bswap_32(len_word1);
	}

	if (len_word != len_word1)
	{
		free(*rec);
		*rec = NULL;
		return dba_error_parse(file->name, ftell(in), "initial length of record is different than the final length of record");
	}

	*len = len_word / 4;

	return dba_error_ok();
}

static enum { END_ARCH = 1, END_LE = 2, END_BE = 3 } writer_endianness = 0;

static inline void init_writer_endiannes_if_needed()
{
	if (writer_endianness == 0)
	{
		char* env_swap = getenv("DBA_AOF_ENDIANNESS");
		if (env_swap == NULL)
			writer_endianness = END_ARCH;
		else if (strcmp(env_swap, "ARCH") == 0)
			writer_endianness = END_ARCH;
		else if (strcmp(env_swap, "LE") == 0)
			writer_endianness = END_LE;
		else if (strcmp(env_swap, "BE") == 0)
			writer_endianness = END_BE;
		else
			writer_endianness = END_ARCH;
	}
}

static dba_err output_word(dba_file file, uint32_t word)
{
	uint32_t oword;
	switch (writer_endianness)
	{
		case END_ARCH: oword = word; break;
#if __BYTE_ORDER == __BIG_ENDIAN
		case END_LE: oword = bswap_32(word); break;
		case END_BE: oword = word; break;
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
		case END_LE: oword = word; break;
		case END_BE: oword = bswap_32(word); break;
#else
		case END_LE: oword = bswap_32(htonl(word)); break;
		case END_BE: oword = htonl(word); break;
#endif
#endif
	}
	if (fwrite(&oword, sizeof(uint32_t), 1, file->fd) != 1)
		return dba_error_system("writing 4 bytes on %s", dba_file_name(file));
	return dba_error_ok();
}

/* Write a fortran "unformatted sequential" record contained in an array of 32-bit words
 * len is the len of 'rec' in words */
dba_err aof_codec_write_record(dba_file file, const uint32_t* rec, int len)
{
	int i;

	init_writer_endiannes_if_needed();

	/* Write the leading length of record word */
	DBA_RUN_OR_RETURN(output_word(file, len * sizeof(uint32_t)));
	for (i = 0; i < len; i++)
		DBA_RUN_OR_RETURN(output_word(file, rec[i]));
	/* Write the trailing length of record word */
	DBA_RUN_OR_RETURN(output_word(file, len * sizeof(uint32_t)));

	return dba_error_ok();
}

/*
 * Header read and write functions
 */

dba_err aof_codec_read_header(dba_file file, \
		uint32_t** res_fdr, int* res_fdr_len, \
		uint32_t** res_ddr, int* res_ddr_len)
{
	dba_err err = DBA_OK;
	FILE* in = file->fd;
	uint32_t* fdr = NULL;
	int fdr_len;
	uint32_t* ddr = NULL;
	int ddr_len;

	/* Read the First Data Record */
	DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &fdr, &fdr_len));

	if (fdr_len != 14)
	{
		err = dba_error_parse(file->name, ftell(in),
				"FDR contains %d octets instead of 14", fdr_len);
		goto cleanup;
	}

	/* Consistency checks */
	if (fdr[0] != 14)
	{
		err = dba_error_parse(file->name, ftell(in),
				"first word of FDR is %d instead of 14", fdr[0]);
		goto cleanup;
	}

	/* Read Data Descriptor Record */
	DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &ddr, &ddr_len));

	if (ddr_len != 17)
	{
		err = dba_error_parse(file->name, ftell(in),
				"DDR contains %d octets instead of 17", ddr_len);
		goto cleanup;
	}

#if 0
	reader->start.tm_hour = ddr[10] % 100;
	reader->start.tm_mday = ((ddr[10] / 100) % 100);
	reader->start.tm_mon = ((ddr[10] / 10000) % 100) - 1;
	reader->start.tm_year = (ddr[10] / 1000000) - 1900;

	reader->end.tm_hour = ddr[12] % 100;
	reader->end.tm_mday = ((ddr[12] / 100) % 100);
	reader->end.tm_mon = ((ddr[12] / 10000) % 100) - 1;
	reader->end.tm_year = (ddr[12] / 1000000) - 1900;
#endif

	*res_fdr = fdr;
	*res_fdr_len = fdr_len;
	fdr = NULL;
	*res_ddr = ddr;
	*res_ddr_len = ddr_len;
	ddr = NULL;

cleanup:
	if (fdr != NULL)
		free(fdr);
	if (ddr != NULL)
		free(ddr);
	return err = DBA_OK ? dba_error_ok() : err;
}

dba_err aof_codec_write_dummy_header(dba_file file)
{
	uint32_t fdr[14];
	uint32_t ddr[17];
	/* Use 'now' for start and end times */
	time_t tnow = time(NULL);
	struct tm* now = gmtime(&tnow);
	struct tm* start = now;
	struct tm* end = now;

	assert(file != NULL);
	
	/* Write FDR */
	fdr[ 0] =   14;
	fdr[ 1] =   13;
	fdr[ 2] =    0;
	fdr[ 3] = 2048;
	fdr[ 4] =    2;
	fdr[ 5] = (now->tm_year + 1900) * 10000 + (now->tm_mon + 1) * 100 + now->tm_mday;
	fdr[ 6] = now->tm_hour * 100 + now->tm_min;
	fdr[ 7] = ((uint32_t)1 << 31) - 1;
	fdr[ 8] =    1;
	fdr[ 9] =   60;
	fdr[10] = ((uint32_t)1 << 31) - 1;
	fdr[11] = ((uint32_t)1 << 31) - 1;
	fdr[12] =    1;
	fdr[13] = ((uint32_t)1 << 31) - 1;
	DBA_RUN_OR_RETURN(aof_codec_write_record(file, fdr, 14));

	/* Write DDR */
	ddr[ 0] =  17;
	ddr[ 1] =  16;
	ddr[ 2] =   0;
	ddr[ 3] = 820;
	ddr[ 4] =   2;
	ddr[ 5] = (now->tm_year + 1900) * 10000 + (now->tm_mon + 1) * 100 + now->tm_mday;
	ddr[ 6] = now->tm_hour * 100 + now->tm_min;
	ddr[ 7] = ((uint32_t)1 << 31) - 1;
	ddr[ 8] = ((uint32_t)1 << 31) - 1;
	ddr[ 9] =   60;
	ddr[10] = (start->tm_year + 1900) * 1000000 + (start->tm_mon + 1) * 10000 +
				(start->tm_mday * 100) + start->tm_hour;
	ddr[11] =   1;
	ddr[12] = (end->tm_year + 1900) * 1000000 + (end->tm_mon + 1) * 10000 +
				(end->tm_mday * 100) + end->tm_hour;
	ddr[13] = ((uint32_t)1 << 31) - 1;
	ddr[14] = ((uint32_t)1 << 31) - 1;
	ddr[15] =    2;
	ddr[16] = ((uint32_t)1 << 31) - 1;
	DBA_RUN_OR_RETURN(aof_codec_write_record(file, ddr, 17));

	return dba_error_ok();
}


dba_err aof_codec_fix_header(dba_file file)
{
	dba_err err = DBA_OK;
	uint32_t* rec = NULL;
	int len;
	uint32_t start = 0xffffffff;
	uint32_t end = 0;
	uint32_t this;
	uint32_t endianness_test;

	/* Read the FDR */
	DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &rec, &len));
	free(rec); rec = NULL;

	/* Read the DDR */
	DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &rec, &len));
	free(rec); rec = NULL;

	/* Iterate through all the records in the file */
	DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &rec, &len));
	while (rec != NULL)
	{
		if (len < 11)
		{
			err = dba_error_consistency("checking for correctness of the length of the observation record");
			goto cleanup;
		}
		/* Compute the extremes of start and end */
		this = rec[10-1] * 100 + rec[11-1]/100;
		if (this < start)
			start = this;
		if (this > end)
			end = this;
		free(rec); rec = NULL;
		DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &rec, &len));
	}

	/* Update the header with the new extremes */

	/* Check if we need to swap bytes to match the header encoding */
	if (fseek(file->fd, 0, SEEK_SET) == -1)
	{
		err = dba_error_system("trying to seek to start of file %s", file->name);
		goto cleanup;
	}

	if (fread(&endianness_test, 4, 1, file->fd) == 0)
	{
		err = dba_error_system("reading the first word of file %s", file->name);
		goto cleanup;
	}

	if ((endianness_test & 0xFF000000) != 0)
	{
		start = bswap_32(start);
		end = bswap_32(end);
	}

	/* Write start of observation period */
	if (fseek(file->fd, 14 + 10, SEEK_SET) == -1)
	{
		err = dba_error_system("trying to seek in file %s", file->name);
		goto cleanup;
	}
	if (fwrite(&start, sizeof(uint32_t), 1, file->fd) != 1)
	{
		err = dba_error_system("rewriting 4 bytes on %s", file->name);
		goto cleanup;
	}

	/* Write end of observation period */
	if (fseek(file->fd, 14 + 12, SEEK_SET) == -1)
	{
		err = dba_error_system("trying to seek in file %s", file->name);
		goto cleanup;
	}
	if (fwrite(&end, sizeof(uint32_t), 1, file->fd) != 1)
	{
		err = dba_error_system("rewriting 4 bytes on %s", file->name);
		goto cleanup;
	}

cleanup:
	if (rec != NULL)
		free(rec);
	return err == DBA_OK ? dba_error_ok() : err;
}

/*
 * Implementation of AOF support for dba_file
 */

static dba_err aof_file_read(dba_file file, dba_rawmsg msg, int* found)
{
	dba_err err = DBA_OK;
	FILE* in = file->fd;
	uint32_t* rec = NULL;
	int rec_len;

	assert(msg != NULL);

	/* If we are at the beginning of the file, then skip the file header */
	if (ftell(file->fd) == 0)
	{
		uint32_t *fdr, *ddr;
		int fdr_len, ddr_len;
		DBA_RUN_OR_RETURN(aof_codec_read_header(file, &fdr, &fdr_len, &ddr, &ddr_len));
		free(fdr);
		free(ddr);
	}

	dba_rawmsg_reset(msg);

	msg->file = file;
	msg->offset = ftell(file->fd);

	/* Read the Observation Header */
	DBA_RUN_OR_GOTO(cleanup, aof_codec_read_record(file, &rec, &rec_len));

	if (rec_len == 0)
	{
		*found = 0;
		goto cleanup;
	}

	if (rec[1] != 4)
	{
		err = dba_error_parse(file->name, ftell(in),
				"value '01 length of preliminary record' should be 4, either big or little endian (it is %d (%08x) instead", rec[1], rec[1]);
		goto cleanup;
	}

	DBA_RUN_OR_GOTO(cleanup, dba_rawmsg_acquire_buf(msg, (unsigned char*)rec, rec_len * sizeof(uint32_t)));
	rec = NULL;

	msg->encoding = AOF;

	*found = 1;

cleanup:
	if (rec != NULL)
		free(rec);
	return err = DBA_OK ? dba_error_ok() : err;
}
static dba_err aof_file_write(dba_file file, dba_rawmsg msg)
{
	long pos = ftell(file->fd);

	if (pos == -1 && errno != ESPIPE)
		return dba_error_system("reading current position in output file %s", dba_file_name(file));

	/* If it's a non-seekable file, we use idx to see if we're at the beginning */
	if (pos == -1 && errno == ESPIPE && file->idx == 0)
		pos = 0;
		
	/* If we are at the beginning of the file, write a dummy header */
	if (pos == 0)
		DBA_RUN_OR_RETURN(aof_codec_write_dummy_header(file));
	
	return aof_codec_write_record(file, (const uint32_t*)msg->buf, msg->len / sizeof(uint32_t));
}
static void aof_file_delete(dba_file file)
{
	free(file);
}
static dba_err aof_file_create(dba_encoding type, FILE* fd, const char* mode, dba_file* file)
{
	*file = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (*file == NULL)
		return dba_error_alloc("allocating new _dba_file");
	(*file)->fun_delete = aof_file_delete;
	(*file)->fun_read = aof_file_read;
	(*file)->fun_write = aof_file_write;
	return dba_error_ok();
}


/* Register / deregister the codec with dba_file */

static dba_file_create_fun old_aof_create_fun;

void aof_codec_init(void)
{
	old_aof_create_fun = dba_file_aof_create;
	dba_file_aof_create = aof_file_create;
}

void aof_codec_shutdown(void)
{
	dba_file_aof_create = old_aof_create_fun;
}

#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
