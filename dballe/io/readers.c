#include "config.h"

#include "readers.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <assert.h>


dba_err dba_file_reader_read(dba_file_reader reader, dba_rawmsg msg, int* found)
{
	return reader->read_fun(reader, msg, found);
}

void dba_file_reader_delete(dba_file_reader reader)
{
	reader->delete_fun(reader);
}


/* ** ** **  Reader for BUFR  ** ** ** */

struct _bufr_reader
{
	struct _dba_file_reader parent;
};
typedef struct _bufr_reader* bufr_reader;

static void bufr_reader_delete(bufr_reader reader);
static dba_err bufr_reader_read(bufr_reader reader, dba_rawmsg msg, int* found);

dba_err dba_file_reader_create_bufr(dba_file_reader* reader, dba_rawfile file)
{
	bufr_reader res = (bufr_reader)calloc(1, sizeof(struct _bufr_reader));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR reader");
	res->parent.delete_fun = (dba_file_reader_delete_fun)bufr_reader_delete;
	res->parent.read_fun = (dba_file_reader_read_fun)bufr_reader_read;
	res->parent.file = file;

	*reader = (dba_file_reader)res;
	return dba_error_ok();
}

static void bufr_reader_delete(bufr_reader reader)
{
	free(reader);
}

static dba_err bufr_reader_read(bufr_reader reader, dba_rawmsg msg, int* found)
{
	dba_rawfile file = reader->parent.file;
	FILE* in = file->fd;

	/* A BUFR message is easy to just read: it starts with "BUFR", then the
	 * message length encoded in 3 bytes */

	/* Reset bufr_message data in case this message has been used before */
	dba_rawmsg_reset(msg);

	msg->file = file;

	/* Seek to start of BUFR data */
	{
		const char* target = "BUFR";
		static const int target_size = 4;
		int got = 0;
		int c;

		errno = 0;
		while (got < target_size && (c = getc(in)) != EOF)
		{
			if (c == target[got])
				got++;
			else
				got = 0;
		}

		if (errno != 0)
			return dba_error_system("when looking for start of BUFR data in %s", file->name);
		
		if (got != target_size)
		{
			/* End of file: return accordingly */
			*found = 0;
			return dba_error_ok();
		}
		/* Allocate initial buffer and set the initial BUFR prefix.  There can
		 * be allocated memory already if the crex_message is reused */
		if (msg->len == msg->alloclen)
			DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(msg));
		memcpy(msg->buf, target, target_size);
		msg->len = target_size;
		msg->offset = ftell(in) - target_size;
	}

	if (fread(msg->buf + msg->len, 4, 1, in) != 1)
		return dba_error_system("reading BUFR section 0 from %s", file->name);
	msg->len += 4;

	{
		/* Read the message length */
		int bufrlen = ntohl(*(uint32_t*)(msg->buf+4)) >> 8;

		if (bufrlen < 12)
			return dba_error_consistency("checking that the size declared by the BUFR message (%d) is less than the minimum of 12");

		/* Allocate enough space to fit the message */
		while (msg->alloclen < bufrlen)
			DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(msg));

		/* Read the rest of the BUFR message */
		if (fread(msg->buf + msg->len, bufrlen - 8, 1, in) != 1)
			return dba_error_system("reading BUFR message from %s", file->name);

		msg->len = bufrlen;
	}
	
	msg->encoding = BUFR;

	*found = 1;
	return dba_error_ok();
}




/* ** ** **  Reader for CREX  ** ** ** */

struct _crex_reader
{
	struct _dba_file_reader parent;
};
typedef struct _crex_reader* crex_reader;

static void crex_reader_delete(crex_reader reader);
static dba_err crex_reader_read(crex_reader reader, dba_rawmsg msg, int* found);

dba_err dba_file_reader_create_crex(dba_file_reader* reader, dba_rawfile file)
{
	crex_reader res = (crex_reader)calloc(1, sizeof(struct _crex_reader));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR reader");
	res->parent.delete_fun = (dba_file_reader_delete_fun)crex_reader_delete;
	res->parent.read_fun = (dba_file_reader_read_fun)crex_reader_read;
	res->parent.file = file;

	*reader = (dba_file_reader)res;
	return dba_error_ok();
}

static void crex_reader_delete(crex_reader reader)
{
	free(reader);
}


dba_err crex_reader_read(crex_reader reader, dba_rawmsg msg, int* found)
{
	dba_rawfile file = reader->parent.file;
	FILE* in = file->fd;
/*
 * The CREX message starts with "CREX" and ends with "++\r\r\n7777".  It's best
 * if any combination of \r and \n is supported.
 *
 * When reading the message, the memory buffer is initially sized 2kb to account
 * for most small CREX messages.  The size is then doubled every time the limit
 * is exceeded until it reaches 16Kb.  From then, it will grow 2kb at a time as
 * a safety procedure, but the CREX specifications require  messages to be
 * shorter than 15Kb.
 */
	/* Reset crex_message data in case this message has been used before */
	dba_rawmsg_reset(msg);

	msg->file = file;

	/* Seek to start of CREX data */
	{
		const char* target = "CREX++";
		static const int target_size = 6;
		int got = 0;
		int c;

		errno = 0;
		while (got < target_size && (c = getc(in)) != EOF)
		{
			if (c == target[got])
				got++;
			else
				got = 0;
		}

		if (errno != 0)
			return dba_error_system("when looking for start of CREX data in %s", file->name);
		
		if (got != target_size)
		{
			/* End of file: return accordingly */
			*found = 0;
			return dba_error_ok();
		}
		/* Allocate initial buffer and set the initial CREX prefix.  There can
		 * be allocated memory already if the crex_message is reused */
		if (msg->len == msg->alloclen)
			DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(msg));
		memcpy(msg->buf, target, target_size);
		msg->len = target_size;
		msg->offset = ftell(in) - target_size;
	}

	/* Read until "\+\+(\r|\n)+7777" */
	{
		const char* target = "++\r\n7777";
		static const int target_size = 8;
		int got = 0;
		int c;

		errno = 0;
		while (got < 8 && (c = getc(in)) != EOF)
		{
			if (target[got] == '\r' && (c == '\n' || c == '\r'))
				got++;
			else if (target[got] == '\n' && (c == '\n' || c == '\r'))
				;
			else if (target[got] == '\n' && c == '7')
				got += 2;
			else if (c == target[got])
				got++;
			else
				got = 0;

			/* If we are at the end of the buffer, get more space */
			if (msg->len == msg->alloclen)
				DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(msg));

			msg->buf[msg->len++] = c;
		}
		if (errno != 0)
			return dba_error_system("when looking for end of CREX data in %s", file->name);

		if (got != target_size)
			return dba_error_parse(file->name, ftell(in), "CREX message is incomplete");
	}

	msg->encoding = CREX;

	*found = 1;
	return dba_error_ok();
}





/* ** ** **  Reader for AOF  ** ** ** */

struct _aof_reader
{
	struct _dba_file_reader parent;

	/* First data record */
	uint32_t fdr[14];
	/* Data description record */
	uint32_t ddr[17];

	/* Start time of the observation */
	struct tm start;

	/* End time of the observation */
	struct tm end;
};
typedef struct _aof_reader* aof_reader;

static void aof_reader_delete(aof_reader reader);
static dba_err aof_reader_read(aof_reader reader, dba_rawmsg msg, int* found);

dba_err dba_file_reader_create_aof(dba_file_reader* reader, dba_rawfile file)
{
	aof_reader res = (aof_reader)calloc(1, sizeof(struct _aof_reader));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR reader");
	res->parent.delete_fun = (dba_file_reader_delete_fun)aof_reader_delete;
	res->parent.read_fun = (dba_file_reader_read_fun)aof_reader_read;
	res->parent.file = file;

	*reader = (dba_file_reader)res;
	return dba_error_ok();
}

static void aof_reader_delete(aof_reader reader)
{
	free(reader);
}

/* Read a fortran "unformatted sequential" record with an array of 32-bit words */
dba_err aof_reader_read_record(dba_rawfile file, uint32_t** rec, int* len)
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
		len_word = ntohl(len_word);
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
			(*rec)[i] = ntohl((*rec)[i]);
		len_word1 = ntohl(len_word1);
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

static dba_err aof_reader_read_header(aof_reader reader)
{
	dba_err err = DBA_OK;
	dba_rawfile file = reader->parent.file;
	FILE* in = file->fd;
	uint32_t* fdr = NULL;
	int fdr_len;
	uint32_t* ddr = NULL;
	int ddr_len;
	int i;

	/* Read the First Data Record */
	DBA_RUN_OR_GOTO(cleanup, aof_reader_read_record(file, &fdr, &fdr_len));

	if (fdr_len != 14)
	{
		err = dba_error_parse(file->name, ftell(in),
				"FDR contains %d octets instead of 14", fdr_len);
		goto cleanup;
	}

	/* Check endianness */
	if (fdr[0] != 14)
	{
		err = dba_error_parse(file->name, ftell(in),
				"first word of FDR is %d instead of 14", fdr[0]);
		goto cleanup;
	}

	for (i = 0; i < 14; i++)
		reader->fdr[i] = fdr[i];

	/* Read Data Descriptor Record */
	DBA_RUN_OR_GOTO(cleanup, aof_reader_read_record(file, &ddr, &ddr_len));

	if (ddr_len != 17)
	{
		err = dba_error_parse(file->name, ftell(in),
				"DDR contains %d octets instead of 17", ddr_len);
		goto cleanup;
	}

	for (i = 0; i < 17; i++)
		reader->ddr[i] = ddr[i];

	reader->start.tm_hour = ddr[10] % 100;
	reader->start.tm_mday = ((ddr[10] / 100) % 100);
	reader->start.tm_mon = ((ddr[10] / 10000) % 100) - 1;
	reader->start.tm_year = (ddr[10] / 1000000) - 1900;

	reader->end.tm_hour = ddr[12] % 100;
	reader->end.tm_mday = ((ddr[12] / 100) % 100);
	reader->end.tm_mon = ((ddr[12] / 10000) % 100) - 1;
	reader->end.tm_year = (ddr[12] / 1000000) - 1900;

cleanup:
	if (fdr != NULL)
		free(fdr);
	if (ddr != NULL)
		free(ddr);
	return err = DBA_OK ? dba_error_ok() : err;
}


static dba_err aof_reader_read(aof_reader reader, dba_rawmsg msg, int* found)
{
	dba_rawfile file = reader->parent.file;
	FILE* in = file->fd;
	dba_err err = DBA_OK;
	uint32_t* rec = NULL;
	int rec_len;

	assert(msg != NULL);

	if (ftell(file->fd) == 0)
	{
		/* We still need to read the file header */
		DBA_RUN_OR_RETURN(aof_reader_read_header(reader));
	}

	dba_rawmsg_reset(msg);

	msg->file = file;
	msg->offset = ftell(file->fd);

	/* Read the Observation Header */
	DBA_RUN_OR_GOTO(cleanup, aof_reader_read_record(file, &rec, &rec_len));

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

/* vim:set ts=4 sw=4: */
