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

#include <dballe/core/file_internals.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*
 * Implementation of BUFR and CREX support for dba_file
 */
static dba_err bufr_file_read(dba_file file, dba_rawmsg msg, int* found)
{
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
static dba_err crex_file_read(dba_file file, dba_rawmsg msg, int* found)
{
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
static dba_err crex_file_write(dba_file file, dba_rawmsg msg)
{
	DBA_RUN_OR_RETURN(dba_file_default_write_impl(file, msg));
	if (fputs("\r\r\n", file->fd) == EOF)
		return dba_error_system("writing CREX data on output");
	return dba_error_ok();
}
static void bufrex_file_delete(dba_file file)
{
	free(file);
}
static dba_err bufr_file_create(dba_encoding type, FILE* fd, const char* mode, dba_file* file)
{
	*file = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (*file == NULL)
		return dba_error_alloc("allocating new _dba_file");
	(*file)->fun_delete = bufrex_file_delete;
	(*file)->fun_read = bufr_file_read;
	(*file)->fun_write = dba_file_default_write_impl;
	return dba_error_ok();
}
static dba_err crex_file_create(dba_encoding type, FILE* fd, const char* mode, dba_file* file)
{
	*file = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (*file == NULL)
		return dba_error_alloc("allocating new _dba_file");
	(*file)->fun_delete = bufrex_file_delete;
	(*file)->fun_read = crex_file_read;
	(*file)->fun_write = crex_file_write;
	return dba_error_ok();
}


/* Register / deregister the codec with dba_file */

static dba_file_create_fun old_bufr_create_fun;
static dba_file_create_fun old_crex_create_fun;

void __attribute__ ((constructor)) bufrex_codec_init(void)
{
	old_bufr_create_fun = dba_file_bufr_create;
	old_crex_create_fun = dba_file_crex_create;
	dba_file_bufr_create = bufr_file_create;
	dba_file_crex_create = crex_file_create;
}

void __attribute__ ((destructor)) bufrex_codec_shutdown(void)
{
	dba_file_bufr_create = old_bufr_create_fun;
	dba_file_crex_create = old_crex_create_fun;
}

/* vim:set ts=4 sw=4: */
