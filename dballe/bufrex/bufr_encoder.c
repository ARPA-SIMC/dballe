/*
 * DB-ALLe - Archive for punctual meteorological data
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

#include "config.h"

#define _ISOC99_SOURCE  // For round()

#include <dballe/core/conv.h>
#include "opcode.h"
#include "msg.h"

#include <stdio.h>
#include <netinet/in.h>

#include <stdlib.h>	/* malloc */
#include <ctype.h>	/* isspace */
#include <string.h>	/* memcpy */
#include <stdarg.h>	/* va_start, va_end */
#include <math.h>	/* NAN */
#include <time.h>
#include <errno.h>

#include <assert.h>

//#define DEFAULT_TABLE_ID "B000000000980601"
/*
For encoding our generics:
#define DEFAULT_ORIGIN 255
#define DEFAULT_MASTER_TABLE 12
#define DEFAULT_LOCAL_TABLE 0
#define DEFAULT_TABLE_ID "B000000002551200"
*/

/* #define TRACE_ENCODER */

#ifdef TRACE_ENCODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif


struct _encoder
{
	/* Input message data */
	bufrex_msg in;
	/* Output decoded variables */
	dba_rawmsg out;

	/* We have to memorise offsets rather than pointers, because e->out->buf
	 * can get reallocated during the encoding */

	/* Offset of the start of BUFR section 1 */
	int sec1_start;
	/* Offset of the start of BUFR section 2 */
	int sec2_start;
	/* Offset of the start of BUFR section 3 */
	int sec3_start;
	/* Offset of the start of BUFR section 4 */
	int sec4_start;
	/* Offset of the start of BUFR section 4 */
	int sec5_start;

	/* Current value of scale change from C modifier */
	int c_scale_change;
	/* Current value of width change from C modifier */
	int c_width_change;

	/* List of opcodes to decode */
	bufrex_opcode ops;
	/* Pointed to next variable not yet encoded in the variable array */
	dba_var* nextvar;
	/* Number of variables left to encode */
	int vars_left;

	/* Support for binary append */
	unsigned char pbyte;
	int pbyte_len;
};
typedef struct _encoder* encoder;

static dba_err encoder_create(encoder* res)
{
	if ((*res = (encoder)calloc(1, sizeof(struct _encoder))) == NULL)
		return dba_error_alloc("allocating a new crex encoder");
	return dba_error_ok();
}

static void encoder_delete(encoder res)
{
	if (res->ops != NULL)
		bufrex_opcode_delete(&(res->ops));
	free(res);
}

#if 0
/* Dump 'count' bits of 'buf', starting at the 'ofs-th' bit */
static dba_err dump_bits(void* buf, int ofs, int count, FILE* out)
{
	bitvec vec;
	int i, j;
	DBA_RUN_OR_RETURN(bitvec_create(&vec, "mem", 0, buf, (count + ofs) / 8 + 2));
	for (i = 0, j = 0; i < ofs; i++, j++)
	{
		uint32_t val;
		DBA_RUN_OR_RETURN(bitvec_get_bits(vec, 1, &val));
		if (j != 0 && (j % 8) == 0)
			putc(' ', out);
		putc(val ? ',' : '.', out);
	}
	for (i = 0; i < count; i++, j++)
	{
		uint32_t val;
		DBA_RUN_OR_RETURN(bitvec_get_bits(vec, 1, &val));
		if (j != 0 && (j % 8) == 0)
			putc(' ', out);
		putc(val ? '1' : '0', out);
	}
	bitvec_delete(vec);
	return dba_error_ok();
}
#endif


/* Write all bits left to the buffer, padding with zeros */
static dba_err encoder_flush(encoder e)
{
	if (e->pbyte_len == 0)
		return dba_error_ok();

	while (e->pbyte_len < 8)
	{
		e->pbyte <<= 1;
		e->pbyte_len++;
	}

	while (e->out->len + 1 > e->out->alloclen)
		DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(e->out));
	e->out->buf[e->out->len++] = e->pbyte;
	e->pbyte_len = 0;
	e->pbyte = 0;

	return dba_error_ok();
}

/**
 * Append n bits from 'val'.  n must be <= 32.
 */
static dba_err encoder_add_bits(encoder e, uint32_t val, int n)
{
	/* Mask for reading data out of val */
	uint32_t mask = 1 << (n - 1);
	int i;

	for (i = 0; i < n; i++) 
	{
		e->pbyte <<= 1;
		e->pbyte |= ((val & mask) != 0) ? 1 : 0;
		val <<= 1;
		e->pbyte_len++;

		if (e->pbyte_len == 8) 
			DBA_RUN_OR_RETURN(encoder_flush(e));
	}
#if 0
	IFTRACE {
		/* Prewrite it when tracing, to allow to dump the buffer as it's
		 * written */
		while (e->out->len + 1 > e->out->alloclen)
			DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(e->out));
		e->out->buf[e->out->len] = e->pbyte << (8 - e->pbyte_len);
	}
#endif

	return dba_error_ok();
}

static dba_err encoder_raw_append(encoder e, const char* str, int len)
{
	while (e->out->len + len > e->out->alloclen)
		DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(e->out));
	memcpy(e->out->buf + e->out->len, str, len);
	e->out->len += len;
	return dba_error_ok();
}

static dba_err encoder_append_short(encoder e, unsigned short val)
{
	return encoder_add_bits(e, val, 16);
}
static dba_err encoder_append_byte(encoder e, unsigned char val)
{
	return encoder_add_bits(e, val, 8);
}

#if 0
static dba_err bufr_message_append_byte(bufr_message msg, unsigned char val)
{
	while (msg->len + 1 > msg->alloclen)
		DBA_RUN_OR_RETURN(dba_message_expand_buffer(msg));
	memcpy(msg->buf + msg->len, &val, 1);
	msg->len += 1;
	return dba_error_ok();
}

static dba_err bufr_message_append_short(bufr_message msg, unsigned short val)
{
	uint16_t encval = htons(val);
	while (msg->len + 2 > msg->alloclen)
		DBA_RUN_OR_RETURN(dba_message_expand_buffer(msg));
	memcpy(msg->buf + msg->len, &encval, 2);
	msg->len += 2;
	return dba_error_ok();
}

static dba_err bufr_message_append_24bit(bufr_message msg, unsigned int val)
{
	uint32_t encval = htonl(val);
	while (msg->len + 3 > msg->alloclen)
		DBA_RUN_OR_RETURN(dba_message_expand_buffer(msg));
	memcpy(msg->buf + msg->len, ((char*)&encval) + 1, 3);
	msg->len += 3;
	return dba_error_ok();
}
#endif


static dba_err encoder_encode_data_section(encoder e);

static dba_err encoder_encode_sec1ed3(encoder e)
{
	/* Encode bufr section 1 (Identification section) */
	/* Length of section */
	DBA_RUN_OR_RETURN(encoder_add_bits(e, 18, 24));
	/* Master table number */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));
	/* Originating/generating sub-centre (defined by Originating/generating centre) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.subcentre));
	/* Originating/generating centre (Common Code tableC-1) */
	/*DBA_RUN_OR_RETURN(bufr_message_append_byte(e, 0xff));*/
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.centre));
	/* Update sequence number (zero for original BUFR messages; incremented for updates) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.update_sequence_number));
	/* Bit 1= 0 No optional section = 1 Optional section included Bits 2 ­ 8 set to zero (reserved) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.optional_section_length ? 0x80 : 0));

	/* Data category (BUFR Table A) */
	/* Data sub-category (defined by local ADP centres) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->type));
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->localsubtype));
	/* Version number of master tables used (currently 9 for WMO FM 94 BUFR tables) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.master_table));
	/* Version number of local tables used to augment the master table in use */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.local_table));

	/* Year of century */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_year == 2000 ? 100 : (e->in->rep_year % 100)));
	/* Month */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_month));
	/* Day */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_day));
	/* Hour */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_hour));
	/* Minute */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_minute));
	/* Century */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_year / 100));

	return dba_error_ok();
}

static dba_err encoder_encode_sec1ed4(encoder e)
{
	/* Encode bufr section 1 (Identification section) */
	/* Length of section */
	DBA_RUN_OR_RETURN(encoder_add_bits(e, 22, 24));
	/* Master table number */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));
	/* Originating/generating centre (Common Code tableC-1) */
	DBA_RUN_OR_RETURN(encoder_append_short(e, e->in->opt.bufr.centre));
	/* Originating/generating sub-centre (defined by Originating/generating centre) */
	DBA_RUN_OR_RETURN(encoder_append_short(e, e->in->opt.bufr.subcentre));
	/* Update sequence number (zero for original BUFR messages; incremented for updates) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));
	/* Bit 1= 0 No optional section = 1 Optional section included Bits 2 ­ 8 set to zero (reserved) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.optional_section_length ? 0x80 : 0));

	/* Data category (BUFR Table A) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->type));
	/* International data sub-category */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->subtype));
 	/* Local subcategory (defined by local ADP centres) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->localsubtype));
	/* Version number of master tables used (currently 9 for WMO FM 94 BUFR tables) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.master_table));
	/* Version number of local tables used to augment the master table in use */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->opt.bufr.local_table));

	/* Year of century */
	DBA_RUN_OR_RETURN(encoder_append_short(e, e->in->rep_year));
	/* Month */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_month));
	/* Day */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_day));
	/* Hour */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_hour));
	/* Minute */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_minute));
	/* Second */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->rep_second));

	return dba_error_ok();
}

dba_err bufr_encoder_encode(bufrex_msg in, dba_rawmsg out)
{
	dba_err err = DBA_OK;
	encoder e = NULL;
	bufrex_opcode ops = NULL;
	bufrex_opcode cur;
	int dslen;
	int i;

	DBA_RUN_OR_RETURN(encoder_create(&e));
	e->in = in;
	e->out = out;


	/* Encode bufr section 0 (Indicator section) */
	DBA_RUN_OR_RETURN(encoder_raw_append(e, "BUFR\0\0\0", 7));
	DBA_RUN_OR_RETURN(encoder_append_byte(e, e->in->edition));

	TRACE("sec0 ends at %d\n", e->out->len);
	e->sec1_start = e->out->len;

	switch (e->in->edition)
	{
		case 3:
			DBA_RUN_OR_RETURN(encoder_encode_sec1ed3(e));
			break;
		case 4:
			DBA_RUN_OR_RETURN(encoder_encode_sec1ed4(e));
			break;
		default:
			break;
	}


	TRACE("sec1 ends at %d\n", e->out->len);
	e->sec2_start = e->out->len;


	/* Encode BUFR section 2 (Optional section) */
	/* Nothing to do */
	if (e->in->opt.bufr.optional_section_length)
	{
		int pad;
		/* Length of section */
		if ((pad = (e->in->opt.bufr.optional_section_length % 2 == 1)))
			DBA_RUN_OR_RETURN(encoder_add_bits(e, 4 + e->in->opt.bufr.optional_section_length + 1, 24));
		else
			DBA_RUN_OR_RETURN(encoder_add_bits(e, 4 + e->in->opt.bufr.optional_section_length, 24));
		/* Set to 0 (reserved) */
		DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));

		DBA_RUN_OR_RETURN(encoder_raw_append(e, e->in->opt.bufr.optional_section, e->in->opt.bufr.optional_section_length));
		// Padd to even number of bytes
		if (pad)
			DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));
	}

	TRACE("sec2 ends at %d\n", e->out->len);
	e->sec3_start = e->out->len;


	/* Encode BUFR section 3 (Data description section) */

	if (e->in->subsets_count == 0)
	{
		err = dba_error_consistency("bufrex_msg to encode has no data subsets");
		goto fail;
	}

	DBA_RUN_OR_GOTO(fail, bufrex_msg_get_datadesc(e->in, &ops));
	if (ops == NULL)
	{
		TRACE("Regenerating datadesc\n");
		/* If the data descriptor list is not already present, try to generate it
		 * from the varcodes of the variables in the first subgroup to encode */
		DBA_RUN_OR_GOTO(fail, bufrex_msg_generate_datadesc(e->in));

		/* Reread the descriptors */
		DBA_RUN_OR_GOTO(fail, bufrex_msg_get_datadesc(e->in, &ops));
	} else {
		TRACE("Reusing datadesc\n");
	}

	/* Count the number of items in the data descriptor section */
	for (cur = ops, dslen = 0; cur != NULL; cur = cur->next, dslen++)
		;

	/* Length of section */
	DBA_RUN_OR_RETURN(encoder_add_bits(e, 8 + 2*dslen, 24));
	/* Set to 0 (reserved) */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));
	/* Number of data subsets */
	DBA_RUN_OR_RETURN(encoder_append_short(e, e->in->subsets_count));
	/* Ensure the bufr subsets indicator in the dba_msg is correct */
	e->in->opt.bufr.subsets = e->in->subsets_count;
	/* Bit 0 = observed data; bit 1 = use compression */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 128));
	
	/* Data descriptors */
	for (cur = ops; cur != NULL; cur = cur->next)
		DBA_RUN_OR_RETURN(encoder_append_short(e, cur->val));

	/* One padding byte to make the section even */
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));

	TRACE("sec3 ends at %d\n", e->out->len);
	e->sec4_start = e->out->len;


	/* Encode BUFR section 4 (Data section) */

	/* Length of section (currently set to 0, will be filled in later) */
	DBA_RUN_OR_RETURN(encoder_add_bits(e, 0, 24));
	DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));

	/* Encode all the subsets, uncompressed */
	for (i = 0; i < e->in->subsets_count; ++i)
	{
		bufrex_subset subset;
		DBA_RUN_OR_GOTO(fail, bufrex_msg_get_subset(e->in, i, &subset));

		/* Initialise the encoder with the list of variables to encode */
		e->nextvar = subset->vars;
		e->vars_left = subset->vars_count;

		/* Encode the data */
		while (e->vars_left > 0)
		{
			DBA_RUN_OR_GOTO(fail, bufrex_opcode_prepend(&(e->ops), ops));

			DBA_RUN_OR_GOTO(fail, encoder_encode_data_section(e));

			if (e->ops != NULL)
			{
				err = dba_error_consistency("not all operators have been encoded");
				goto fail;
			}
		}
	}

	/* Write all the bits and pad the data section to reach an even length */
	TRACE("PRE FLUSH %d:%d\n", e->out->len, e->pbyte_len);
	DBA_RUN_OR_RETURN(encoder_flush(e));
	TRACE("POST FLUSH %d:%d\n", e->out->len, e->pbyte_len);
	if ((e->out->len % 2) == 1)
	{
		TRACE("PRE APPENDBYTE %d:%d\n", e->out->len, e->pbyte_len);
		DBA_RUN_OR_RETURN(encoder_append_byte(e, 0));
		TRACE("POST APPENDBYTE %d:%d\n", e->out->len, e->pbyte_len);
	}
	TRACE("PRE FLUSH2 %d:%d\n", e->out->len, e->pbyte_len);
	DBA_RUN_OR_RETURN(encoder_flush(e));
	TRACE("POST FLUSH2 %d:%d\n", e->out->len, e->pbyte_len);

	/* Write the length of the section in its header */
	{
		uint32_t val = htonl(e->out->len - e->sec4_start);
		memcpy(e->out->buf + e->sec4_start, ((char*)&val) + 1, 3);

		TRACE("sec4 size %d\n", e->out->len - e->sec4_start);
	}

	TRACE("sec4 ends at %d\n", e->out->len);
	e->sec5_start = e->out->len;

	/* Encode section 5 (End section) */
	DBA_RUN_OR_RETURN(encoder_raw_append(e, "7777", 4));

	TRACE("sec5 ends at %d\n", e->out->len);

	/* Write the length of the BUFR message in its header */
	{
		uint32_t val = htonl(e->out->len);
		memcpy(e->out->buf + 4, ((char*)&val) + 1, 3);

		TRACE("msg size %d\n", e->out->len);
	}

	e->out->encoding = BUFR;

	return dba_error_ok();

fail:
	if (e != NULL)
		encoder_delete(e);
	return err;
}

static const double e10[] = {
	1.0,
	10.0,
	100.0,
	1000.0,
	10000.0,
	100000.0,
	1000000.0,
	10000000.0,
	100000000.0,
	1000000000.0,
	10000000000.0,
	100000000000.0,
	1000000000000.0,
	10000000000000.0,
	100000000000000.0,
	1000000000000000.0,
	10000000000000000.0,
};

static int bufr_encode_double(encoder e, double dval, int ref, int scale)
{
	if (scale >= 0)
		return (int)round(dval * e10[scale]) - ref;
	else
		return (int)round(dval / e10[-scale]) - ref;
}

static dba_err encoder_encode_b_data(encoder e)
{
	dba_err err = DBA_OK;
	dba_varinfo info = NULL;
	unsigned int len;
	dba_var var;
	dba_var tmpvar = NULL;
#ifdef TRACE_ENCODER
	int startofs, startbofs;
#endif

	IFTRACE{
		TRACE("bufr_message_encode_b_data: items: ");
		bufrex_opcode_print(e->ops, stderr);
		TRACE("\n");
	}

	/* Get the next variable to encode */
	if (e->vars_left <= 0)
	{
		err = dba_error_consistency("checking for availability of data to encode");
		goto cleanup;
	}
	var = *e->nextvar;
	e->nextvar++;
	e->vars_left--;

	/* Get informations from the variable */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_query_btable(e->in, dba_var_code(var), &info));

	IFTRACE{
#ifndef TRACE_ENCODER
		int startofs, startbofs;
#endif
		TRACE("Encoding @.d+%d [bl %d+%d sc %d+%d ref %d]: ", /*e->in->len - (e->sec4_start + 4),*/ e->pbyte_len,
				info->bit_len, e->c_width_change,
				info->scale, e->c_scale_change,
				info->bit_ref);
		startofs = e->out->len;
		startbofs = e->pbyte_len;
		dba_var_print(var, stderr);
	}

	if (info->var != e->ops->val)
	{
		err = dba_error_consistency("input variable %d%02d%03d differs from expected variable %d%02d%03d",
				DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var),
				DBA_VAR_F(e->ops->val), DBA_VAR_X(e->ops->val), DBA_VAR_Y(e->ops->val));
		goto cleanup;
	}
	
	len = info->bit_len;
	if (dba_var_value(var) == NULL)
	{
		DBA_RUN_OR_GOTO(cleanup, encoder_add_bits(e, 0xffffffff, len));
	} else if (info->is_string) {
		const char* val = dba_var_value(var);
		int i, bi;
		int smax = strlen(val);
		for (i = 0, bi = 0; bi < len; i++)
		{
			TRACE("len: %d, smax: %d, i: %d, bi: %d\n", len, smax, i, bi);
			/* Strings are space-padded in BUFR */
			char todo = (i < smax) ? val[i] : ' ';
			if (len - bi >= 8)
			{
				DBA_RUN_OR_GOTO(cleanup, encoder_append_byte(e, todo));
				bi += 8;
			}
			else
			{
				/* Pad with zeros if writing strings with a number of bits
				 * which is not multiple of 8.  It's not worth to implement
				 * writing partial bytes at the moment and it's better to fail
				 * gracefully, as my understanding is that this case should
				 * never happen anyway. */
				DBA_RUN_OR_GOTO(cleanup, encoder_add_bits(e, 0, len - bi));
				bi = len;
			}
		}
	} else {
		double dval;
		int ival;
		DBA_RUN_OR_GOTO(cleanup, dba_var_enqd(var, &dval));
		TRACE("Starting point %s: %f %s\n", info->desc, dval, info->unit);
		DBA_RUN_OR_GOTO(cleanup, dba_convert_units(info->unit, info->bufr_unit, dval, &dval));
		TRACE("Unit conversion gives: %f %s\n", dval, info->bufr_unit);
		/* Convert to int, optionally applying scale change */
		TRACE("Scale change: %d\n", e->c_scale_change);
		ival = bufr_encode_double(e, dval, info->bit_ref, info->bufr_scale - e->c_scale_change);
		TRACE("Converted to int (ref %d, scale %d): %d\n", info->bit_ref, info->bufr_scale - e->c_scale_change, ival);
		if (e->c_width_change != 0)
			TRACE("Width change: %d\n", e->c_width_change);
		TRACE("Writing %u with size %d\n", ival, len + e->c_width_change);
		/* In case of overflow, store 'missing value' */
		if ((unsigned)ival >= (1u<<(len + e->c_width_change)))
		{
			err = dba_error_consistency("value %f does not fit in variable B%02d%03d", dval, DBA_VAR_X(info->var), DBA_VAR_Y(info->var));
			goto cleanup;
			TRACE("Overflow: %x %u %d >= (1<<(%u + %u)) = %x %u %d\n",
				ival, ival, ival,
				len, e->c_width_change,
				1<<(len + e->c_width_change), 1<<(len + e->c_width_change), 1<<(len + e->c_width_change));
			ival = 0xffffffff;
		}
		TRACE("About to encode: %x %u %d\n", ival, ival, ival);
		DBA_RUN_OR_GOTO(cleanup, encoder_add_bits(e, ival, len + e->c_width_change));
	}
	
	/* Remove from the chain the item that we handled */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
		bufrex_opcode_delete(&op);
	}

	IFTRACE {
		/*
#ifndef TRACE_ENCODER
		int startofs, startbofs;
#endif
		*/
		TRACE("Encoded as: ");
		/*DBA_RUN_OR_RETURN(dump_bits(e->out->buf + startofs, startbofs, 32, stderr));*/
		TRACE("\n");

		TRACE("bufr_message_encode_b_data (items:");
		bufrex_opcode_print(e->ops, stderr);
		TRACE(")\n");
	}

cleanup:
	if (tmpvar != NULL)
		dba_var_delete(tmpvar);
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err encoder_encode_r_data(encoder e)
{
	dba_err err = DBA_OK;
	dba_varinfo info = NULL;
	int group = DBA_VAR_X(e->ops->val);
	int count = DBA_VAR_Y(e->ops->val);
	bufrex_opcode rep_op = NULL;
	
	TRACE("R DATA %01d%02d%03d %d %d", 
			DBA_VAR_F(e->ops->val), DBA_VAR_X(e->ops->val), DBA_VAR_Y(e->ops->val), group, count);

	/* Pop the R repetition node, since we have read its value in group and count */
	{
		bufrex_opcode op;
		DBA_RUN_OR_GOTO(cleanup, bufrex_opcode_pop(&(e->ops), &op));
		bufrex_opcode_delete(&op);
	}

	if (count == 0)
	{
		/* Delayed replication */
		if (e->vars_left <= 0)
		{
			err = dba_error_consistency("checking for availability of data to encode");
			goto cleanup;
		}

		/* Get encoding informations for this repetition count */
		info = dba_var_info(*e->nextvar);

		/* Get the repetition count */
		DBA_RUN_OR_GOTO(cleanup, dba_var_enqi(*e->nextvar, &count));

		/* Encode the repetition count */
		DBA_RUN_OR_GOTO(cleanup, encoder_add_bits(e, count, info->bit_len));

		e->nextvar++;
		e->vars_left--;

		/* Pop the node with the repetition count */
		{
			bufrex_opcode op;
			DBA_RUN_OR_GOTO(cleanup, bufrex_opcode_pop(&(e->ops), &op));
			bufrex_opcode_delete(&op);
		}

		TRACE("encode_r_data %d items %d times (delayed)\n", group, count);
	} else
		TRACE("encode_r_data %d items %d times\n", group, count);

	/* Pop the first `group' nodes, since we handle them here */
	DBA_RUN_OR_GOTO(cleanup, bufrex_opcode_pop_n(&(e->ops), &rep_op, group));

	/* Perform replication */
	while (count--)
	{
		/* fprintf(stderr, "Debug: rep %d left\n", count); */
		bufrex_opcode chain = NULL;
		bufrex_opcode saved = NULL;
		DBA_RUN_OR_GOTO(cleanup, bufrex_opcode_prepend(&chain, rep_op));

		IFTRACE{
			TRACE("encode_r_data %d items %d times left; items: ", group, count);
			bufrex_opcode_print(chain, stderr);
			TRACE("\n");
		}

		saved = e->ops;
		e->ops = chain;
		if ((err = encoder_encode_data_section(e)) != DBA_OK)
		{
			if (e->ops != NULL)
				bufrex_opcode_delete(&(e->ops));
			e->ops = saved;
			goto cleanup;
		}
		/* chain should always be NULL when encoding succeeded */
		assert(e->ops == NULL);
		e->ops = saved;
	}

cleanup:
	bufrex_opcode_delete(&rep_op);

	return err = DBA_OK ? dba_error_ok() : err;
}

static dba_err encoder_encode_c_data(encoder e)
{
	dba_err err = DBA_OK;
	dba_varcode code = e->ops->val;

	TRACE("C DATA %01d%02d%03d\n", DBA_VAR_F(code), DBA_VAR_X(code), DBA_VAR_Y(code));

	/* Pop the C node, since we have read its value in `code' */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
		bufrex_opcode_delete(&op);
	}

	switch (DBA_VAR_X(code))
	{
		case 1:
			e->c_width_change = DBA_VAR_Y(code) - 128;
			break;
		case 2:
			e->c_scale_change = DBA_VAR_Y(code) - 128;
			break;
		case 22:
			if (DBA_VAR_Y(code) == 0)
			{
				DBA_RUN_OR_GOTO(cleanup, encoder_encode_r_data(e));
			} else
				return dba_error_consistency("C modifier %d%02d%03d not yet supported",
							DBA_VAR_F(code),
							DBA_VAR_X(code),
							DBA_VAR_Y(code));
			break;
		case 24:
			if (DBA_VAR_Y(code) == 0)
			{
				DBA_RUN_OR_GOTO(cleanup, encoder_encode_r_data(e));
			} else
				return dba_error_consistency("C modifier %d%02d%03d not yet supported",
							DBA_VAR_F(code),
							DBA_VAR_X(code),
							DBA_VAR_Y(code));
			break;
		default:
			return dba_error_unimplemented("C modifier %d%02d%03d is not yet supported",
						DBA_VAR_F(code),
						DBA_VAR_X(code),
						DBA_VAR_Y(code));
	}

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}


static dba_err encoder_encode_data_section(encoder e)
{
	dba_err err;

	TRACE("bufr_message_encode_data_section: START\n");

	while (e->ops != NULL)
	{
		IFTRACE{
			TRACE("bufr_message_encode_data_section TODO: ");
			bufrex_opcode_print(e->ops, stderr);
			TRACE("\n");
		}

		switch (DBA_VAR_F(e->ops->val))
		{
			case 0:
				DBA_RUN_OR_RETURN(encoder_encode_b_data(e));
				break;
			case 1:
				DBA_RUN_OR_RETURN(encoder_encode_r_data(e));
				break;
			case 2:
				DBA_RUN_OR_RETURN(encoder_encode_c_data(e));
				break;
			case 3:
			{
				/* D table opcode: expand the chain */
				bufrex_opcode op;
				bufrex_opcode exp;

				/* Pop the first opcode */
				DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(e->ops), &op));
				
				if ((err = bufrex_msg_query_dtable(e->in, op->val, &exp)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					return err;
				}

				/* Push the expansion back in the fields */
				if ((err = bufrex_opcode_join(&exp, e->ops)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					bufrex_opcode_delete(&exp);
					return err;
				}
				e->ops = exp;
				break;
			}
			default:
				return dba_error_consistency(
						"variable %01d%02d%03d cannot be handled",
							DBA_VAR_F(e->ops->val),
							DBA_VAR_X(e->ops->val),
							DBA_VAR_Y(e->ops->val));
		}
	}

	return dba_error_ok();
}


/* vim:set ts=4 sw=4: */
