/*
 * wreport/bulletin - Archive for punctual meteorological data
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

#include "error.h"
#include "opcode.h"
#include "bulletin.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

using namespace std;

namespace wreport {

Bulletin::Bulletin() {}
Bulletin::~Bulletin() {}

void Bulletin::clear()
{
	datadesc.clear();
	subsets.clear();
	type = subtype = localsubtype = edition = 0;
	rep_year = rep_month = rep_day = rep_hour = rep_minute = rep_second = 0;
	btable = 0;
	dtable = 0;
}

Subset& Bulletin::obtain_subset(unsigned subsection)
{
	while (subsection >= subsets.size())
		subsets.push_back(Subset(btable));
	return subsets[subsection];
}

const Subset& Bulletin::subset(unsigned subsection) const
{
	if (subsection >= subsets.size())
		error_notfound::throwf("Requested subset %u but there are only %zd available",
				subsection, subsets.size());
	return subsets[subsection];
}


BufrBulletin::BufrBulletin()
	: optional_section_length(0), optional_section(0)
{
}

BufrBulletin::~BufrBulletin()
{
	if (optional_section) delete[] optional_section;
}

void BufrBulletin::clear()
{
	Bulletin::clear();
	centre = subcentre = master_table = local_table = 0;
	compression = update_sequence_number = optional_section_length = 0;
	if (optional_section) delete[] optional_section;
	optional_section = 0;
}

void BufrBulletin::load_tables()
{
	char id[30];
	int ce = centre;
	int sc = subcentre;
	int mt = master_table;
	int lt = local_table;

/* fprintf(stderr, "ce %d sc %d mt %d lt %d\n", ce, sc, mt, lt); */

	int found = 0;
	int i;
	for (i = 0; !found && i < 3; ++i)
	{
		if (i == 1)
		{
			// Default to WMO tables if the first
			// attempt with local tables failed
			/* fprintf(stderr, "FALLBACK from %d %d %d %d to 0 %d 0 0\n", sc, ce, mt, lt, mt); */
			ce = sc = lt = 0;
		} else if (i == 2 && mt < 14) {
			// Default to the latest WMO table that
			// we have if the previous attempt has
			// failed
			/* fprintf(stderr, "FALLBACK from %d %d %d %d to 0 14 0 0\n", sc, ce, mt, lt); */
			mt = 14;
		}
		switch (edition)
		{
			case 2:
				sprintf(id, "B%05d%02d%02d", ce, mt, lt);
				if ((found = Vartable::exists(id)))
					break;
			case 3:
				sprintf(id, "B00000%03d%03d%02d%02d",
						0, ce, mt, lt);
				/* Some tables used by BUFR3 are
				 * distributed using BUFR4 names
				 */
				if ((found = Vartable::exists(id)))
					break;
				else
					sc = 0;
			case 4:
				sprintf(id, "B00%03d%04d%04d%03d%03d",
						0, sc, ce, mt, lt);
				found = Vartable::exists(id);
				break;
			default:
				error_consistency::throwf("BUFR edition number is %d but I can only load tables for 3 or 4", edition);
		}
	}

	btable = Vartable::get(id);
	/* TRACE(" -> loaded B table %s\n", id); */

	id[0] = 'D';
	dtable = DTable::get(id);
	/* TRACE(" -> loaded D table %s\n", id); */
}

/*
implemented in bufr_decoder.cc
void BufrBulletin::decode_header(const Rawmsg& raw)
{
}

void BufrBulletin::decode(const Rawmsg& raw)
{
}

implemented in bufr_encoder.cc
void BufrBulletin::encode(std::string& buf) const
{
}
*/

void CrexBulletin::clear()
{
	Bulletin::clear();
	master_table = table = has_check_digit = 0;
}

void CrexBulletin::load_tables()
{
	char id[30];
/* fprintf(stderr, "ce %d sc %d mt %d lt %d\n", ce, sc, mt, lt); */

	sprintf(id, "B%02d%02d%02d", master_table, edition, table);

	btable = Vartable::get(id);
	/* TRACE(" -> loaded B table %s\n", id); */

	id[0] = 'D';
	dtable = DTable::get(id);
	/* TRACE(" -> loaded D table %s\n", id); */
}

/*
implemented in crex_decoder.cc
void CrexBulletin::decode_header(const Rawmsg& raw)
{
}

void CrexBulletin::decode(const Rawmsg& raw)
{
}

implemented in crex_encoder.cc
void CrexBulletin::encode(std::string& buf) const
{
}
*/

void Bulletin::print(FILE* out) const
{
	fprintf(out, "%s ed%d %d:%d:%d %04d-%02d-%02d %02d:%02d:%02d %d subsets\n",
		encoding_name(), edition,
		type, subtype, localsubtype,
		rep_year, rep_month, rep_day, rep_hour, rep_minute, rep_second,
		subsets.size());
	fprintf(out, " Tables: %s %s\n",
		btable ? btable->id().c_str() : "(not loaded)",
		dtable ? dtable->id().c_str() : "(not loaded)");
	fprintf(out, " Data descriptors:\n");
	for (vector<Varcode>::const_iterator i = datadesc.begin(); i != datadesc.end(); ++i)
		fprintf(out, "  %d%02d%03d\n", WR_VAR_F(*i), WR_VAR_X(*i), WR_VAR_Y(*i));
	print_details(out);
	fprintf(out, " Variables:\n");
	for (unsigned i = 0; i < subsets.size(); ++i)
	{
		const Subset& s = subset(i);
		for (unsigned j = 0; j < s.size(); ++j)
		{
			fprintf(out, "  [%d][%d] ", i, j);
			s[j].print(out);
		}
	}
}

void Bulletin::print_details(FILE* out) const {}

void BufrBulletin::print_details(FILE* out) const
{
	fprintf(out, " BUFR details: c%d/%d mt%d lt%d co%d usn%d osl%d\n",
			centre, subcentre, master_table, local_table,
			compression, update_sequence_number, optional_section_length);
}

void CrexBulletin::print_details(FILE* out) const
{
	fprintf(out, " CREX details: T00%02d%02d cd%d\n", master_table, table, has_check_digit);
}

unsigned Bulletin::diff(const Bulletin& msg, FILE* out) const
{
	unsigned diffs = 0;
	if (string(encoding_name()) != string(msg.encoding_name()))
	{
		fprintf(out, "Encodings differ (first is %s, second is %s)\n",
				encoding_name(), msg.encoding_name());
		++diffs;
	} else
		diffs += diff_details(msg, out);
	if (type != msg.type)
	{
		fprintf(out, "Template types differ (first is %d, second is %d)\n",
				type, msg.type);
		++diffs;
	}
	if (subtype != msg.subtype)
	{
		fprintf(out, "Template subtypes differ (first is %d, second is %d)\n",
				subtype, msg.subtype);
		++diffs;
	}
	if (localsubtype != msg.localsubtype)
	{
		fprintf(out, "Template local subtypes differ (first is %d, second is %d)\n",
				localsubtype, msg.localsubtype);
		++diffs;
	}
	if (edition != msg.edition)
	{
		fprintf(out, "Edition numbers differ (first is %d, second is %d)\n",
				edition, msg.edition);
		++diffs;
	}
	if (rep_year != msg.rep_year)
	{
		fprintf(out, "Reference years differ (first is %d, second is %d)\n",
				rep_year, msg.rep_year);
		++diffs;
	}
	if (rep_month != msg.rep_month)
	{
		fprintf(out, "Reference months differ (first is %d, second is %d)\n",
				rep_month, msg.rep_month);
		++diffs;
	}
	if (rep_day != msg.rep_day)
	{
		fprintf(out, "Reference days differ (first is %d, second is %d)\n",
				rep_day, msg.rep_day);
		++diffs;
	}
	if (rep_hour != msg.rep_hour)
	{
		fprintf(out, "Reference hours differ (first is %d, second is %d)\n",
				rep_hour, msg.rep_hour);
		++diffs;
	}
	if (rep_minute != msg.rep_minute)
	{
		fprintf(out, "Reference minutes differ (first is %d, second is %d)\n",
				rep_minute, msg.rep_minute);
		++diffs;
	}
	if (rep_second != msg.rep_second)
	{
		fprintf(out, "Reference seconds differ (first is %d, second is %d)\n",
				rep_second, msg.rep_second);
		++diffs;
	}
	if (btable == NULL && msg.btable != NULL)
	{
		fprintf(out, "First message did not load B btables, second message has %s\n",
				msg.btable->id().c_str());
		++diffs;
	} else if (btable != NULL && msg.btable == NULL) {
		fprintf(out, "Second message did not load B btables, first message has %s\n",
				btable->id().c_str());
		++diffs;
	} else if (btable != NULL && msg.btable != NULL && btable->id() != msg.btable->id()) {
		fprintf(out, "B tables differ (first has %s, second has %s)\n",
				btable->id().c_str(), msg.btable->id().c_str());
		++diffs;
	}
	if (dtable == NULL && msg.dtable != NULL)
	{
		fprintf(out, "First message did not load B dtable, second message has %s\n",
				msg.dtable->id().c_str());
		++diffs;
	} else if (dtable != NULL && msg.dtable == NULL) {
		fprintf(out, "Second message did not load B dtable, first message has %s\n",
				dtable->id().c_str());
		++diffs;
	} else if (dtable != NULL && msg.dtable != NULL && dtable->id() != msg.dtable->id()) {
		fprintf(out, "D tables differ (first has %s, second has %s)\n",
				dtable->id().c_str(), msg.dtable->id().c_str());
		++diffs;
	}

	if (datadesc.size() != msg.datadesc.size())
	{
		fprintf(out, "Data descriptor sections differ (first has %zd elements, second has %zd)\n",
				datadesc.size(), msg.datadesc.size());
		++diffs;
	} else {
		for (unsigned i = 0; i < datadesc.size(); ++i)
			if (datadesc[i] != msg.datadesc[i])
			{
				fprintf(out, "Data descriptors differ at element %u (first has %01d%02d%03d, second has %01d%02d%03d)\n",
						i, WR_VAR_F(datadesc[i]), WR_VAR_X(datadesc[i]), WR_VAR_Y(datadesc[i]),
						WR_VAR_F(msg.datadesc[i]), WR_VAR_X(msg.datadesc[i]), WR_VAR_Y(msg.datadesc[i]));
				++diffs;
			}
	}

	if (subsets.size() != msg.subsets.size())
	{
		fprintf(out, "Number of subsets differ (first is %zd, second is %zd)\n",
				subsets.size(), msg.subsets.size());
		++diffs;
	} else
		for (unsigned i = 0; i < subsets.size(); ++i)
			diffs += subsets[i].diff(msg.subsets[i], out);
	return diffs;
}

unsigned Bulletin::diff_details(const Bulletin& msg, FILE* out) const { return 0; }

unsigned BufrBulletin::diff_details(const Bulletin& msg, FILE* out) const
{
	unsigned diffs = Bulletin::diff_details(msg, out);
	const BufrBulletin* m = dynamic_cast<const BufrBulletin*>(&msg);
	if (!m) throw error_consistency("BufrBulletin::diff_details called with a non-BufrBulletin argument");

	if (centre != m->centre)
	{
		fprintf(out, "BUFR centres differ (first is %d, second is %d)\n",
				centre, m->centre);
		++diffs;
	}
	if (subcentre != m->subcentre)
	{
		fprintf(out, "BUFR subcentres differ (first is %d, second is %d)\n",
				subcentre, m->subcentre);
		++diffs;
	}
	if (master_table != m->master_table)
	{
		fprintf(out, "BUFR master tables differ (first is %d, second is %d)\n",
				master_table, m->master_table);
		++diffs;
	}
	if (local_table != m->local_table)
	{
		fprintf(out, "BUFR local tables differ (first is %d, second is %d)\n",
				local_table, m->local_table);
		++diffs;
	}
	/*
// TODO: uncomment when we implement encoding BUFR with compression
	if (compression != m->compression)
	{
		fprintf(out, "BUFR compression differs (first is %d, second is %d)\n",
				compression, m->compression);
		++diffs;
	}
	*/
	if (update_sequence_number != m->update_sequence_number)
	{
		fprintf(out, "BUFR update sequence numbers differ (first is %d, second is %d)\n",
				update_sequence_number, m->update_sequence_number);
		++diffs;
	}
	if (optional_section_length != m->optional_section_length)
	{
		fprintf(out, "BUFR optional section lenght (first is %d, second is %d)\n",
				optional_section_length, m->optional_section_length);
		++diffs;
	}
	if (optional_section_length != 0)
	{
		if (memcmp(optional_section, m->optional_section, optional_section_length) != 0)
		{
			fprintf(out, "BUFR optional section contents differ\n");
			++diffs;
		}
	}
	return diffs;
}

unsigned CrexBulletin::diff_details(const Bulletin& msg, FILE* out) const
{
	unsigned diffs = Bulletin::diff_details(msg, out);
	const CrexBulletin* m = dynamic_cast<const CrexBulletin*>(&msg);
	if (!m) throw error_consistency("CrexBulletin::diff_details called with a non-CrexBulletin argument");

	if (master_table != m->master_table)
	{
		fprintf(out, "CREX master tables differ (first is %d, second is %d)\n",
				master_table, m->master_table);
		++diffs;
	}
	if (table != m->table)
	{
		fprintf(out, "CREX local tables differ (first is %d, second is %d)\n",
				table, m->table);
		++diffs;
	}
	if (has_check_digit != m->has_check_digit)
	{
		fprintf(out, "CREX has_check_digit differ (first is %d, second is %d)\n",
				has_check_digit, m->has_check_digit);
		++diffs;
	}
	return diffs;
}

static bool seek_past_signature(FILE* fd, const char* sig, unsigned sig_len, const char* fname)
{
	int got = 0;
	int c;

	errno = 0;

	while (got < sig_len && (c = getc(fd)) != EOF)
	{
		if (c == sig[got])
			got++;
		else
			got = 0;
	}

	if (errno != 0)
		if (fname)
			error_system::throwf("looking for start of %.4s data in %s:", sig, fname);
		else
			error_system::throwf("looking for start of %.4s data", sig);
	
	if (got != sig_len)
	{
		/* End of file: return accordingly */
		return false;
	}
	return true;
}

bool BufrBulletin::read(FILE* fd, std::string& buf, const char* fname)
{
	/* A BUFR message is easy to just read: it starts with "BUFR", then the
	 * message length encoded in 3 bytes */

	// Reset bufr_message data in case this message has been used before
	buf.clear();

	/* Seek to start of BUFR data */
	if (!seek_past_signature(fd, "BUFR", 4, fname))
		return false;
	buf += "BUFR";

	// Read the remaining 4 bytes of section 0
	buf.resize(8);
	if (fread((char*)buf.data() + 4, 4, 1, fd) != 1)
	{
		if (fname)
			error_system::throwf("reading BUFR section 0 from %s", fname);
		else
			throw error_system("reading BUFR section 0 from");
	}

	/* Read the message length */
	int bufrlen = ntohl(*(uint32_t*)(buf.data()+4)) >> 8;
	if (bufrlen < 12)
	{
		if (fname)
			error_consistency::throwf("%s: the size declared by the BUFR message (%d) is less than the minimum of 12", fname, bufrlen);
		else
			error_consistency::throwf("the size declared by the BUFR message (%d) is less than the minimum of 12", bufrlen);
	}

	/* Allocate enough space to fit the message */
	buf.resize(bufrlen);

	/* Read the rest of the BUFR message */
	if (fread((char*)buf.data() + 8, bufrlen - 8, 1, fd) != 1)
	{
		if (fname)
			error_system::throwf("reading BUFR message from %s", fname);
		else
			throw error_system("reading BUFR message");
	}

	return true;
}

void BufrBulletin::write(const std::string& buf, FILE* out, const char* fname)
{
	if (fwrite(buf.data(), buf.size(), 1, out) != 1)
	{
		if (fname)
			error_system::throwf("%s: writing %zd bytes", fname, buf.size());
		else
			error_system::throwf("writing %zd bytes", buf.size());
	}
}

bool CrexBulletin::read(FILE* fd, std::string& buf, const char* fname)
{
/*
 * The CREX message starts with "CREX" and ends with "++\r\r\n7777".  Ideally
 * any combination of \r and \n should be supported.
 */
	/* Reset crex_message data in case this message has been used before */
	buf.clear();

	/* Seek to start of CREX data */
	if (!seek_past_signature(fd, "CREX++", 6, fname))
		return false;
	buf += "CREX++";

	/* Read until "\+\+(\r|\n)+7777" */
	{
		const char* target = "++\r\n7777";
		static const int target_size = 8;
		int got = 0;
		int c;

		errno = 0;
		while (got < 8 && (c = getc(fd)) != EOF)
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

			buf += (char)c;
		}
		if (errno != 0)
		{
			if (fname)
				error_system::throwf("looking for end of CREX data in %s", fname);
			else
				throw error_system("looking for end of CREX data");
		}

		if (got != target_size)
		{
			if (fname)
				throw error_parse(fname, ftell(fd), "CREX message is incomplete");
			else
				throw error_parse("(unknown)", ftell(fd), "CREX message is incomplete");
		}
	}

	return true;
}

void CrexBulletin::write(const std::string& buf, FILE* out, const char* fname)
{
	if (fwrite(buf.data(), buf.size(), 1, out) != 1)
	{
		if (fname)
			error_system::throwf("%s: writing %zd bytes", fname, buf.size());
		else
			error_system::throwf("writing %zd bytes", buf.size());
	}
	if (fputs("\r\r\n", out) == EOF)
	{
		if (fname)
			error_system::throwf("writing CREX data on %s", fname);
		else
			throw error_system("writing CREX data");
	}
}


#if 0
std::auto_ptr<Bulletin> Bulletin::create(dballe::Encoding encoding)
{
	std::auto_ptr<bufrex::Bulletin> res;
	switch (encoding)
	{
		case BUFR: res.reset(new BufrBulletin); break;
		case CREX: res.reset(new CrexBulletin); break;
		default: error_consistency::throwf("the bufrex library does not support encoding %s (only BUFR and CREX are supported)",
					 encoding_name(encoding));
	}
	return res;
}
#endif

#if 0
dba_err bufrex_msg_generate_datadesc(bufrex_msg msg)
{
	bufrex_subset subset;
	int i;

	if (msg->subsets_count == 0)
		return dba_error_consistency("tried to guess data description section from a bufrex_msg without subsets");
	subset = msg->subsets[0];

	for (i = 0; i < subset->vars_count; ++i)
	{
		dba_varcode code = dba_var_code(subset->vars[i]);
		if (msg->subsets_count != 1 && WR_VAR_X(code) == 31)
			return dba_error_unimplemented("autogenerating data description sections from a variable list that contains delayed replication counts");
		DBA_RUN_OR_RETURN(bufrex_msg_append_datadesc(msg, code));
	}
	return dba_error_ok();
}

dba_err bufrex_msg_decode_header(bufrex_msg msg, dba_rawmsg raw)
{
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: return bufr_decoder_decode_header(raw, msg);
		case BUFREX_CREX: return crex_decoder_decode_header(raw, msg);
	}
	return dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
}

dba_err bufrex_msg_decode(bufrex_msg msg, dba_rawmsg raw)
{
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: return bufr_decoder_decode(raw, msg);
		case BUFREX_CREX: return crex_decoder_decode(raw, msg);
	}
	return dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
}

dba_err bufrex_msg_encode(bufrex_msg msg, dba_rawmsg* raw)
{
	dba_err err;

	DBA_RUN_OR_RETURN(dba_rawmsg_create(raw));
	
	switch (msg->encoding_type)
	{
		case BUFREX_BUFR: DBA_RUN_OR_GOTO(fail, bufr_encoder_encode(msg, *raw)); break;
		case BUFREX_CREX: DBA_RUN_OR_GOTO(fail, crex_encoder_encode(msg, *raw)); break;
		default:
			err = dba_error_consistency("Got invalid encoding type %d", msg->encoding_type);
			goto fail;
	}

	return dba_error_ok();

fail:
	if (*raw != NULL)
		dba_rawmsg_delete(*raw);
	return err;
}
#endif

#if 0
#include <dballe/dba_check.h>

#ifdef HAVE_CHECK

#include <string.h> /* strcmp */

void test_bufrex_msg()
{
	/* dba_err err; */
	int val, val1;
	dba_var* vars;
	bufrex_msg msg;

	CHECKED(bufrex_msg_create(&msg));

	/* Resetting things should not fail */
	bufrex_msg_reset(msg);

	/* The message should be properly empty */
	CHECKED(bufrex_msg_get_category(msg, &val, &val1));
	fail_unless(val == 0);
	fail_unless(val1 == 0);
	CHECKED(bufrex_msg_get_vars(msg, &vars, &val));
	fail_unless(vars == 0);
	fail_unless(val == 0);

	CHECKED(bufrex_msg_set_category(msg, 42, 24));
	CHECKED(bufrex_msg_get_category(msg, &val, &val1));
	fail_unless(val == 42);
	fail_unless(val1 == 24);

	{
		dba_varinfo info;
		dba_var v;
		CHECKED(dba_varinfo_query_local(WR_VAR(0, 1, 2), &info));
		CHECKED(dba_var_createi(info, &v, 7));
		CHECKED(bufrex_msg_store_variable(msg, v));
	}

	CHECKED(bufrex_msg_get_vars(msg, &vars, &val));
	fail_unless(vars != 0);
	fail_unless(vars[0] != 0);
	fail_unless(val == 1);
	fail_unless(dba_var_code(vars[0]) == WR_VAR(0, 1, 2));
	fail_unless(strcmp(dba_var_value(vars[0]), "7") == 0);

	bufrex_msg_delete(msg);
}

#endif
#endif

}

/* vim:set ts=4 sw=4: */
