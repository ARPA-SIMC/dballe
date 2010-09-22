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

#include "processor.h"

#include <wreport/bulletin.h>
#include <dballe/core/file.h>
#include <dballe/msg/aof_codec.h>
#include <dballe/msg/msgs.h>

#include <cstring>
#include <cstdlib>

using namespace wreport;
using namespace std;

extern int op_verbose;

namespace dballe {
namespace proc {

static int match_index(int idx, const char* expr)
{
	size_t pos;
	size_t len;
	for (pos = 0; (len = strcspn(expr + pos, ",")) > 0; pos += len + 1)
	{
		int start, end;
		int found = sscanf(expr + pos, "%d-%d", &start, &end);
		switch (found)
		{
			case 1:
				if (start == idx)
					return 1;
				break;
			case 2: 
				if (start <= idx && idx <= end)
					return 1;
				break;
			default:
				fprintf(stderr, "Cannot parse index string %s\n", expr);
				return 0;
		}
	}
	return 0;
}

bool match_common(const Rawmsg& rmsg, const Msgs* msgs, struct grep_t* grepdata)
{
	if (msgs == NULL && grepdata->parsable)
		return false;
	if (msgs != NULL && grepdata->unparsable)
		return false;
	return true;
}

bool match_bufrex(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs, struct grep_t* grepdata)
{
	if (!match_common(rmsg, msgs, grepdata))
		return false;

	if (grepdata->category != -1)
		if (grepdata->category != rm->type)
			return false;

	if (grepdata->subcategory != -1)
		if (grepdata->subcategory != rm->subtype)
			return false;

	return true;
}

bool match_bufr(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs, struct grep_t* grepdata)
{
	if (!match_bufrex(rmsg, rm, msgs, grepdata))
		return false;
	return true;
}

bool match_crex(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs, struct grep_t* grepdata)
{
	if (!match_bufrex(rmsg, rm, msgs, grepdata))
		return false;
	return true;

#if 0
	if (grepdata->checkdigit != -1)
	{
		int checkdigit;
		DBA_RUN_OR_RETURN(crex_message_has_check_digit(msg, &checkdigit));
		if (grepdata->checkdigit != checkdigit)
		{
			*match = 0;
			return dba_error_ok();
		}
	}
#endif
}

bool match_aof(const Rawmsg& rmsg, const Msgs* msgs, struct grep_t* grepdata)
{
	int category, subcategory;
	msg::AOFImporter::get_category(rmsg, &category, &subcategory);

	if (!match_common(rmsg, msgs, grepdata))
		return false;

	if (grepdata->category != -1)
		if (grepdata->category != category)
			return false;

	if (grepdata->subcategory != -1)
		if (grepdata->subcategory != subcategory)
			return false;

	return true;
}

static void print_parse_error(const Rawmsg& msg, error& e)
{
	fprintf(stderr, "Cannot parse %s message #%d: %s at offset %d.\n",
			encoding_name(msg.encoding), msg.index, e.what(), msg.offset);
}

static void process_input(File& file, const Rawmsg& rmsg, struct grep_t* grepdata, Action& action)
{
	int print_errors = (grepdata == NULL || !grepdata->unparsable);
	auto_ptr<Msgs> parsed(new Msgs);
	auto_ptr<Bulletin> br;
	bool match = true;

	switch (rmsg.encoding)
	{
		case BUFR:
			br.reset(new BufrBulletin);
			try {
				br->decode(rmsg, rmsg.filename().c_str(), rmsg.offset);
			} catch (error& e) {
				if (print_errors) print_parse_error(rmsg, e);
				br.release();
				parsed.release();
			}
			if (br.get())
			{
				std::auto_ptr<msg::Importer> imp = msg::Importer::create(rmsg.encoding);
				try {
					imp->from_bulletin(*br, *parsed);
				} catch (error& e) {
					if (print_errors) print_parse_error(rmsg, e);
					br.release();
					parsed.release();
				}
			}
			if (grepdata != NULL)
				match = match_bufr(rmsg, br.get(), parsed.get(), grepdata);
			break;
		case CREX:
			br.reset(new CrexBulletin);
			try {
				br->decode(rmsg, rmsg.filename().c_str(), rmsg.offset);
			} catch (error& e) {
				if (print_errors) print_parse_error(rmsg, e);
				br.release();
				parsed.release();
			}
			if (br.get())
			{
				std::auto_ptr<msg::Importer> imp = msg::Importer::create(rmsg.encoding);
				try {
					imp->from_bulletin(*br, *parsed);
				} catch (error& e) {
					if (print_errors) print_parse_error(rmsg, e);
					br.release();
					parsed.release();
				}
			}
			if (grepdata != NULL)
				match = match_crex(rmsg, br.get(), parsed.get(), grepdata);
			break;
		case AOF: {
			std::auto_ptr<msg::Importer> imp = msg::Importer::create(rmsg.encoding);
			try {
				imp->from_rawmsg(rmsg, *parsed);
			} catch (error& e) {
				if (print_errors) print_parse_error(rmsg, e);
				parsed.release();
			}
			if (grepdata != NULL)
				match = match_aof(rmsg, parsed.get(), grepdata);
			break;
		}
	}

	if (!match) return;

	action(rmsg, br.get(), parsed.get());
}

void process_all(poptContext optCon,
		 Encoding type,
		 struct grep_t* grepdata,
		 Action& action)
{
	const char* name = poptGetArg(optCon);
	Rawmsg rmsg;
	int index = 0;

	if (name == NULL)
		name = "(stdin)";

	do
	{
		auto_ptr<File> file(File::create(type, name, "r"));

		while (file->read(rmsg))
		{
			++index;

			if (op_verbose)
				fprintf(stderr, "Reading message #%d...\n", index);

			if (grepdata->index[0] == 0 || match_index(index, grepdata->index))
			{
				rmsg.index = index;
				process_input(*file, rmsg, grepdata, action);
			}
		}
	} while ((name = poptGetArg(optCon)) != NULL);
}

}
}

/* vim:set ts=4 sw=4: */
