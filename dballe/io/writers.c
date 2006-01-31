#include "config.h"

#include "writers.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <assert.h>

dba_err dba_file_writer_write_raw(dba_file_writer writer, dba_rawmsg msg)
{
	return writer->write_raw_fun(writer, msg);
}

dba_err dba_file_writer_write(dba_file_writer writer, dba_msg msg)
{
	return writer->write_fun(writer, msg);
}

void dba_file_writer_delete(dba_file_writer writer)
{
	writer->delete_fun(writer);
}


/* ** ** **  Writer for BUFR  ** ** ** */

struct _bufr_writer
{
	struct _dba_file_writer parent;
	int out_type;
	int out_subtype;
};
typedef struct _bufr_writer* bufr_writer;

static void bufr_writer_delete(bufr_writer writer);
static dba_err bufr_writer_write(bufr_writer writer, dba_msg msg);
static dba_err bufr_writer_write_raw(bufr_writer writer, dba_rawmsg msg);

dba_err dba_file_writer_create_bufr(dba_file_writer* writer, dba_rawfile file)
{
	bufr_writer res = (bufr_writer)calloc(1, sizeof(struct _bufr_writer));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR writer");
	res->parent.delete_fun = (dba_file_writer_delete_fun)bufr_writer_delete;
	res->parent.write_fun = (dba_file_writer_write_fun)bufr_writer_write;
	res->parent.write_raw_fun = (dba_file_writer_write_raw_fun)bufr_writer_write_raw;
	res->parent.file = file;

	*writer = (dba_file_writer)res;
	return dba_error_ok();
}

static void bufr_writer_delete(bufr_writer writer)
{
	free(writer);
}

dba_err dba_file_writer_set_bufr_template(dba_file_writer writer, int type, int subtype)
{
	bufr_writer w = (bufr_writer)writer;
	w->out_type = type;
	w->out_subtype = subtype;
	return dba_error_ok();
}

static dba_err bufr_writer_write_raw(bufr_writer writer, dba_rawmsg msg)
{
	return dba_rawfile_write(writer->parent.file, msg);
}

static dba_err bufr_writer_write(bufr_writer writer, dba_msg msg)
{
	dba_err err = DBA_OK;
#if 0
	bufr_message cmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, bufr_from_msg(msg, &cmsg, writer->out_type, writer->out_subtype));
	DBA_RUN_OR_GOTO(cleanup, bufr_message_encode(cmsg));
	DBA_RUN_OR_GOTO(cleanup, bufr_writer_write_raw(writer, cmsg));

cleanup:
	if (cmsg != NULL)
		bufr_message_delete(cmsg);
#endif
	return err == DBA_OK ? dba_error_ok() : err;
}




/* ** ** **  Writer for CREX  ** ** ** */

struct _crex_writer
{
	struct _dba_file_writer parent;
	int out_type;
	int out_subtype;
};
typedef struct _crex_writer* crex_writer;

static void crex_writer_delete(crex_writer writer);
static dba_err crex_writer_write(crex_writer writer, dba_msg msg);
static dba_err crex_writer_write_raw(crex_writer writer, dba_rawmsg msg);

dba_err dba_file_writer_create_crex(dba_file_writer* writer, dba_rawfile file)
{
	crex_writer res = (crex_writer)calloc(1, sizeof(struct _crex_writer));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR writer");
	res->parent.delete_fun = (dba_file_writer_delete_fun)crex_writer_delete;
	res->parent.write_fun = (dba_file_writer_write_fun)crex_writer_write;
	res->parent.write_raw_fun = (dba_file_writer_write_raw_fun)crex_writer_write_raw;
	res->parent.file = file;

	*writer = (dba_file_writer)res;
	return dba_error_ok();
}

static void crex_writer_delete(crex_writer writer)
{
	free(writer);
}

dba_err dba_file_writer_set_crex_template(dba_file_writer writer, int type, int subtype)
{
	crex_writer w = (crex_writer)writer;
	w->out_type = type;
	w->out_subtype = subtype;
	return dba_error_ok();
}

static dba_err crex_writer_write_raw(crex_writer writer, dba_rawmsg msg)
{
	DBA_RUN_OR_RETURN(dba_rawfile_write(writer->parent.file, msg));
	if (fputs("\r\r\n", writer->parent.file->fd) == EOF)
		return dba_error_system("writing CREX data on output");
	return dba_error_ok();
}

static dba_err crex_writer_write(crex_writer writer, dba_msg msg)
{
	dba_err err = DBA_OK;
#if 0
	crex_message cmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, crex_from_msg(msg, &cmsg, writer->out_type, writer->out_subtype));
	DBA_RUN_OR_GOTO(cleanup, crex_message_encode(cmsg));
	DBA_RUN_OR_GOTO(cleanup, crex_writer_write_raw(writer, cmsg));

cleanup:
	if (cmsg != NULL)
		crex_message_delete(cmsg);
#endif
	return err == DBA_OK ? dba_error_ok() : err;
}






/* ** ** **  Writer for AOF  ** ** ** */

struct _aof_writer
{
	struct _dba_file_writer parent;

	/* First data record */
	uint32_t fdr[14];
	/* Data description record */
	uint32_t ddr[17];

	/* Start time of the observation */
	struct tm start;

	/* End time of the observation */
	struct tm end;

	/* True if we need to swap words to preserve endianness */
	int swapwords;
};
typedef struct _aof_writer* aof_writer;

static void aof_writer_delete(aof_writer writer);
static dba_err aof_writer_write(aof_writer writer, dba_msg msg);
static dba_err aof_writer_write_raw(aof_writer writer, dba_rawmsg msg);

dba_err dba_file_writer_create_aof(dba_file_writer* writer, dba_rawfile file)
{
	aof_writer res = (aof_writer)calloc(1, sizeof(struct _aof_writer));
	if (res == NULL)
		return dba_error_alloc("Allocating a new BUFR writer");
	res->parent.delete_fun = (dba_file_writer_delete_fun)aof_writer_delete;
	res->parent.write_fun = (dba_file_writer_write_fun)aof_writer_write;
	res->parent.write_raw_fun = (dba_file_writer_write_raw_fun)aof_writer_write_raw;
	res->parent.file = file;

	*writer = (dba_file_writer)res;
	return dba_error_ok();
}

static void aof_writer_delete(aof_writer writer)
{
	free(writer);
}


static dba_err output_word(aof_writer writer, uint32_t word)
{
	uint32_t oword = htonl(word);
	if (fwrite(&oword, sizeof(uint32_t), 1, writer->parent.file->fd) != 1)
		return dba_error_system("writing 4 bytes on %s", writer->parent.file->name);
	return dba_error_ok();
}

/* Write a fortran "unformatted sequential" record contained in an array of 32-bit words
 * len is the len of 'rec' in words */
static dba_err aof_file_writer_write_record(aof_writer writer, const uint32_t* rec, int len)
{
	int i;

	/* Write the first Fortran length of record word */
	DBA_RUN_OR_RETURN(output_word(writer, len * 4));
	for (i = 0; i < len; i++)
		DBA_RUN_OR_RETURN(output_word(writer, rec[i]));
	DBA_RUN_OR_RETURN(output_word(writer, len * 4));

	return dba_error_ok();
}

static dba_err aof_file_writer_write_header(aof_writer writer)
{
	uint32_t fdr[14];
	uint32_t ddr[17];
	time_t tnow = time(NULL);
	struct tm* now = gmtime(&tnow);
	struct tm* start = now;
	struct tm* end = now;

	assert(writer != NULL);
	
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
	DBA_RUN_OR_RETURN(aof_file_writer_write_record(writer, fdr, 14));

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
	DBA_RUN_OR_RETURN(aof_file_writer_write_record(writer, ddr, 17));

	return dba_error_ok();
}

#if 0
dba_err aof_file_write(aof_file file, aof_message msg)
{
	DBA_RUN_OR_RETURN(aof_file_write_record(file, msg->obs, msg->obs_len));
	return dba_error_ok();
}
#endif

static dba_err aof_writer_write_raw(aof_writer writer, dba_rawmsg msg)
{
	const unsigned char* buf;
	int size;
	long pos = ftell(writer->parent.file->fd);

	if (pos == -1)
		return dba_error_system("reading current position in output file %s",
				writer->parent.file->name);
	if (pos == 0)
		DBA_RUN_OR_RETURN(aof_file_writer_write_header(writer));
	
	DBA_RUN_OR_RETURN(dba_rawmsg_get_raw(msg, &buf, &size));
	return aof_file_writer_write_record(writer, (const uint32_t*)buf, size / sizeof(uint32_t));
}

static dba_err aof_writer_write(aof_writer writer, dba_msg msg)
{
	dba_err err = DBA_OK;
#if 0
	aof_message cmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, aof_from_msg(msg, &cmsg, writer->out_type, writer->out_subtype));
	DBA_RUN_OR_GOTO(cleanup, aof_message_encode(cmsg));
	DBA_RUN_OR_GOTO(cleanup, aof_writer_write_raw(writer, cmsg));

cleanup:
	if (cmsg != NULL)
		aof_message_delete(cmsg);
#endif
	return err == DBA_OK ? dba_error_ok() : err;
}



/* vim:set ts=4 sw=4: */
