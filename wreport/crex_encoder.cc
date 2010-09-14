/*
 * wreport/bulletin - CREX encoder
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

// #define TRACE_ENCODER

#ifdef TRACE_ENCODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

using namespace std;

namespace wreport {

namespace {

/// Iterate variables in a subset, one at a time
struct Varqueue
{
	const Subset& subset;
	unsigned cur;

	Varqueue(const Subset& subset) : subset(subset), cur(0) {}

	const bool empty() const { return cur >= subset.size(); }
	const unsigned size() const { return subset.size() - cur; }
	const Var& peek() const { return subset[cur]; }
	const Var& pop() { return subset[cur++]; }
};

struct Encoder
{
	/* Input message data */
	const CrexBulletin& in;
	/* Output decoded variables */
	std::string& out;

	/* Offset of the start of CREX section 1 */
	int sec1_start;
	/* Offset of the start of CREX section 2 */
	int sec2_start;
	/* Offset of the start of CREX section 3 */
	int sec3_start;
	/* Offset of the start of CREX section 4 */
	int sec4_start;

	/* True if the CREX message uses the check digit feature */
	int has_check_digit;
	/* Value of the next expected check digit */
	int expected_check_digit;

	/* Subset we are encoding */
	const Subset* subset;

	Encoder(const CrexBulletin& in, std::string& out)
		: in(in), out(out),
		  sec1_start(0), sec2_start(0), sec3_start(0), sec4_start(0),
		  has_check_digit(0), expected_check_digit(0), subset(0)
	{
	}

	void raw_append(const char* str, int len)
	{
		out.append(str, len);
	}

	void raw_appendf(const char* fmt, ...) __attribute__ ((format(printf, 2, 3)))
	{
		char buf[256];
		va_list ap;
		va_start(ap, fmt);
		int len = vsnprintf(buf, 255, fmt, ap);
		va_end(ap);

		out.append(buf, len);
	}

	void encode_check_digit()
	{
		if (!has_check_digit) return;

		char c = '0' + expected_check_digit;
		raw_append(&c, 1);
		expected_check_digit = (expected_check_digit + 1) % 10;
	}

	void encode_sec1()
	{
		raw_appendf("T%02d%02d%02d A%03d%03d",
				in.master_table,
				in.edition,
				in.table,
				in.type,
				in.localsubtype);

		/* Encode the data descriptor section */

#if 0
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
#endif
	
		for (vector<Varcode>::const_iterator i = in.datadesc.begin();
				i != in.datadesc.end(); ++i)
		{
			char prefix;
			switch (WR_VAR_F(*i))
			{
				case 0: prefix = 'B'; break;
				case 1: prefix = 'R'; break;
				case 2: prefix = 'C'; break;
				case 3: prefix = 'D'; break;
				default: prefix = '?'; break;
			}

			// Don't put delayed replication counts in the data section
			if (WR_VAR_F(*i) == 0 && WR_VAR_X(*i) == 31 && WR_VAR_Y(*i) < 3)
				continue;

			raw_appendf(" %c%02d%03d", prefix, WR_VAR_X(*i), WR_VAR_Y(*i));
		}

		if (has_check_digit)
		{
			raw_append(" E", 2);
			expected_check_digit = 1;
		}

		raw_append("++\r\r\n", 5);
	}

	void run()
	{
		/* Encode section 0 */
		raw_append("CREX++\r\r\n", 9);

		/* Encode section 1 */
		sec1_start = out.size();
		encode_sec1();
		TRACE("SEC1 encoded as [[[%s]]]", out.substr(sec1_start).c_str());

		/* Encode section 2 */
		sec2_start = out.size();
		for (unsigned i = 0; i < in.subsets.size(); ++i)
		{
			TRACE("Start encoding subsection %d\n", i);

			/* Encode the data of this subset */
			subset = &in.subset(i);
			Varqueue varqueue(*subset);
			encode_data_section(Opcodes(in.datadesc), varqueue);

			if (!varqueue.empty())
				error_consistency::throwf("subset %d, %d variables left after processing all descriptors",
						varqueue.size());

			/* Encode the subsection terminator */
			if (i < in.subsets.size() - 1)
				raw_append("+\r\r\n", 4);
			else
				raw_append("++\r\r\n", 5);
		}
		TRACE("SEC2 encoded as [[[%s]]]", out.substr(sec2_start).c_str());

		/* Encode section 3 */
		sec3_start = out.size();
		/* Nothing to do, as we have no custom section */

		/* Encode section 4 */
		sec4_start = out.size();
		raw_append("7777\r\r\n", 7);
	}

	unsigned encode_b_data(const Opcodes& ops, Varqueue& vars);
	unsigned encode_r_data(const Opcodes& ops, Varqueue& vars);
	void encode_data_section(const Opcodes& ops, Varqueue& vars);
};

#if 0
dba_err encoder_has_check_digit(crex_message msg, int* has_check_digit)
{
	*has_check_digit = msg->has_check_digit;
	return dba_error_ok();
}

dba_err encoder_set_check_digit(crex_message msg, int has_check_digit)
{
	msg->has_check_digit = has_check_digit;
	return dba_error_ok();
}
#endif


unsigned Encoder::encode_b_data(const Opcodes& ops, Varqueue& vars)
{
	IFTRACE{
		TRACE("crex encode_b_data: items: ");
		ops.print(stderr);
		TRACE("\n");

		/*
		TRACE("Next 5 variables:\n");
		int i = 0;
		for (; i < 5 && i < e->vars_left; i++)
			dba_var_print(e->nextvar[i], stderr);
		*/
	}

	if (vars.empty())
		throw error_consistency("no more variables to encode");

	/* Get informations about the variable */
	Varinfo info = in.btable->query(ops.head());
	const Var* var = &vars.pop();

	int len = info->len;
	raw_append(" ", 1);
	encode_check_digit();
	if (var->value() == NULL)
	{
		for (int i = 0; i < len; i++)
			raw_append("/", 1);
		TRACE("encode_b missing len: %d\n", len);
	} else if (info->is_string()) {
		raw_appendf("%-*.*s", len, len, var->value());
		TRACE("encode_b string len: %d val %-*.*s\n", len, len, var->value());
	} else {
		int val = var->enqi();

		/* FIXME: here goes handling of active C table modifiers */
		
		if (val < 0) ++len;
		
		raw_appendf("%0*d", len, val);
		TRACE("encode_b num len: %d val %0*d\n", len, len, val);
	}
	
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

unsigned Encoder::encode_r_data(const Opcodes& ops, Varqueue& vars)
{
	unsigned used = 1;
	int group = WR_VAR_X(ops.head());
	int count = WR_VAR_Y(ops.head());

	IFTRACE{
		TRACE("crex encode_r_data %01d%02d%03d %d %d: items: ",
			WR_VAR_F(ops.head()), WR_VAR_X(ops.head()), WR_VAR_Y(ops.head()), group, count);
		ops.print(stderr);
		TRACE("\n");

		/*
		TRACE("Next 5 variables:\n");
		int i = 0;
		for (; i < 5 && e->nextvar[i] != NULL; i++)
			dba_var_print(e->nextvar[i], stderr);
		*/
	}

	if (count == 0)
	{
		/* Delayed replication */

		/* Look for a delayed replication factor in the input vars */
		if (vars.empty())
			throw error_consistency("checking for availability of data to encode");

		/* Get the repetition count */
		count = vars.pop().enqi();

		TRACE("delayed replicator count read as %d\n", count);

		/* Encode the repetition count */
		raw_append(" ", 1);
		encode_check_digit();
		raw_appendf("%04d", count);

		/* No need to move past the node with the repetition count, as
		 * in crex there is no opcode for it */
		// ++used;

		TRACE("encode_r_data %d items %d times (delayed)\n", group, count);
	} else
		TRACE("encode_r_data %d items %d times\n", group, count);

	// Extract the first `group' nodes, to handle here
	Opcodes group_ops = ops.sub(used, group);

	// encode_data_section on it `count' times
	for (int i = 0; i < count; ++i)
		encode_data_section(group_ops, vars);

	return used + group;
}

void Encoder::encode_data_section(const Opcodes& ops, Varqueue& vars)
{
	TRACE("crex_message_encode_data_section: START\n");

	for (unsigned i = 0; i < ops.size(); )
	{
		IFTRACE{
			TRACE("crex encode_data_section TODO: ");
			ops.sub(i).print(stderr);
			TRACE("\n");
			TRACE("crex encode_data_section NEXTVAR: ");
			if (vars.empty())
				TRACE("(none)\n");
			else
				vars.peek().print(stderr);
		}

		switch (WR_VAR_F(ops[i]))
		{
			case 0: i += encode_b_data(ops.sub(i), vars); break;
			case 1: i += encode_r_data(ops.sub(i), vars); break;
			case 2: throw error_unimplemented("encoding C modifiers is not supported yet");
			case 3:
			{
				Opcodes exp = in.dtable->query(ops[i]);
				encode_data_section(exp, vars);
				++i;
				break;
			}
			default:
				error_consistency::throwf(
						"variable %01d%02d%03d cannot be handled",
							WR_VAR_F(ops[i]),
							WR_VAR_X(ops[i]),
							WR_VAR_Y(ops[i]));
		}
	}
}

} // Unnamed namespace

void CrexBulletin::encode(std::string& buf) const
{
	Encoder e(*this, buf);
	e.run();
	//out.encoding = CREX;
}

} // bufrex namespace

/* vim:set ts=4 sw=4: */
