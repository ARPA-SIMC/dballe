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

#include "bufrex_codec.h"
#include <dballe/bufrex/msg.h>
#include <dballe/bufrex/subset.h>
#include "bufrex_exporters/exporters.h"
#include <dballe/core/verbose.h>
#include <dballe/core/file_internals.h>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>	/* malloc */
#include <errno.h>
#include <string.h>

extern dba_err bufrex_copy_to_generic(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_synop(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_metar(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_temp(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_pilot(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_flight(dba_msg msg, bufrex_msg raw, bufrex_subset sset);
extern dba_err bufrex_copy_to_sat(dba_msg msg, bufrex_msg raw, bufrex_subset sset);

dba_err bufrex_decode_bufr(dba_rawmsg raw, dba_msgs* msgs)
{
	dba_err err = DBA_OK;
	bufrex_msg rmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&rmsg, BUFREX_BUFR));
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_decode(rmsg, raw));
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "Decoded BUFR data:\n");
		bufrex_msg_print(rmsg, DBA_VERBOSE_STREAM);
	}
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_to_dba_msgs(rmsg, msgs));

cleanup:
	if (rmsg != NULL)
		bufrex_msg_delete(rmsg);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_decode_crex(dba_rawmsg raw, dba_msgs* msgs)
{
	dba_err err = DBA_OK;
	bufrex_msg rmsg = NULL;

	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&rmsg, BUFREX_CREX));
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_decode(rmsg, raw));
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "Decoded CREX data:\n");
		bufrex_msg_print(rmsg, DBA_VERBOSE_STREAM);
	}
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_to_dba_msgs(rmsg, msgs));

cleanup:
	if (rmsg != NULL)
		bufrex_msg_delete(rmsg);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_encode_bufr(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_msg braw = NULL;
	*raw = NULL;

	if (msgs->len == 0) return dba_error_consistency("tried to encode an empty dba_msgs");

	/* Create and setup the bufrex_msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&braw, BUFREX_BUFR));

	/* Compute the right type and subtype if missing */
	if (type == 0 || subtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msgs->msgs[0], &(braw->type), &(braw->subtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
	}

	/* Setup encoding parameters */
	if (msgs->msgs[0]->type == MSG_GENERIC)
	{
		braw->opt.bufr.origin = 200;
		braw->opt.bufr.master_table = 12;
		braw->opt.bufr.local_table = 0;
	} else {
		braw->opt.bufr.origin = 98;
		braw->opt.bufr.master_table = 6;
		braw->opt.bufr.local_table = 1;
	}

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_from_dba_msgs(braw, msgs));

	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "BUFR data before encoding:\n");
		bufrex_msg_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_msg_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_encode_crex(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_msg braw = NULL;
	*raw = NULL;

	if (msgs->len == 0) return dba_error_consistency("tried to encode an empty dba_msgs");

	/* Create and setup the bufrex_msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(&braw, BUFREX_CREX));

	/* Compute the right type and subtype if missing */
	if (type == 0 || subtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msgs->msgs[0], &(braw->type), &(braw->subtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
	}

	/* Setup encoding parameters */
	if (msgs->msgs[0]->type == MSG_GENERIC)
	{
		braw->opt.crex.master_table = 99;
		braw->edition = 2;
		braw->opt.crex.table = 3;
	} else {
		braw->opt.crex.master_table = 0;
		braw->edition = 1;
		braw->opt.crex.table = 3;
	}

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_from_dba_msgs(braw, msgs));
	
	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "CREX data before encoding:\n");
		bufrex_msg_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_msg_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_msg_to_dba_msgs(bufrex_msg raw, dba_msgs* msgs)
{
	dba_err err = DBA_OK;
	dba_msgs res = NULL;
	dba_msg msg = NULL;
	int i;

	DBA_RUN_OR_GOTO(cleanup, dba_msgs_create(&res));

	for (i = 0; i < raw->subsets_count; ++i)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_msg_create(&msg));

		switch (raw->type)
		{
			case 0:
			case 1:
				if (raw->subtype == 140)
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_metar(msg, raw, raw->subsets[i]));
				else
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_synop(msg, raw, raw->subsets[i]));
				break;
			case 2:
				if (raw->subtype == 91 || raw->subtype == 92)
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_pilot(msg, raw, raw->subsets[i]));
				else
					DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_temp(msg, raw, raw->subsets[i]));
				break;
			case 3: DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_sat(msg, raw, raw->subsets[i])); break;
			case 4: DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_flight(msg, raw, raw->subsets[i])); break;
			default: DBA_RUN_OR_GOTO(cleanup, bufrex_copy_to_generic(msg, raw, raw->subsets[i])); break;
		}

		DBA_RUN_OR_GOTO(cleanup, dba_msgs_append_acquire(res, msg));
		msg = NULL;
	}

	*msgs = res;
	res = NULL;

cleanup:
	if (msg != NULL)
		dba_msg_delete(msg);
	if (res != NULL)
		dba_msgs_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err bufrex_msg_from_dba_msg(bufrex_msg raw, dba_msg msg)
{
	bufrex_exporter exp;
	bufrex_subset subset;

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(bufrex_get_exporter(msg, raw->type, raw->subtype, &exp));

	/* Init the bufrex_msg data descriptor chain */
	DBA_RUN_OR_RETURN(exp->datadesc(exp, msg, raw));

	/* Import the message into the first subset */
	DBA_RUN_OR_RETURN(bufrex_msg_get_subset(raw, 0, &subset));

	/* Fill up the bufrex_msg with variables from msg */
	DBA_RUN_OR_RETURN(exp->exporter(msg, raw, subset, raw->encoding_type == BUFREX_BUFR ? 0 : 1));

	/* Fill in the nominal datetime informations */
	{
		dba_var var;
		if ((var = dba_msg_get_year_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_year)));
		if ((var = dba_msg_get_month_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_month)));
		if ((var = dba_msg_get_day_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_day)));
		if ((var = dba_msg_get_hour_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_hour)));
		if ((var = dba_msg_get_minute_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_minute)));
	}
	
	return dba_error_ok();
}

dba_err bufrex_msg_from_dba_msgs(bufrex_msg raw, dba_msgs msgs)
{
	bufrex_exporter exp;
	int i;

	if (msgs->len == 0)
		return dba_error_consistency("tried to export an empty dba_msgs");

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(bufrex_get_exporter(msgs->msgs[0], raw->type, raw->subtype, &exp));

	/* Init the bufrex_msg data descriptor chain */
	DBA_RUN_OR_RETURN(exp->datadesc(exp, msgs->msgs[0], raw));

	/* Import each message into its own subset */
	for (i = 0; i < msgs->len; ++i)
	{
		bufrex_subset subset;
		DBA_RUN_OR_RETURN(bufrex_msg_get_subset(raw, i, &subset));

		/* Fill up the bufrex_msg with variables from msg */
		DBA_RUN_OR_RETURN(exp->exporter(msgs->msgs[i], raw, subset, raw->encoding_type == BUFREX_BUFR ? 0 : 1));
	}

	/* Fill in the nominal datetime informations */
	{
		dba_var var;
		if ((var = dba_msg_get_year_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_year)));
		if ((var = dba_msg_get_month_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_month)));
		if ((var = dba_msg_get_day_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_day)));
		if ((var = dba_msg_get_hour_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_hour)));
		if ((var = dba_msg_get_minute_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_minute)));
	}
	
	return dba_error_ok();
}


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
