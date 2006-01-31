#include "config.h"

#include "bufrex_opcode.h"
#include "bufrex_raw.h"
#include <dballe/io/dba_rawfile.h>

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

/* #define TRACE_DECODER */

#ifdef TRACE_DECODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

struct _decoder 
{
	/* Input message data */
	dba_rawmsg in;
	/* Output decoded variables */
	bufrex_raw out;

	/* Offset of the start of BUFR section 1 */
	const unsigned char* sec1;
	/* Offset of the start of BUFR section 2 */
	const unsigned char* sec2;
	/* Offset of the start of BUFR section 3 */
	const unsigned char* sec3;
	/* Offset of the start of BUFR section 4 */
	const unsigned char* sec4;
	/* Offset of the start of BUFR section 5 */
	const unsigned char* sec5;

	/* Number of subsets present in the message */
	int subsets;

	/* Current value of scale change from C modifier */
	int c_scale_change;
	/* Current value of width change from C modifier */
	int c_width_change;

	/* List of opcodes to decode */
	bufrex_opcode ops;

	/* Bit decoding data */
	int cursor;
	unsigned char pbyte;
	int pbyte_len;
};
typedef struct _decoder* decoder;

static dba_err decoder_create(decoder* res)
{
	if ((*res = (decoder)calloc(1, sizeof(struct _decoder))) == NULL)
		return dba_error_alloc("allocating a new crex decoder");
	return dba_error_ok();
}

static void decoder_delete(decoder res)
{
	if (res->ops != NULL)
		bufrex_opcode_delete(&(res->ops));
	free(res);
}

static int decoder_offset(decoder d)
{
	return d->cursor;
}

static int decoder_bits_left(decoder d)
{
	return (d->in->len - d->cursor) * 8 + d->pbyte_len;
}

#define PARSE_ERROR(pos, ...) do { \
		err = dba_error_parse(d->in->file->name, d->in->offset + (pos - d->in->buf), __VA_ARGS__); \
		goto fail; \
	} while (0)


/**
 * Get the integer value of the next 'n' bits from the decode input
 * n must be <= 32.
 */
static dba_err decoder_get_bits (decoder d, int n, uint32_t *val)
{
	dba_err err;
	uint32_t result = 0;
	int i;

	if (d->cursor == d->in->len)
		PARSE_ERROR(d->in->buf + d->cursor, "end of buffer while looking for %d bits of bit-packed data", n);

	for (i = 0; i < n; i++) 
	{
		if (d->pbyte_len == 0) 
		{
			d->pbyte_len = 8;
			d->pbyte = d->in->buf[d->cursor++];
		}
		result <<= 1;
		if (d->pbyte & 0x80)
			result |= 1;
		d->pbyte <<= 1;
		d->pbyte_len--;
	}

	*val = result;

	return dba_error_ok();
fail:
	return err;
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


/* Forward declaration of this static function, as we write recursive decoding
 * functions later */
static dba_err bufr_decode_data_section(decoder d);

#define CHECK_AVAILABLE_DATA(start, datalen, next) do { \
		if (start + datalen > d->in->buf + d->in->len) \
			PARSE_ERROR(start, "end of BUFR message while looking for " next); \
		TRACE(next " starts at %d and contains at least %d bytes\n", start - d->in->buf, datalen); \
	} while (0)

dba_err bufr_decoder_decode(dba_rawmsg in, bufrex_raw out)
{
	dba_err err;
	decoder d = NULL;
	int has_optional;
	int i;

	bufrex_raw_reset(out);

	DBA_RUN_OR_RETURN(decoder_create(&d));
	d->in = in;
	d->out = out;

	/* Read BUFR section 0 (Indicator section) */
	CHECK_AVAILABLE_DATA(d->in->buf, 8, "section 0 of BUFR message (indicator section)");
	d->sec1 = d->in->buf + 8;
	if (memcmp(d->in->buf, "BUFR", 4) != 0)
		PARSE_ERROR(d->in->buf, "data does not start with BUFR header (\"%.4s\" was read instead)", d->in->buf);
	TRACE(" -> is BUFR\n");

	/* Check the BUFR edition number */
	if (d->in->buf[7] != 3)
		PARSE_ERROR(d->in->buf + 7, "Only BUFR edition 3 is supported (this message is edition %d)", (unsigned)d->in->buf[7]);
	d->out->edition = d->in->buf[7];

	/* Read bufr section 1 (Identification section) */
	CHECK_AVAILABLE_DATA(d->sec1, 18, "section 1 of BUFR message (identification section)");
	d->sec2 = d->sec1 + (ntohl(*(uint32_t*)(d->sec1)) >> 8);
	if (d->sec2 > d->in->buf + d->in->len)
		PARSE_ERROR(d->sec1, "Section 1 claims to end past the end of the BUFR message");

	has_optional = d->sec1[7] & 0x80;
	d->out->opt.bufr.origin = d->sec1[5];
	d->out->opt.bufr.master_table = d->sec1[10];
	d->out->opt.bufr.local_table = d->sec1[11];
	d->out->type = (int)d->sec1[8];
	d->out->subtype = (int)d->sec1[9];

	d->out->rep_year = (int)d->sec1[12];
	d->out->rep_month = (int)d->sec1[13];
	d->out->rep_day = (int)d->sec1[14];
	d->out->rep_hour = (int)d->sec1[15];
	d->out->rep_minute = (int)d->sec1[16];
	if ((int)d->sec1[17] != 0)
		d->out->rep_year = (int)d->sec1[17] * 100 + (d->out->rep_year % 100);

	TRACE(" -> category %d subcategory %d\n", 
				(int)d->sec1[8],
				(int)d->sec1[9]);

	DBA_RUN_OR_GOTO(fail, bufrex_raw_load_tables(d->out));

#if 0
	fprintf(stderr, "S1 Len %d  table #%d  osc %d  oc %d  seq #%d  optsec %x\n",
			bufr_sec1_len,
			master_table,
			(int)psec1[4],
			(int)psec1[5],
			(int)psec1[6],
			(int)psec1[7]);
		
	fprintf(stderr, "S1 cat %d  subc %d  verm %d verl %d  %d/%d/%d %d:%d\n",
			(int)psec1[8],
			(int)psec1[9],
			(int)psec1[10],
			(int)psec1[11],

			(int)psec1[12],
			(int)psec1[13],
			(int)psec1[14],
			(int)psec1[15],
			(int)psec1[16]);
#endif
	
	/* Read BUFR section 2 (Optional section) */
	if (has_optional)
	{
		CHECK_AVAILABLE_DATA(d->sec2, 4, "section 2 of BUFR message (optional section)");
		d->sec3 = d->sec2 + (ntohl(*(uint32_t*)(d->sec2)) >> 8);
		if (d->sec3 > d->in->buf + d->in->len)
			PARSE_ERROR(d->sec2, "Section 2 claims to end past the end of the BUFR message");
	} else
		d->sec3 = d->sec2;

	/* Read BUFR section 3 (Data description section) */
	CHECK_AVAILABLE_DATA(d->sec3, 8, "section 3 of BUFR message (data description section)");
	d->sec4 = d->sec3 + (ntohl(*(uint32_t*)(d->sec3)) >> 8);
	if (d->sec4 > d->in->buf + d->in->len)
		PARSE_ERROR(d->sec3, "Section 3 claims to end past the end of the BUFR message");
	d->subsets = ntohs(*(uint16_t*)(d->sec3 + 4));
	for (i = 0; i < (d->sec4 - d->sec3 - 8)/2; i++)
	{
		dba_varcode var = (dba_varcode)ntohs(*(uint16_t*)(d->sec3 + 7 + (i*2)));
		DBA_RUN_OR_GOTO(fail, bufrex_raw_append_datadesc(d->out, var));
	}
	/*
	IFTRACE{
		TRACE(" -> data descriptor section: ");
		bufrex_opcode_print(msg->datadesc, stderr);
		TRACE("\n");
	}
	*/

	/* Read BUFR section 4 (Data section) */
	CHECK_AVAILABLE_DATA(d->sec4, 4, "section 4 of BUFR message (data section)");

	d->sec5 = d->sec4 + (ntohl(*(uint32_t*)(d->sec4)) >> 8);
	if (d->sec5 > d->in->buf + d->in->len)
		PARSE_ERROR(d->sec4, "Section 4 claims to end past the end of the BUFR message");
	TRACE("section 4 is %d bytes long (%02x %02x %02x %02x)\n", (ntohl(*(uint32_t*)(d->sec4)) >> 8),
			(unsigned int)*(d->sec4), (unsigned int)*(d->sec4+1), (unsigned int)*(d->sec4+2), (unsigned int)*(d->sec4+3));

	/* Initialize bit-decoding structures */
	d->cursor = d->sec4 + 4 - d->in->buf;

	/* Iterate on the number of subgroups */
	for (i = 0; i < d->subsets; i++)
	{
		DBA_RUN_OR_GOTO(fail, bufrex_raw_get_datadesc(d->out, &(d->ops)));
		DBA_RUN_OR_GOTO(fail, bufr_decode_data_section(d));
	}

	IFTRACE {
	if (decoder_bits_left(d) > 15)
	{
		fprintf(stderr, "The data section of %s still contains %d unparsed bits\n",
				d->in->file->name, decoder_bits_left(d));
		/*
		err = dba_error_parse(msg->file->name, POS + vec->cursor,
				"the data section still contains %d unparsed bits",
				bitvec_bits_left(vec));
		goto fail;
		*/
	}
	}

	/* Read BUFR section 5 (Data section) */
	CHECK_AVAILABLE_DATA(d->sec5, 4, "section 5 of BUFR message (end section)");

	if (memcmp(d->sec5, "7777", 4) != 0)
		PARSE_ERROR(d->sec5, "section 5 does not contain '7777'");

	/* Copy the decoded attributes into the decoded variables */
	DBA_RUN_OR_GOTO(fail, bufrex_raw_apply_attributes(out));

	if (d != NULL)
		decoder_delete(d);
	return dba_error_ok();

fail:
	TRACE(" -> bufr_message_decode failed.\n");

	if (d != NULL)
		decoder_delete(d);
	return err;
}


#if 0
/**
 * Compute a value from a BUFR message
 *
 * @param value
 *   The value as found in the BUFR message
 *
 * @param info
 *   The B table informations for the value
 *
 * @param cmodifier
 *   The C table modifier in effect for this value, or NULL if no C table
 *   modifier is in effect
 *
 * @returns
 *   The decoded value
 */
/* TODO: implement c modifier computation */
static double bufr_decoder_compute_value(bufrex_decoder decoder, const char* value, dba_varinfo* info)
{
	double val;

	/* TODO use the C table values */
	
	if (value[0] == '/')
		return NAN;
 
	val = strtol(value, NULL, 10);

	if (info->scale != 0)
	{
		int scale = info->scale;

		if (info->scale > 0)
			while (scale--)
				val /= 10;
		else
			while (scale++)
				val *= 10;
	}

	3A
	return val;
}
#endif

static double bufr_decode_int(decoder d, uint32_t ival, dba_varinfo info)
{
	double val = ival;
	
	val = (val + info->bit_ref) / pow(10, info->scale);

	return val;
}

static dba_err bufr_decode_b_data(decoder d)
{
	dba_err err = DBA_OK;
	dba_var var = NULL;
	dba_varinfo info = NULL;
	char *str = NULL;

	IFTRACE {
		TRACE("bufr_message_decode_b_data: items: ");
		bufrex_opcode_print(d->ops, stderr);
		TRACE("\n");
	}

	DBA_RUN_OR_RETURN(bufrex_raw_query_btable(d->out, d->ops->val, &info));

	IFTRACE {
		TRACE("Parsing @%d+%d [bl %d+%d sc %d+%d ref %d]: %d%02d%03d %s[%s]\n", d->cursor, 8-d->pbyte_len,
				info->bit_len, d->c_width_change,
				info->scale, d->c_scale_change,
				info->bit_ref,
				DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var),
				info->desc, info->unit);
		/*DBA_RUN_OR_RETURN(dump_bits(d->in->buf + d->cursor - 1, 8 - d->pbyte_len, 32, stderr));*/
		TRACE("\n");
	}

	/* Create the new dba_var */
	DBA_RUN_OR_GOTO(cleanup, dba_var_create(info, &var));

	/* Get the real datum */
	if (info->is_string)
	{
		/* Read a string */
		int toread = info->bit_len;
		int len = 0;

		str = (char*)malloc(info->bit_len / 8 + 2);

		if (str == NULL)
		{
			err = dba_error_alloc("allocating space for a BUFR string variable");
			goto cleanup;
		}

		while (toread > 0)
		{
			uint32_t bitval;
			int count = toread > 8 ? 8 : toread;
			DBA_RUN_OR_GOTO(cleanup, decoder_get_bits(d, count, &bitval));
			str[len++] = bitval;
			toread -= count;
		}
		str[len] = 0;

		/* Trim trailing spaces */
		for (--len; len > 0 && isspace(str[len]);
				len--)
			str[len] = 0;

		DBA_RUN_OR_GOTO(cleanup, dba_var_setc(var, str));
		TRACE("bufr_message_decode_b_data len %d val %s info-len %d info-desc %s\n", len, str, info->bit_len, info->desc);
	} else {
		/* Read a value */
		uint32_t val;
		
		/* Change the variable definition according to the current C modifiers */
		info->scale += d->c_scale_change;
		info->bit_len += d->c_width_change;
		if (d->c_scale_change > 0)
			TRACE("Applied %d scale change\n", d->c_scale_change);
		if (d->c_width_change > 0)
			TRACE("Applied %d scale change\n", d->c_width_change);

		DBA_RUN_OR_GOTO(cleanup, decoder_get_bits(d, info->bit_len, &val));

		TRACE("Reading %s, size %d, starting point %d\n", info->desc, info->bit_len, val);

		/* Check if there are bits which are not 1 (that is, if the value is present) */
		if (val != (((1 << (info->bit_len - 1))-1) | (1 << (info->bit_len - 1))))
		{
			double dval = bufr_decode_int(d, val, info);
			TRACE("Decoded as %f\n", dval);
			DBA_RUN_OR_GOTO(cleanup, dba_var_setd(var, dval));
		}

		/*bufr_decoder_debug(decoder, "  %s: %d%s\n", info.desc, val, info.type);*/
		TRACE("bufr_message_decode_b_data len %d val %d info-len %d info-desc %s\n", info->bit_len, val, info->bit_len, info->desc);
	}

	/* Store the variable that we found */
	DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable(d->out, var));
	var = NULL;

	/* Remove from the chain the item that we handled */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
		bufrex_opcode_delete(&op);
	}

	/*
	IFTRACE {
		TRACE("bufr_message_decode_b_data (items:");
		bufrex_opcode_print(*ops, stderr);
		TRACE(")\n");
	}
	*/

cleanup:
	if (str != NULL)
		free(str);
	if (var != NULL)
		dba_var_delete(var);
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err bufr_decode_r_data(decoder d)
{
	dba_err err = DBA_OK;
	int group = DBA_VAR_X(d->ops->val);
	int count = DBA_VAR_Y(d->ops->val);
	bufrex_opcode rep_op;
	
	TRACE("R DATA %01d%02d%03d %d %d", 
			DBA_VAR_F(d->ops->val), DBA_VAR_X(d->ops->val), DBA_VAR_Y(d->ops->val), group, count);

	/* Pop the R repetition node, since we have read its value in group and count */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
		bufrex_opcode_delete(&op);
	}

	if (count == 0)
	{
		/* Delayed replication */

		bufrex_opcode info_op;
		dba_varinfo info;
		dba_var rep_var;
		uint32_t _count;
		
		/* Read the descriptor for the delayed replicator count */
		DBA_RUN_OR_GOTO(cleanup, bufrex_opcode_pop(&(d->ops), &info_op));

		/* Read variable informations about the delayed replicator count */
		DBA_RUN_OR_GOTO(cleanup, bufrex_raw_query_btable(d->out, info_op->val, &info));
		
		/* Fetch the repetition count */
		DBA_RUN_OR_GOTO(cleanup, decoder_get_bits(d, info->bit_len, &_count));
		count = _count;

		/* Insert the repetition count among the parsed variables */
		DBA_RUN_OR_GOTO(cleanup, dba_var_createi(info, count, &rep_var));
		DBA_RUN_OR_GOTO(cleanup, bufrex_raw_store_variable(d->out, rep_var));

		bufrex_opcode_delete(&info_op);
	/*	dba_var_delete(rep_var);   rep_var is taken in charge by bufrex_raw */

		TRACE("bufr_decode_r_data %d items %d times (delayed)\n", group, count);
	} else
		TRACE("bufr_decode_r_data %d items %d times\n", group, count);

	/* Pop the first `group' nodes, since we handle them here */
	DBA_RUN_OR_RETURN(bufrex_opcode_pop_n(&(d->ops), &rep_op, group));


	/* Perform replication */
	while (count--)
	{
		bufrex_opcode saved = NULL;
		bufrex_opcode chain = NULL;

		/* fprintf(stderr, "Debug: rep %d left\n", count); */
		DBA_RUN_OR_GOTO(cleanup, bufrex_opcode_prepend(&chain, rep_op));

		IFTRACE{
			TRACE("bufr_decode_r_data %d items %d times left; items: ", group, count);
			bufrex_opcode_print(chain, stderr);
			TRACE("\n");
		}

		saved = d->ops;
		d->ops = chain;
		if ((err = bufr_decode_data_section(d)) != DBA_OK)
		{
			if (d->ops != NULL)
				bufrex_opcode_delete(&(d->ops));
			d->ops = saved;
			goto cleanup;
		}
		/* chain should always be NULL when parsing succeeds */
		assert(d->ops == NULL);
		d->ops = saved;
	}

cleanup:
	bufrex_opcode_delete(&rep_op);

	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err bufr_decode_c_data(decoder d)
{
	dba_err err = DBA_OK;
	dba_varcode code = d->ops->val;

	TRACE("C DATA %01d%02d%03d\n", DBA_VAR_F(code), DBA_VAR_X(code), DBA_VAR_Y(code));

	/* Pop the C node, since we have read its value in `code' */
	{
		bufrex_opcode op;
		DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
		bufrex_opcode_delete(&op);
	}

	switch (DBA_VAR_X(code))
	{
		case 1:
			d->c_width_change = DBA_VAR_Y(code) - 128;
			break;
		case 2:
			d->c_scale_change = DBA_VAR_Y(code) - 128;
			break;
		case 22:
			if (DBA_VAR_Y(code) == 0)
			{
				DBA_RUN_OR_GOTO(cleanup, bufr_decode_r_data(d));
			} else {
				err = dba_error_parse(
						d->in->file->name,
						d->sec4 + 4 + decoder_offset(d) - d->in->buf,
						"C modifier %d%02d%03d not yet supported",
							DBA_VAR_F(code),
							DBA_VAR_X(code),
							DBA_VAR_Y(code));
				goto cleanup;
			}
			break;
		case 24:
			if (DBA_VAR_Y(code) == 0)
			{
				DBA_RUN_OR_GOTO(cleanup, bufr_decode_r_data(d));
			} else {
				err = dba_error_parse(d->in->file->name, d->sec4 + 4 + decoder_offset(d) - d->in->buf,
						"C modifiers %d%02d%03d not yet supported",
							DBA_VAR_F(code),
							DBA_VAR_X(code),
							DBA_VAR_Y(code));
				goto cleanup;
			}
			break;
		default:
			err = dba_error_parse(d->in->file->name, d->sec4 + 4 + decoder_offset(d) - d->in->buf,
					"C modifiers (%d%02d%03d in this case) are not yet supported",
						DBA_VAR_F(code),
						DBA_VAR_X(code),
						DBA_VAR_Y(code));
			goto cleanup;
	}

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err bufr_decode_data_section(decoder d)
{
	dba_err err = DBA_OK;

	/*
	fprintf(stderr, "read_data: ");
	bufrex_opcode_print(ops, stderr);
	fprintf(stderr, "\n");
	*/
	TRACE("bufr_message_decode_data_section: START\n");

	while (d->ops != NULL)
	{
		IFTRACE{
			TRACE("bufr_message_decode_data_section TODO: ");
			bufrex_opcode_print(d->ops, stderr);
			TRACE("\n");
		}

		switch (DBA_VAR_F(d->ops->val))
		{
			case 0:
				DBA_RUN_OR_RETURN(bufr_decode_b_data(d));
				break;
			case 1:
				DBA_RUN_OR_RETURN(bufr_decode_r_data(d));
				break;
			case 2:
				DBA_RUN_OR_RETURN(bufr_decode_c_data(d));
				/* DBA_RUN_OR_RETURN(bufr_message_decode_c_data(msg, ops, &s)); */
				break;
			case 3:
			{
				/* D table opcode: expand the chain */
				bufrex_opcode op;
				bufrex_opcode exp;

				/* Pop the first opcode */
				DBA_RUN_OR_RETURN(bufrex_opcode_pop(&(d->ops), &op));
				
				if ((err = bufrex_raw_query_dtable(d->out, op->val, &exp)) != DBA_OK)
				{
					bufrex_opcode_delete(&op);
					return err;
				}
				bufrex_opcode_delete(&op);

				/* Push the expansion back in the fields */
				if ((err = bufrex_opcode_join(&exp, d->ops)) != DBA_OK)
				{
					bufrex_opcode_delete(&exp);
					return err;
				}
				d->ops = exp;
				break;
			}
			default:
				return dba_error_parse(d->in->file->name, d->sec4 + 4 + decoder_offset(d) - d->in->buf,
						"cannot handle field %01d%02d%03d",
							DBA_VAR_F(d->ops->val),
							DBA_VAR_X(d->ops->val),
							DBA_VAR_Y(d->ops->val));
		}
	}

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
