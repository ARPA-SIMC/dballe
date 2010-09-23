/*
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

/* For %zd */
#define _ISOC99_SOURCE

#include <dballe/init.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/aof_codec.h>
#include <dballe/core/record.h>
#include <dballe/core/file.h>
#include <dballe/core/aoffile.h>
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <dballe/cmdline/cmdline.h>
#include <dballe/cmdline/processor.h>
#include <dballe/cmdline/conversion.h>

#include <stdlib.h>

#include <dballe/bufrex/opcode.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace wreport;
using namespace dballe;
using namespace dballe::cmdline;
using namespace std;

static int op_dump_interpreted = 0;
static int op_dump_text = 0;
static const char* op_input_type = "auto";
static const char* op_output_type = "bufr";
static const char* op_output_template = "";
static const char* op_report = "";
static const char* op_bisect_cmd = NULL;
int op_verbose = 0;

struct cmdline::grep_t grepdata = { -1, -1, -1, 0, 0, "" };
struct poptOption grepTable[] = {
	{ "category", 0, POPT_ARG_INT, &grepdata.category, 0,
		"match messages with the given data category", "num" },
	{ "subcategory", 0, POPT_ARG_INT, &grepdata.subcategory, 0,
		"match BUFR messages with the given data subcategory", "num" },
	{ "check-digit", 0, POPT_ARG_INT, &grepdata.checkdigit, 0,
		"match CREX messages with check digit (if 1) or without check digit (if 0)", "num" },
	{ "unparsable", 0, 0, &grepdata.unparsable, 0,
		"match only messages that cannot be parsed" },
	{ "parsable", 0, 0, &grepdata.parsable, 0,
		"match only messages that can be parsed" },
	{ "index", 0, POPT_ARG_STRING, &grepdata.index, 0,
		"match messages with the index in the given range (ex.: 1-5,9,22-30)", "expr" },
	POPT_TABLEEND
};

volatile int flag_bisect_stop = 0;
void stop_bisect(int sig)
{
	/* The signal handler just clears the flag and re-enables itself. */
	flag_bisect_stop = 1;
	signal(sig, stop_bisect);
}

static int count_nonnulls(const Subset& raw)
{
	unsigned i, count = 0;
	for (i = 0; i < raw.size(); i++)
		if (raw[i].value() != NULL)
			count++;
	return count;
}

static void dump_bufr_header(const Rawmsg& rmsg, const BufrBulletin& braw)
{
	printf("Message %d\n", rmsg.index);
	printf("Size: %zd\n", rmsg.size());
	printf("Edition: %d\n", braw.edition);
	printf("Centre: %d:%d\n", braw.centre, braw.subcentre);
	printf("Category: %d:%d:%d\n", braw.type, braw.subtype, braw.localsubtype);
	printf("Datetime: %04d-%02d-%02d %02d:%02d:%02d\n",
			braw.rep_year, braw.rep_month, braw.rep_day,
			braw.rep_hour, braw.rep_minute, braw.rep_second);
	printf("Tables: %d:%d\n", braw.master_table, braw.local_table);
	printf("Table: %s\n", braw.btable ? braw.btable->id().c_str() : "(none)");
	printf("Compression: %s\n", braw.compression ? "yes" : "no");
	printf("Update sequence number: %d\n", braw.update_sequence_number);
	printf("Optional section length: %d\n", braw.optional_section_length);
	printf("Subsets: %zd\n\n", braw.subsets.size());

	// Copy data descriptor section
	//for (bufrex_opcode i = orig->datadesc; i != NULL; i = i->next)
		//DBA_RUN_OR_GOTO(cleanup, bufrex_msg_append_datadesc(msg, i->val));
}

static void dump_crex_header(const Rawmsg& rmsg, const CrexBulletin& braw)
{
	printf("Message %d\n", rmsg.index);
	printf("Size: %zd\n", rmsg.size());
	printf("Edition: %d\n", braw.edition);
	printf("Category: %d:%d:%d\n", braw.type, braw.subtype, braw.localsubtype);
	printf("Datetime: %04d-%02d-%02d %02d:%02d:%02d\n",
			braw.rep_year, braw.rep_month, braw.rep_day,
			braw.rep_hour, braw.rep_minute, braw.rep_second);
	printf("Tables: %d:%d\n", braw.master_table, braw.table);
	printf("Table: %s\n", braw.btable ? braw.btable->id().c_str() : "(none)");
	printf("Check digit: %s\n\n", braw.has_check_digit ? "yes" : "no");
}

static void dump_aof_header(const Rawmsg& rmsg)
{
	int category, subcategory;
	msg::AOFImporter::get_category(rmsg, &category, &subcategory);
	/* DBA_RUN_OR_RETURN(bufrex_message_get_vars(msg, &vars, &count)); */

	printf("Message %d\n", rmsg.index);
	printf("Size: %zd\n", rmsg.size());
	printf("Category: %d:%d\n\n", category, subcategory);
}

static void print_bufr_header(const Rawmsg& rmsg, const BufrBulletin& braw)
{
	printf("#%d BUFR message: %zd bytes, category %d:%d:%d, table %s, subsets %zd, values:",
			rmsg.index, rmsg.size(), braw.type, braw.subtype, braw.localsubtype,
			braw.btable ? braw.btable->id().c_str() : "(none)",
			braw.subsets.size());
	for (size_t i = 0; i < braw.subsets.size(); ++i)
		printf(" %d/%zd", count_nonnulls(braw.subsets[i]), braw.subsets[i].size());
}

static void print_crex_header(const Rawmsg& rmsg, const CrexBulletin& braw)
{
	/* DBA_RUN_OR_RETURN(crex_message_has_check_digit(msg, &checkdigit)); */

#if 0
	printf("#%d CREX message: %d bytes, category %d, subcategory %d, table %s, %scheck digit, %d/%d values",
			rmsg->index, size, braw->type, braw->subtype, table_id, /*checkdigit ? "" : "no "*/"? ", count_nonnulls(braw), braw->vars_count);
#endif

	printf("#%d CREX message: %zd bytes, category %d, subcategory %d, table %s, subsets %zd, values:",
			rmsg.index, rmsg.size(), braw.type, braw.subtype,
			braw.btable ? braw.btable->id().c_str() : "(none)",
			braw.subsets.size());
	for (size_t i = 0; i < braw.subsets.size(); ++i)
		printf(" %d/%zd", count_nonnulls(braw.subsets[i]), braw.subsets[i].size());
}

static void print_aof_header(const Rawmsg& rmsg)
{
	int category, subcategory;
	msg::AOFImporter::get_category(rmsg, &category, &subcategory);
	/* DBA_RUN_OR_RETURN(bufrex_message_get_vars(msg, &vars, &count)); */

	printf("#%d AOF message: %zd bytes, category %d, subcategory %d",
			rmsg.index, rmsg.size(), category, subcategory);
}

struct Summarise : public cmdline::Action
{
	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		switch (rmsg.encoding)
		{
			case BUFR:
				if (braw == NULL) return;
				print_bufr_header(rmsg, *dynamic_cast<const BufrBulletin*>(braw)); puts(".");
				break;
			case CREX:
				if (braw == NULL) return;
				print_crex_header(rmsg, *dynamic_cast<const CrexBulletin*>(braw)); puts(".");
				break;
			case AOF:
				print_aof_header(rmsg); puts(".");
				break;
		}
	}
};

struct Head : public cmdline::Action
{
	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		switch (rmsg.encoding)
		{
			case BUFR:
				if (braw == NULL) return;
				dump_bufr_header(rmsg, *dynamic_cast<const BufrBulletin*>(braw)); puts(".");
				break;
			case CREX:
				if (braw == NULL) return;
				dump_crex_header(rmsg, *dynamic_cast<const CrexBulletin*>(braw)); puts(".");
				break;
			case AOF:
				dump_aof_header(rmsg);
				break;
		}
	}
};

static void dump_dba_vars(const Subset& msg)
{
	for (size_t i = 0; i < msg.size(); ++i)
		msg[i].print(stdout);
}

struct DumpMessage : public cmdline::Action
{
	void print_subsets(const Bulletin& braw)
	{
		for (size_t i = 0; i < braw.subsets.size(); ++i)
		{
			printf("Subset %zd:\n", i);
			dump_dba_vars(braw.subsets[i]);
		}
	}

	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		switch (rmsg.encoding)
		{
			case BUFR:
			{
				if (braw == NULL) return;
				const BufrBulletin& b = *dynamic_cast<const BufrBulletin*>(braw);
				print_bufr_header(rmsg, b); puts(":");
				printf(" Edition %d, origin %d/%d, master table %d, local table %d\n",
						b.edition, b.centre, b.subcentre, b.master_table, b.local_table);
				print_subsets(*braw);
				break;
			}
			case CREX:
			{
				if (braw == NULL) return;
				const CrexBulletin& b = *dynamic_cast<const CrexBulletin*>(braw);
				print_crex_header(rmsg, b); puts(":");
				printf(" Edition %d, master table %d, table %d\n",
						b.edition, b.master_table, b.table);
				print_subsets(*braw);
				break;
			}
			case AOF:
				print_aof_header(rmsg); puts(":");
				msg::AOFImporter::dump(rmsg, stdout);
				break;
		}
	}
};

struct DumpCooked : public cmdline::Action
{
	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		if (msgs == NULL) return;
		for (size_t i = 0; i < msgs->size(); ++i)
		{
			printf("#%d[%zd] ", rmsg.index, i);
			(*msgs)[i]->print(stdout);
		}
	}
};

static void print_var(const Var& var)
{
	printf("B%02d%03d", WR_VAR_X(var.code()), WR_VAR_Y(var.code()));
	if (var.value() != NULL)
	{
		if (var.info()->is_string())
		{
			printf(" %s\n", var.value());
		} else {
			double value = var.enqd();;
			printf(" %.*f\n", var.info()->scale > 0 ? var.info()->scale : 0, value);
		}
	} else
		printf("\n");
}

struct DumpText : public cmdline::Action
{
	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		if (braw == NULL)
			throw error_consistency("source is not a BUFR or CREX message");
		const BufrBulletin* b = dynamic_cast<const BufrBulletin*>(braw);
		if (!b) throw error_consistency("source is not BUFR");
		printf("Edition: %d\n", b->edition);
		printf("Type: %d\n", b->type);
		printf("Subtype: %d\n", b->subtype);
		printf("Localsubtype: %d\n", b->localsubtype);
		printf("Centre: %d\n", b->centre);
		printf("Subcentre: %d\n", b->subcentre);
		printf("Mastertable: %d\n", b->master_table);
		printf("Localtable: %d\n", b->local_table);
		printf("Compression: %d\n", b->compression);
		printf("Reftime: %04d-%02d-%02d %02d:%02d:%02d\n",
				b->rep_year, b->rep_month, b->rep_day,
				b->rep_hour, b->rep_minute, b->rep_second);
		printf("Descriptors:");
		for (vector<Varcode>::const_iterator i = b->datadesc.begin();
				i != b->datadesc.end(); ++i)
		{
			char type;
			switch (WR_VAR_F(*i))
			{
				case 0: type = 'B'; break;
				case 1: type = 'R'; break;
				case 2: type = 'C'; break;
				case 3: type = 'D'; break;
				default: type = '?'; break;
			}
			printf(" %c%02d%03d", type, WR_VAR_X(*i), WR_VAR_Y(*i));
		}
		printf("\n");
		for (size_t i = 0; i < b->subsets.size(); ++i)
		{
			const Subset& subset = b->subsets[i];
			printf("Data:\n");
			for (size_t j = 0; j < subset.size(); ++j)
			{
				const Var& var = subset[i];
				printf(" ");
				print_var(var);
				for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
				{
					printf(" *");
					print_var(*attr);
				}
			}
		}
	}
};

struct WriteRaw : public cmdline::Action
{
	File* file;
	WriteRaw() : file(0) {}
	~WriteRaw() { if (file) delete file; }

	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		if (!file) file = File::create(rmsg.encoding, "(stdout)", "w");
		file->write(rmsg);
	}
};

int do_scan(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	Summarise s;
	process_all(optCon, dba_cmdline_stringToMsgType(op_input_type, optCon), &grepdata, s);
	return 0;
}

int do_head(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	Head head;
	process_all(optCon, dba_cmdline_stringToMsgType(op_input_type, optCon), &grepdata, head);
	return 0;
}

int do_dump(poptContext optCon)
{
	auto_ptr<cmdline::Action> action;
	if (op_dump_interpreted)
		action.reset(new DumpCooked);
	else if (op_dump_text)
		action.reset(new DumpText);
	else
		action.reset(new DumpMessage);

	/* Throw away the command name */
	poptGetArg(optCon);
	process_all(optCon, dba_cmdline_stringToMsgType(op_input_type, optCon), &grepdata, *action);
	return 0;
}

int do_cat(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	/*DBA_RUN_OR_RETURN(aof_file_write_header(file, 0, 0)); */
	WriteRaw wraw;
	process_all(optCon, dba_cmdline_stringToMsgType(op_input_type, optCon), &grepdata, wraw);
	return 0;
}

struct StoreMessages : public cmdline::Action, public vector<Rawmsg>
{
	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs)
	{
		push_back(rmsg);
	}
};

#if 0
static dba_err bisect_test(struct message_vector* vec, size_t first, size_t last, int* fails)
{
	FILE* out = popen(op_bisect_cmd, "w");
	int res;
	for (; first < last; ++first)
	{
		dba_rawmsg msg = vec->messages[first];
		if (fwrite(msg->buf, msg->len, 1, out) == 0)
			return dba_error_system("writing message %d to test script", msg->index);
	}
	res = pclose(out);
	if (res == -1)
		return dba_error_system("running test script", first);
	*fails = (res != 0);
	return dba_error_ok();
}

struct bisect_candidate
{
	size_t first;
	size_t last;
};

static dba_err bisect(
	struct bisect_candidate* cand,
   	struct message_vector* vec,
   	size_t first, size_t last)
{
	int fails = 0;

	/* If we already narrowed it down to 1 messages, there is no need to test
	 * further */
	if (flag_bisect_stop || cand->last == cand->first + 1)
		return dba_error_ok();

	if (op_verbose)
		fprintf(stderr, "Trying messages %zd-%zd (%zd selected, kill -HUP %d to stop)... ", first, last, cand->last - cand->first, getpid());

	DBA_RUN_OR_RETURN(bisect_test(vec, first, last, &fails));

	if (op_verbose)
		fprintf(stderr, fails ? "fail.\n" : "ok.\n");

	if (fails)
	{
		size_t mid = (first + last) / 2;
		if (last-first < cand->last - cand->first)
		{
			cand->last = last;
			cand->first = first;
		}
		if (first < mid && mid != last) DBA_RUN_OR_RETURN(bisect(cand, vec, first, mid));
		if (mid < last && mid != first) DBA_RUN_OR_RETURN(bisect(cand, vec, mid, last));
	}

	return dba_error_ok();
}
#endif

int do_bisect(poptContext optCon)
{
#if 0
	struct message_vector vec = { 0, 0, 0 };
	struct bisect_candidate candidate;
	int old_op_verbose = op_verbose;
	size_t i;

	/* Throw away the command name */
	poptGetArg(optCon);

	if (op_bisect_cmd == NULL)
		return dba_error_consistency("you need to use --test=command");

	/* Read all input messages a vector of dba_rawmsg */
	op_verbose = 0;
	DBA_RUN_OR_RETURN(process_all(optCon, 
				dba_cmdline_stringToMsgType(op_input_type, optCon),
				&grepdata, store_messages, &vec));
	op_verbose = old_op_verbose;

	/* Establish a handler for SIGHUP signals. */
	signal(SIGHUP, stop_bisect);

	/* Bisect working on the vector */
	candidate.first = 0;
	candidate.last = vec.len;
	DBA_RUN_OR_RETURN(bisect(&candidate, &vec, candidate.first, candidate.last));

	if (op_verbose)
	{
		if (flag_bisect_stop)
			fprintf(stderr, "Stopped by SIGHUP.\n");
		fprintf(stderr, "Selected messages %zd-%zd.\n", candidate.first, candidate.last);
	}

	/* Output the candidate messages */
	for (; candidate.first < candidate.last; ++candidate.first)
	{
		dba_rawmsg msg = vec.messages[candidate.first];
		if (fwrite(msg->buf, msg->len, 1, stdout) == 0)
			return dba_error_system("writing message %d to standard output", msg->index);
	}

	for (i = 0; i < vec.len; ++i)
		dba_rawmsg_delete(vec.messages[i]);
	free(vec.messages);

	return dba_error_ok();
#endif
	throw error_unimplemented("bisect is currently not implemented");
}

int do_convert(poptContext optCon)
{
	msg::Exporter::Options opts;
	cmdline::Converter conv;

	/* Throw away the command name */
	poptGetArg(optCon);

	if (strcmp(op_output_template, "list") == 0)
	{
		list_templates();
		return 0;
	}

	Encoding intype = dba_cmdline_stringToMsgType(op_input_type, optCon);
	Encoding outtype = dba_cmdline_stringToMsgType(op_output_type, optCon);

	if (op_report[0] != 0)
		conv.dest_rep_memo = op_report;
	else
		conv.dest_rep_memo = NULL;

	if (op_output_template[0] != 0)
	{
		conv.dest_template = op_output_template;
		opts.template_name = op_output_template;
	}

	conv.file = File::create(outtype, "(stdout)", "w");
	conv.importer = msg::Importer::create(intype).release();
	conv.exporter = msg::Exporter::create(outtype, opts).release();

	process_all(optCon, intype, &grepdata, conv);

	return 0;
}

int do_compare(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	/* Read the file names */
	const char* file1_name = poptGetArg(optCon);
	if (file1_name == NULL)
		dba_cmdline_error(optCon, "input file needs to be specified");

	const char* file2_name = poptGetArg(optCon);
	if (file2_name == NULL)
		file2_name = "(stdin)";

	Encoding in_type = dba_cmdline_stringToMsgType(op_input_type, optCon);
	Encoding out_type = dba_cmdline_stringToMsgType(op_output_type, optCon);
	File* file1 = File::create(in_type, file1_name, "r");
	File* file2 = File::create(out_type, file2_name, "r");
	std::auto_ptr<msg::Importer> importer = msg::Importer::create(in_type);
	std::auto_ptr<msg::Exporter> exporter = msg::Exporter::create(out_type);
	size_t idx = 0;
	for ( ; ; ++idx)
	{
		++idx;

		Rawmsg msg1;
		Rawmsg msg2;
		bool found1 = file1->read(msg1);
		bool found2 = file2->read(msg2);

		if (found1 != found2)
			throw error_consistency("The files contain a different number of messages");
		if (!found1 && !found2)
			break;

		Msgs msgs1;
		Msgs msgs2;
		importer->from_rawmsg(msg1, msgs1);
		importer->from_rawmsg(msg2, msgs2);

		int diffs = msgs1.diff(msgs2, stderr);
		if (diffs > 0)
			error_consistency::throwf("Messages #%zd contain %d differences", idx, diffs);
	}
	if (idx == 0)
		throw error_consistency("The files do not contain messages");
	return 0;
}

#if 0
struct filter_data
{
	dba_file output;
	int mindate[6];
	int maxdate[6];
	double latmin, latmax, lonmin, lonmax;
	dba_var varfilter[20];
};

inline static int get_date_value(dba_msg msg, int value)
{
	dba_var var = dba_msg_find_by_id(msg, value);
	const char* val = var == NULL ? NULL : dba_var_value(var);
	if (val == NULL)
		return -1;
	else
		return strtoul(val, 0, 10);
}

inline static double get_lat_value(dba_msg msg, int value)
{
	double res;
	dba_var var = dba_msg_find_by_id(msg, value);
	if (var == NULL || dba_var_value(var) == NULL)
		return -1000000;
	dba_var_enqd(var, &res);
	return res;
}

dba_err filter_message(dba_rawmsg rmsg, bufrex_msg braw, dba_msgs msgs, void* data)
{
	struct filter_data* fdata = (struct filter_data*)data;
	double dval;
	int msgs_reject;
	int i, j, k;

	if (msgs == NULL) return dba_error_ok();

	for (i = 0; i < 20 && fdata->varfilter[i] != NULL; ++i)
	{
		dba_var test = fdata->varfilter[i];
		int found = 0;
		for (j = 0; !found && j < braw->subsets_count; ++j)
		{
			for (k = 0; !found && k < braw->subsets[j]->vars_count; ++k)
			{
				dba_var t = braw->subsets[j]->vars[k];
				if (dba_var_code(test) == dba_var_code(t)
				 && strcmp(dba_var_value(test), dba_var_value(t)) == 0)
					found = 1;
			}
		}
		if (!found)
			return dba_error_ok();
	}

	msgs_reject = 1;
	for (i = 0; i < msgs->len; ++i)
	{
		dba_msg msg = msgs->msgs[i];
		int msg_reject = 0;

		if (fdata->mindate[0] != -1 || fdata->maxdate[0] != -1)
		{
			int i, date[6];
			date[0] = get_date_value(msg, DBA_MSG_YEAR);
			date[1] = get_date_value(msg, DBA_MSG_MONTH);
			date[2] = get_date_value(msg, DBA_MSG_DAY);
			date[3] = get_date_value(msg, DBA_MSG_HOUR);
			date[4] = get_date_value(msg, DBA_MSG_MINUTE);
			date[5] = 0;

			/* Check that the message contains proper datetime indications */
			for (i = 0; i < 6; i++)
				if (date[i] == -1)
					msg_reject = 1;

			/* Compare with the two extremes */

			if (fdata->mindate[0] != -1)
				for (i = 0; !msg_reject && i < 6; i++)
					if (fdata->mindate[i] > date[i])
						msg_reject = 1;

			if (fdata->maxdate[0] != -1)
				for (i = 0; !msg_reject && i < 6; i++)
					if (fdata->maxdate[i] < date[i])
						msg_reject = 1;
		}

		/* Latitude and longitude must exist */
		if (!msg_reject && (dval = get_lat_value(msg, DBA_MSG_LATITUDE)) == -1000000)
			msg_reject = 1;
		if (!msg_reject && fdata->latmin != -1000000 && fdata->latmin > dval)
			msg_reject = 1;
		if (!msg_reject && fdata->latmax != -1000000 && fdata->latmax < dval)
			msg_reject = 1;

		if (!msg_reject && (dval = get_lat_value(msg, DBA_MSG_LONGITUDE)) == -1000000)
			msg_reject = 1;
		if (!msg_reject && fdata->lonmin != -1000000 && fdata->lonmin > dval)
			msg_reject = 1;
		if (!msg_reject && fdata->lonmax != -1000000 && fdata->lonmax < dval)
			msg_reject = 1;

		if (!msg_reject)
		{
			msgs_reject = 0;
			break;
		}
	}

	if (!msgs_reject)
		DBA_RUN_OR_RETURN(dba_file_write(fdata->output, rmsg));

	return dba_error_ok();
}

inline static double get_input_lat_value(dba_record rec, dba_keyword key)
{
	dba_var var = dba_record_key_peek(rec, key); 
	double res;
	if (var == NULL)
		return -1000000;
	if (dba_var_value(var) == NULL)
		return -1000000;
	dba_var_enqd(var, &res);
	return res;
}
#endif

int do_filter(poptContext optCon)
{
	throw error_unimplemented("filter not implemented");
#if 0
	dba_encoding type;
	dba_encoding otype;
	struct filter_data fdata;
	dba_record query;
	memset(&fdata, 0, sizeof(fdata));
	fdata.varfilter[0] = NULL;
	dba_record_cursor c;
	int i;

	/* Throw away the command name */
	poptGetArg(optCon);

	/* Create the query */
	DBA_RUN_OR_RETURN(dba_record_create(&query));
	DBA_RUN_OR_RETURN(dba_cmdline_get_query(optCon, query));

	/* Digest the query in good values for filter_data */
	DBA_RUN_OR_RETURN(dba_record_parse_date_extremes(query, fdata.mindate, fdata.maxdate));
	fdata.latmin = get_input_lat_value(query, DBA_KEY_LATMIN);
	fdata.latmax = get_input_lat_value(query, DBA_KEY_LATMAX);
	fdata.lonmin = get_input_lat_value(query, DBA_KEY_LONMIN);
	fdata.lonmax = get_input_lat_value(query, DBA_KEY_LONMAX);

	for (i = 0, c = dba_record_iterate_first(query);
			c != NULL && i < 20; c = dba_record_iterate_next(query, c), ++i)
		fdata.varfilter[i] = dba_record_cursor_variable(c);

	type = dba_cmdline_stringToMsgType(op_input_type, optCon);
	otype = dba_cmdline_stringToMsgType(op_output_type, optCon);

	DBA_RUN_OR_RETURN(dba_file_create(otype, "(stdout)", "w", &fdata.output));
	DBA_RUN_OR_RETURN(process_all(optCon, type, &grepdata, filter_message, &fdata));
	/*DBA_RUN_OR_RETURN(aof_file_write_header(file, 0, 0)); */
	dba_file_delete(fdata.output);

	return dba_error_ok();
#endif
}

int do_fixaof(poptContext optCon)
{
	/* Throw away the command name */
	poptGetArg(optCon);

	int count = 0;
	while (const char* filename = poptGetArg(optCon))
	{
		auto_ptr<File> file(File::create(AOF, filename, "rb+"));
		AofFile* aoffile = dynamic_cast<AofFile*>(file.get());
		aoffile->fix_header();
		++count;
	}

	if (count == 0)
		dba_cmdline_error(optCon, "at least one input file needs to be specified");

	return 0;
}

#if 0
static dba_err readfield(FILE* in, char** name, char** value)
{
	static char line[1000];
	char* s;

	if (fgets(line, 1000, in) == NULL)
	{
		*name = *value = NULL;
		return dba_error_ok();
	}

	s = strchr(line, ':');
	if (s == NULL)
	{
		*name = NULL;
		*value = line;
	}
	else
	{
		*s = 0;
		*name = line;
		*value = s + 1;
	}

	if (*value)
	{
		int len;
		/* Trim value */
		while (**value && isspace(**value))
			++*value;

		len = strlen(*value);
		while (len > 0 && isspace((*value)[len-1]))
		{
			--len;
			(*value)[len] = 0;
		}
	}

	return dba_error_ok();
}
#endif

#if 0
static dba_err parsetextgrib(FILE* in, bufrex_msg msg, int* found)
{
	dba_err err = DBA_OK;
	bufrex_subset subset = NULL;
	char* name;
	char* value;
	dba_var var = NULL;

	*found = 0;
	bufrex_msg_reset(msg);

	while (1)
	{
		DBA_RUN_OR_GOTO(cleanup, readfield(in, &name, &value));
		/* fprintf(stderr, "GOT NAME %s VALUE \"%s\"\n", name, value); */
		if (name != NULL)
		{
			if (strcasecmp(name, "edition") == 0) {
				msg->edition = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "type") == 0) {
				msg->type = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "subtype") == 0) {
				msg->subtype = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "localsubtype") == 0) {
				msg->localsubtype = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "centre") == 0) {
				msg->opt.bufr.centre = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "subcentre") == 0) {
				msg->opt.bufr.subcentre = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "mastertable") == 0) {
				msg->opt.bufr.master_table = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "localtable") == 0) {
				msg->opt.bufr.local_table = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "compression") == 0) {
				msg->opt.bufr.compression = strtoul(value, 0, 10);
			} else if (strcasecmp(name, "reftime") == 0) {
				if (sscanf(value, "%04d-%02d-%02d %02d:%02d:%02d",
					&msg->rep_year, &msg->rep_month, &msg->rep_day,
					&msg->rep_hour, &msg->rep_minute, &msg->rep_second) != 6)
					return dba_error_consistency("Reference time \"%s\" cannot be parsed", value);
			} else if (strcasecmp(name, "descriptors") == 0) {
				const char* s = value;
				DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(msg));
				while (1)
				{
					size_t size = strcspn(s, " \t");
					s += size;
					size = strspn(s, "BCDR0123456789");
					if (size == 0)
						break;
					else
						DBA_RUN_OR_GOTO(cleanup, bufrex_msg_append_datadesc(msg, dba_descriptor_code(s)));
				}
			} else if (strcasecmp(name, "data") == 0) {
				/* Start a new subset */
				DBA_RUN_OR_GOTO(cleanup, bufrex_msg_get_subset(msg, msg->subsets_count, &subset));
				*found = 1;
			} 
		} else if (value != NULL) {
			dba_varinfo info;
			int isattr = 0;
			dba_varcode code;
			if (value[0] == 0)
				/* End of one message */
				break;

			/* Read a Bsomething (value or attribute) and append it to the subset */
			if (value[0] == '*')
			{
				isattr = 1;
				++value;
			}

			code = dba_descriptor_code(value);
			DBA_RUN_OR_GOTO(cleanup, bufrex_msg_query_btable(msg, code, &info));
			while (*value && !isspace(*value))
				++value;
			while (*value && isspace(*value))
				++value;
			if (*value == 0)
			{
				/* Undef */
				DBA_RUN_OR_GOTO(cleanup, dba_var_create(info, &var));
			} else {
				if (VARINFO_IS_STRING(info))
				{
					DBA_RUN_OR_GOTO(cleanup, dba_var_createc(info, value, &var));
				} else {
					DBA_RUN_OR_GOTO(cleanup, dba_var_created(info, strtod(value, NULL), &var));
				}
			}
			if (isattr)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_add_attr(subset, var));
				dba_var_delete(var);
			}
			else
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_subset_store_variable(subset, var));
			}
			var = NULL;
		} else {
			/* End of input */
			break;
		}
	}

cleanup:
	if (var)
		dba_var_delete(var);
	return err == DBA_OK ? dba_error_ok() : err;
}
#endif

int do_makebufr(poptContext optCon)
{
	throw error_unimplemented("makebufr not implemented");
#if 0
	dba_err err = DBA_OK;
	bufrex_msg msg = NULL;
	dba_rawmsg rmsg = NULL;
	dba_file outfile = NULL;
	const char* filename;
	FILE* in = NULL;
	int count = 0;

	DBA_RUN_OR_RETURN(bufrex_msg_create(BUFREX_BUFR, &msg));
	DBA_RUN_OR_GOTO(cleanup, dba_file_create(BUFR, "(stdout)", "w", &outfile));

	/* Throw away the command name */
	poptGetArg(optCon);

	while ((filename = poptGetArg(optCon)) != NULL)
	{
		int found;
		in = fopen(filename, "r");
		if (in == NULL)
		{
			err = dba_error_system("opening file %s", filename);
			goto cleanup;
		}
		while (1)
		{
			DBA_RUN_OR_GOTO(cleanup, parsetextgrib(in, msg, &found));
			if (found)
			{
				DBA_RUN_OR_GOTO(cleanup, bufrex_msg_encode(msg, &rmsg));
				DBA_RUN_OR_GOTO(cleanup, dba_file_write(outfile, rmsg));
				dba_rawmsg_delete(rmsg); rmsg = NULL;
			} else
				break;
		}
		fclose(in); in = NULL;
		++count;
	}

	if (count == 0)
		dba_cmdline_error(optCon, "at least one input file needs to be specified");

cleanup:
	if (in != NULL)
		fclose(in);
	if (msg)
		bufrex_msg_delete(msg);
	if (rmsg)
		dba_rawmsg_delete(rmsg);
	if (outfile)
		dba_file_delete(outfile);
	return err == DBA_OK ? dba_error_ok() : err;
#endif
	return 0;
}

static struct tool_desc dbamsg;

struct poptOption dbamsg_scan_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_dump_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the unput data ('bufr', 'crex', 'aof')", "type" },
	{ "interpreted", 0, 0, &op_dump_interpreted, 0,
		"dump the message as understood by the importer" },
	{ "text", 0, 0, &op_dump_text, 0,
		"dump as text that can be processed by dbamsg makebufr" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_cat_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_convert_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ "template", 0, POPT_ARG_STRING, &op_output_template, 0,
		"template of the data in output (autoselect if not specified)", "type.sub.local" },
	{ "report", 'r', POPT_ARG_STRING, &op_report, 0,
		"force output data to be of this type of report", "rep_memo" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_compare_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type1", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the first file to compare ('bufr', 'crex', 'aof')", "type" },
	{ "type2", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the second file to compare ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_filter_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
		"format of the data in output ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

struct poptOption dbamsg_fixaof_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	POPT_TABLEEND
};

struct poptOption dbamsg_makebufr_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	POPT_TABLEEND
};

struct poptOption dbamsg_bisect_options[] = {
	{ "help", '?', 0, 0, 1, "print an help message" },
	{ "verbose", 0, POPT_ARG_NONE, &op_verbose, 0, "verbose output" },
	{ "test", 0, POPT_ARG_STRING, &op_bisect_cmd, 0,
		"command to run to test a message group", "cmd" },
	{ "type", 't', POPT_ARG_STRING, &op_input_type, 0,
		"format of the input data ('bufr', 'crex', 'aof')", "type" },
	{ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
		"Options used to filter messages" },
	POPT_TABLEEND
};

static void init()
{
	dbamsg.desc = "Work with encoded meteorological data";
	dbamsg.longdesc =
		"Examine, dump and convert files containing meteorological data. "
		"It supports observations encoded in BUFR, CREX and AOF formats";
	dbamsg.ops = (struct op_dispatch_table*)calloc(11, sizeof(struct op_dispatch_table));

	dbamsg.ops[0].func = do_scan;
	dbamsg.ops[0].aliases[0] = "scan";
	dbamsg.ops[0].usage = "scan [options] filename [filename [...]]";
	dbamsg.ops[0].desc = "Summarise the contents of a file with meteorological data";
	dbamsg.ops[0].longdesc = NULL;
	dbamsg.ops[0].optable = dbamsg_scan_options;

	dbamsg.ops[1].func = do_dump;
	dbamsg.ops[1].aliases[0] = "dump";
	dbamsg.ops[1].usage = "dump [options] filename [filename [...]]";
	dbamsg.ops[1].desc = "Dump the contents of a file with meteorological data";
	dbamsg.ops[1].longdesc = NULL;
	dbamsg.ops[1].optable = dbamsg_dump_options;

	dbamsg.ops[2].func = do_cat;
	dbamsg.ops[2].aliases[0] = "cat";
	dbamsg.ops[2].usage = "cat [options] filename [filename [...]]";
	dbamsg.ops[2].desc = "Dump the raw data of a file with meteorological data";
	dbamsg.ops[2].longdesc = NULL;
	dbamsg.ops[2].optable = dbamsg_cat_options;

	dbamsg.ops[3].func = do_convert;
	dbamsg.ops[3].aliases[0] = "convert";
	dbamsg.ops[3].aliases[1] = "conv";
	dbamsg.ops[3].usage = "convert [options] filename [filename [...]]";
	dbamsg.ops[3].desc = "Convert meteorological data between different formats";
	dbamsg.ops[3].longdesc = NULL;
	dbamsg.ops[3].optable = dbamsg_convert_options;

	dbamsg.ops[4].func = do_compare;
	dbamsg.ops[4].aliases[0] = "compare";
	dbamsg.ops[4].aliases[1] = "cmp";
	dbamsg.ops[4].usage = "compare [options] filename1 [filename2]";
	dbamsg.ops[4].desc = "Compare two files with meteorological data";
	dbamsg.ops[4].longdesc = NULL;
	dbamsg.ops[4].optable = dbamsg_compare_options;

	dbamsg.ops[5].func = do_filter;
	dbamsg.ops[5].aliases[0] = "filter";
	dbamsg.ops[5].usage = "filter [options] [queryparm1=val1 [queryparm2=val2 [...]]] filename1 [filename2...] ";
	dbamsg.ops[5].desc = "Copy only those messages whose contents match the given query parameters";
	dbamsg.ops[5].longdesc = "The query can be done using the same parameters as dbadb or the Fortran DB-ALLe API.  Only the parameters starting with lat, lon, year, month, day, hour, minu, sec are actually used for filtering.";
	dbamsg.ops[5].optable = dbamsg_filter_options;

	dbamsg.ops[6].func = do_fixaof;
	dbamsg.ops[6].aliases[0] = "fixaof";
	dbamsg.ops[6].usage = "fixaof [options] filename [filename1 [...]]]";
	dbamsg.ops[6].desc = "Recomputes the start and end of observation period in the headers of the given AOF files";
	dbamsg.ops[6].longdesc = NULL;
	dbamsg.ops[6].optable = dbamsg_fixaof_options;

	dbamsg.ops[7].func = do_makebufr;
	dbamsg.ops[7].aliases[0] = "makebufr";
	dbamsg.ops[7].aliases[1] = "mkbufr";
	dbamsg.ops[7].usage = "makebufr [options] filename [filename1 [...]]]";
	dbamsg.ops[7].desc = "Read a simple description of a BUFR file and output the BUFR file.";
	dbamsg.ops[7].longdesc = "Read a simple description of a BUFR file and output the BUFR file.  This only works for simple BUFR messages without attributes encoded with data present bitmaps";
	dbamsg.ops[7].optable = dbamsg_makebufr_options;

	dbamsg.ops[8].func = do_bisect;
	dbamsg.ops[8].aliases[0] = "bisect";
	dbamsg.ops[8].usage = "bisect [options] --test=testscript filename";
	dbamsg.ops[8].desc = "Bisect filename and output the minimum subsequence found for which testscript fails.";
	dbamsg.ops[8].longdesc = "Run testscript passing parts of filename on its stdin and checking the return code.  Then divide the input in half and try on each half.  Keep going until testscript does not fail in any portion of the file.  Output to stdout the smallest portion for which testscript fails.  This is useful to isolate the few messages in a file that cause problems";
	dbamsg.ops[8].optable = dbamsg_bisect_options;

	dbamsg.ops[9].func = do_head;
	dbamsg.ops[9].aliases[0] = "head";
	dbamsg.ops[9].usage = "head [options] filename [filename [...]]";
	dbamsg.ops[9].desc = "Dump the contents of the header of a file with meteorological data";
	dbamsg.ops[9].longdesc = NULL;
	dbamsg.ops[9].optable = dbamsg_scan_options;

	dbamsg.ops[10].func = NULL;
	dbamsg.ops[10].usage = NULL;
	dbamsg.ops[10].desc = NULL;
	dbamsg.ops[10].longdesc = NULL;
	dbamsg.ops[10].optable = NULL;
};

static struct program_info proginfo = {
	"dbamsg",
	"Here are some example invocations of \\fBdbamsg\\fP:\n"
	".P\n"
	".nf\n"
	"  # Convert an AOF message to BUFR\n"
	"  dbamsg convert file.aof > file.bufr\n"
	"\n"
	"  # Convert a BUFR message to CREX\n"
	"  dbamsg convert file.bufr -d crex > file.crex\n"
	"\n"
	"  # Dump the content of a message, as they are in the message\n"
	"  dbamsg dump file.bufr\n"
	"\n"
	"  # Dump the content of a message, interpreted as physical quantities\n"
	"  dbamsg dump --interpreted file.bufr\n"
	".fi\n"
	,
	NULL,
	NULL
};

int main (int argc, const char* argv[])
{
	int res;
	init();
	res = dba_cmdline_dispatch_main(&proginfo, &dbamsg, argc, argv);
	return res;
}

/* vim:set ts=4 sw=4: */