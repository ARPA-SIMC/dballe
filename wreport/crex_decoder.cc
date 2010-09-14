/*
 * wreport/bulletin - CREX decoder
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

#include "opcode.h"
#include "bulletin.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>	/* isspace */
#include <stdlib.h>	/* malloc */
#include <string.h>	/* memcpy */
#include <math.h>	/* NAN */
#include <assert.h>	/* NAN */
#include <errno.h>	/* NAN */

// #define TRACE_DECODER

#ifdef TRACE_DECODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

using namespace std;

namespace wreport {

namespace {
struct Decoder 
{
	/* Input message data */
	const std::string& in;
	/* Output decoded variables */
	CrexBulletin& out;

	/* File name to use for error messages */
	const char* fname;
	/* File offset to use for error messages */
	size_t offset;

	/* Current subset we are decoding */
	Subset* current_subset;

	/* Offset of the start of CREX section 1 */
	int sec1_start;
	/* Offset of the start of CREX section 2 */
	int sec2_start;
	/* Offset of the start of CREX section 3 */
	int sec3_start;
	/* Offset of the start of CREX section 4 */
	int sec4_start;

	/* Value of the next expected check digit */
	int expected_check_digit;

	/* Start of data not yet decoded */
	const char* start;
	const char* cur;

	Decoder(const std::string& in, const char* fname, size_t offset, CrexBulletin& out)
		: in(in), out(out), fname(fname), offset(offset), current_subset(0),
		  sec1_start(0), sec2_start(0), sec3_start(0), sec4_start(0),
		  expected_check_digit(0), start(in.data()), cur(in.data())
	{
	}

	void parse_error(const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3)
	{
		char* context;
		char* message;

		va_list ap;
		va_start(ap, fmt);
		vasprintf(&message, fmt, ap);
		va_end(ap);

		asprintf(&context, "%s:%d+%d: %s", fname, offset, cur - start, message);

		string msg(context);
		free(context);
		free(message);
		throw error_parse(msg);
	}

	void check_eof(const char* next)
	{
		TRACE(" - step %s -\n", next); \
		if (cur >= start + in.size())
			parse_error("end of CREX message while looking for %s", next);
	}

	void check_available_data(unsigned datalen, const char* next)
	{
		if (cur + datalen > start + in.size())
			parse_error("end of CREX message while looking for %s", next);
	}

	void skip_spaces()
	{
		while (cur < start + in.size() && isspace(*cur))
			++cur;
	}

	void skip_data_and_spaces(unsigned datalen)
	{
		cur += datalen;
		skip_spaces();
	}

	void decode_header()
	{
		/* Read crex section 0 (Indicator section) */
		check_available_data(6, "initial header of CREX message");
		if (strncmp((const char*)cur, "CREX++", 6) != 0)
			parse_error("data does not start with CREX header (\"%.6s\" was read instead)", cur);

		skip_data_and_spaces(6);
		TRACE(" -> is CREX\n");

		/* Read crex section 1 (Data description section) */

		check_eof("start of section 1");
		sec1_start = cur - start;

		/* T<version> */
		if (*cur != 'T')
			parse_error("version not found in CREX data description");

		{
			char edition[11];
			int i;
			for (i = 0; i < 10 && cur < start + in.size() && !isspace(*cur); cur++, i++)
				edition[i] = *cur;
			edition[i] = 0;

			if (sscanf(edition, "T%02d%02d%02d",
						&(out.master_table),
						&(out.edition),
						&(out.table)) != 3)
				error_consistency::throwf("Edition (%s) is not in format Ttteevv", edition);
			
			skip_spaces();
			TRACE(" -> edition %d\n", strtol(edition + 1, 0, 10));
		}

		/* A<atable code> */
		check_eof("A code");
		if (*cur != 'A')
			parse_error("A Table informations not found in CREX data description");
		{
			char atable[20];
			int i;
			++cur;
			for (i = 0; i < 10 && cur < start + in.size() && isdigit(*cur); ++cur, ++i)
				atable[i] = *cur;
			atable[i] = 0;
			TRACE("ATABLE \"%s\"\n", atable);
			int val = strtol(atable, 0, 10);
			switch (strlen(atable))
			{
				case 3:
					out.type = val;
					out.subtype = 255;
					out.localsubtype = 0;
					TRACE(" -> category %d\n", strtol(atable, 0, 10));
					break;
				case 6:
					out.type = val / 1000;
					out.subtype = 255;
					out.localsubtype = val % 1000;
					TRACE(" -> category %d, subcategory %d\n", val / 1000, val % 1000);
					break;
				default:
					error_consistency::throwf("Cannot parse an A table indicator %d digits long", strlen(atable));
			}
		}
		skip_spaces();

		/* data descriptors followed by (E?)\+\+ */
		check_eof("data descriptor section");

		out.has_check_digit = 0;
		while (1)
		{
			if (*cur == 'B' || *cur == 'R' || *cur == 'C' || *cur == 'D')
			{
				check_available_data(6, "one data descriptor");
				out.datadesc.push_back(descriptor_code((const char*)cur));
				skip_data_and_spaces(6);
			}
			else if (*cur == 'E')
			{
				out.has_check_digit = 1;
				expected_check_digit = 1;
				skip_data_and_spaces(1);
			}
			else if (*cur == '+')
			{
				check_available_data(1, "end of data descriptor section");
				if (*(cur+1) != '+')
					parse_error("data descriptor section ends with only one '+'");
				skip_data_and_spaces(2);
				break;
			}
		}
		IFTRACE{
			TRACE(" -> data descriptor section:");
			for (vector<Varcode>::const_iterator i = out.datadesc.begin();
					i != out.datadesc.end(); ++i)
				TRACE(" %01d%02d%03d", WR_VAR_F(*i), WR_VAR_X(*i), WR_VAR_Y(*i));
			TRACE("\n");
		}
	}

	void decode_data()
	{
		// Load tables and set category/subcategory
		out.load_tables();

		/* Decode crex section 2 */
		check_eof("data section");
		sec2_start = cur - start;
		// Scan the various subsections
		for (int i = 0; ; ++i)
		{
			current_subset = &out.obtain_subset(i);
			parse_data_section(Opcodes(out.datadesc));
			skip_spaces();
			check_eof("end of data section");

			if (*cur != '+')
				parse_error("there should be a '+' at the end of the data section");
			++cur;

			/* Peek at the next character to see if it's end of section */
			check_eof("end of data section");
			if (*cur == '+')
			{
				++cur;
				break;
			}
		}
		skip_spaces();

		/* Decode crex optional section 3 */
		check_available_data(4, "CREX optional section 3 or end of CREX message");
		sec3_start = cur - start;
		if (strncmp((const char*)cur, "SUPP", 4) == 0)
		{
			for (cur += 4; strncmp((const char*)cur, "++", 2) != 0; ++cur)
				check_available_data(2, "end of CREX optional section 3");
			skip_spaces();
		}

		/* Decode crex end section 4 */
		check_available_data(4, "end of CREX message");
		sec4_start = cur - start;
		if (strncmp((const char*)cur, "7777", 4) != 0)
			parse_error("unexpected data after data section or optional section 3");
	}

	void parse_data_section(const Opcodes& ops);
	unsigned parse_b_data(const Opcodes& ops);
	unsigned parse_r_data(const Opcodes& ops);
	void parse_value(int len, int is_signed, const char** d_start, const char** d_end);
};

void Decoder::parse_value(int len, int is_signed, const char** d_start, const char** d_end)
{
	TRACE("crex_decoder_parse_value(%d, %s): ", len, is_signed ? "signed" : "unsigned");

	/* Check for 2 more because we may have extra sign and check digit */
	check_available_data(len + 2, "end of data descriptor section");

	if (out.has_check_digit)
	{
		if ((*cur - '0') != expected_check_digit)
			parse_error("check digit mismatch: expected %d, found %d, rest of message: %.*s",
					expected_check_digit,
					(*cur - '0'),
					(start + in.size()) - cur,
					cur);

		expected_check_digit = (expected_check_digit + 1) % 10;
		++cur;
	}

	/* Set the value to start after the check digit (if present) */
	*d_start = cur;

	/* Cope with one extra character in case the sign is present */
	if (is_signed && *cur == '-')
		++len;

	/* Go to the end of the message */
	cur += len;

	/* Set the end value, removing trailing spaces */
	for (*d_end = cur; *d_end > *d_start && isspace(*(*d_end - 1)); (*d_end)--)
		;
	
	/* Skip trailing spaces */
	skip_spaces();

	TRACE("%.*s\n", *d_end - *d_start, *d_start);
}

#if 0
/**
 * Compute a value from a CREX message
 *
 * @param value
 *   The value as found in the CREX message
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
static double crex_decoder_compute_value(bufrex_decoder decoder, const char* value, dba_varinfo* info)
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

unsigned Decoder::parse_b_data(const Opcodes& ops)
{
	IFTRACE{
		TRACE("crex_decoder_parse_b_data: items: ");
		ops.print(stderr);
		TRACE("\n");
	}

	// Get variable information
	Varinfo info = out.btable->query(ops.head());

	// Create the new Var
	Var var(info);

	// Parse value from the data section
	const char* d_start;
	const char* d_end;
	parse_value(info->len, !info->is_string(), &d_start, &d_end);

	/* If the variable is not missing, set its value */
	if (*d_start != '/')
	{
		if (info->is_string())
		{
			const int len = d_end - d_start;
			string buf(d_start, len);
			var.setc(buf.c_str());
		} else {
			int val = strtol((const char*)d_start, 0, 10);

			/* FIXME: here goes handling of active C table modifiers */

			var.seti(val);
		}
	}

	/* Store the variable that we found */
	current_subset->store_variable(var);
	IFTRACE{
		TRACE("  stored variable: "); var.print(stderr); TRACE("\n");
	}

#ifdef TRACE_DECODER
	{
		int left = (start + in.size()) - cur;
		TRACE("crex_decoder_parse_b_data -> %.*s (items:", left > 30 ? 30 : left, cur);
		ops.sub(1).print(stderr);
		TRACE(")\n");
	}
#endif

	return 1;
}

#if 0
static dba_err crex_read_c_data(bufrex_decoder decoder, bufrex_opcode* ops)
{
	bufrex_opcode op;
	dba_err err;
	/* Node affected by the operator */
	bufrex_opcode affected_op;

	/* Pop the C modifier node */
	DBA_RUN_OR_RETURN(bufrex_opcode_pop(ops, &op));

	TRACE("read_c_data\n");

	/* Pop the first node, since we handle it here */
	if ((err = bufrex_opcode_pop(ops, &affected_op)) != DBA_OK)
		goto fail1;

	/* Activate this C modifier */
	switch (WR_VAR_X(op->val))
	{
		case 1:
			decoder->c_width = WR_VAR_Y(op->val);
			break;
		case 2:
			decoder->c_scale = WR_VAR_Y(op->val);
			break;
		case 5:
		case 7:
		case 60:
			err = dba_error_parse(decoder->fname, decoder->line_no,
					"C modifier %d is not supported", WR_VAR_X(op->val));
			goto fail;
		default:
			err = dba_error_parse(decoder->fname, decoder->line_no,
					"Unknown C modifier %d", WR_VAR_X(op->val));
			goto fail;
	}

	/* Decode the affected data */
	if ((err = crex_read_data(decoder, &affected_op)) != DBA_OK)
		goto fail;

	/* Deactivate the C modifier */
	decoder->c_width = 0;
	decoder->c_scale = 0;

	/* FIXME: affected_op should always be NULL */
	assert(affected_op == NULL);
	bufrex_opcode_delete(&affected_op);
	return dba_error_ok();

fail:
	bufrex_opcode_delete(&affected_op);
fail1:
	bufrex_opcode_delete(&op);
	return err;
}
#endif

unsigned Decoder::parse_r_data(const Opcodes& ops)
{
	unsigned first = 1;
	int group = WR_VAR_X(ops.head());
	int count = WR_VAR_Y(ops.head());
	
	TRACE("R DATA %01d%02d%03d %d %d", 
			WR_VAR_F(ops.head()), WR_VAR_X(ops.head()), WR_VAR_Y(ops.head()), group, count);

	if (count == 0)
	{
		/* Delayed replication */
		
		/* Fetch the repetition count */
		const char* d_start;
		const char* d_end;
		parse_value(4, 0, &d_start, &d_end);
		count = strtol((const char*)d_start, NULL, 10);

		/* Insert the repetition count among the parsed variables */
		current_subset->store_variable_i(WR_VAR(0, 31, 1), count);

		TRACE("read_c_data %d items %d times (delayed)\n", group, count);
	} else
		TRACE("read_c_data %d items %d times\n", group, count);

	// Extract the first `group' nodes, to handle here
	Opcodes group_ops = ops.sub(first, group);

	// parse_data_section on it `count' times
	for (int i = 0; i < count; ++i)
		parse_data_section(group_ops);

	// Number of items processed
	return first + group;
}

void Decoder::parse_data_section(const Opcodes& ops)
{
	/*
	fprintf(stderr, "read_data: ");
	bufrex_opcode_print(ops, stderr);
	fprintf(stderr, "\n");
	*/
	TRACE("crex_decoder_parse_data_section: START\n");

	for (unsigned i = 0; i < ops.size(); )
	{
		IFTRACE{
			TRACE("crex_decoder_parse_data_section TODO: ");
			ops.sub(i).print(stderr);
			TRACE("\n");
		}

		switch (WR_VAR_F(ops[i]))
		{
			case 0: i += parse_b_data(ops.sub(i)); break;
			case 1: i += parse_r_data(ops.sub(i)); break;
			case 2: parse_error("C modifiers are not yet supported for CREX");
			case 3:
			{
				Opcodes exp = out.dtable->query(ops[i]);
				parse_data_section(exp);
				++i;
				break;
			}
			default:
				parse_error("cannot handle field %01d%02d%03d",
							WR_VAR_F(ops[i]),
							WR_VAR_X(ops[i]),
							WR_VAR_Y(ops[i]));
		}
	}
}

}

void CrexBulletin::decode_header(const std::string& buf, const char* fname, size_t offset)
{
	clear();
	Decoder d(buf, fname, offset, *this);
	d.decode_header();
}

void CrexBulletin::decode(const std::string& buf, const char* fname, size_t offset)
{
	clear();
	Decoder d(buf, fname, offset, *this);
	d.decode_header();
	d.decode_data();
}

}

/* vim:set ts=4 sw=4: */
