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
#include <dballe/core/record.h>
#include <dballe/core/match-wreport.h>
#include <dballe/msg/aof_codec.h>
#include <dballe/msg/msgs.h>
#include <dballe/cmdline/cmdline.h>

#include <cstring>
#include <cstdlib>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

grep_t::grep_t()
    : category(-1), subcategory(-1), checkdigit(-1), unparsable(0), parsable(0),
      index(0), matcher(0)
{
}

grep_t::~grep_t()
{
    if (matcher) delete matcher;
}

void grep_t::matcher_from_args(poptContext optCon)
{
    if (matcher)
    {
        delete matcher;
        matcher = 0;
    }
    Record query;
    if (dba_cmdline_get_query(optCon, query) > 0)
        matcher = Matcher::create(query).release();
}

bool grep_t::match_index(int idx) const
{
    if (index == 0 || index[0] == 0)
        return true;

	size_t pos;
	size_t len;
	for (pos = 0; (len = strcspn(index + pos, ",")) > 0; pos += len + 1)
	{
		int start, end;
		int found = sscanf(index + pos, "%d-%d", &start, &end);
		switch (found)
		{
			case 1:
				if (start == idx)
					return true;
				break;
			case 2: 
				if (start <= idx && idx <= end)
					return true;
				break;
			default:
				fprintf(stderr, "Cannot parse index string %s\n", index);
				return false;
		}
	}
	return false;
}

bool grep_t::match_common(const Rawmsg& rmsg, const Msgs* msgs) const
{
    if (msgs == NULL && parsable)
        return false;
    if (msgs != NULL && unparsable)
        return false;
    return true;
}

bool grep_t::match_bufrex(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs) const
{
    if (!match_common(rmsg, msgs))
        return false;

    if (category != -1)
        if (category != rm->type)
            return false;

    if (subcategory != -1)
        if (subcategory != rm->subtype)
            return false;

    if (matcher)
    {
        if (msgs)
        {
            if (matcher->match(MatchedMsgs(*msgs)) != matcher::MATCH_YES)
                return false;
        } else if (rm) {
            if (matcher->match(MatchedBulletin(*rm)) != matcher::MATCH_YES)
                return false;
        }
    }

    return true;
}

bool grep_t::match_bufr(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
        return false;
    return true;
}

bool grep_t::match_crex(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
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

bool grep_t::match_aof(const Rawmsg& rmsg, const Msgs* msgs) const
{
    int category, subcategory;
    msg::AOFImporter::get_category(rmsg, &category, &subcategory);

    if (!match_common(rmsg, msgs))
        return false;

    if (category != -1)
        if (category != category)
            return false;

    if (subcategory != -1)
        if (subcategory != subcategory)
            return false;

    if (matcher && msgs)
        if (matcher->match(MatchedMsgs(*msgs)) != matcher::MATCH_YES)
            return false;

    return true;
}

static void print_parse_error(const Rawmsg& msg, error& e)
{
	fprintf(stderr, "Cannot parse %s message #%d: %s at offset %ld.\n",
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
				br->decode(rmsg, rmsg.file.c_str(), rmsg.offset);
			} catch (error& e) {
				if (print_errors) print_parse_error(rmsg, e);
				br.release();
				parsed.release();
			}
			if (br.get())
			{
				std::auto_ptr<msg::Importer> imp = msg::Importer::create(rmsg.encoding, grepdata->import_opts);
				try {
					imp->from_bulletin(*br, *parsed);
				} catch (error& e) {
					if (print_errors) print_parse_error(rmsg, e);
					br.release();
					parsed.release();
				}
			}
			if (grepdata != NULL)
				match = grepdata->match_bufr(rmsg, br.get(), parsed.get());
			break;
		case CREX:
			br.reset(new CrexBulletin);
			try {
				br->decode(rmsg, rmsg.file.c_str(), rmsg.offset);
			} catch (error& e) {
				if (print_errors) print_parse_error(rmsg, e);
				br.release();
				parsed.release();
			}
			if (br.get())
			{
				std::auto_ptr<msg::Importer> imp = msg::Importer::create(rmsg.encoding, grepdata->import_opts);
				try {
					imp->from_bulletin(*br, *parsed);
				} catch (error& e) {
					if (print_errors) print_parse_error(rmsg, e);
					br.release();
					parsed.release();
				}
			}
			if (grepdata != NULL)
				match = grepdata->match_crex(rmsg, br.get(), parsed.get());
			break;
		case AOF: {
			std::auto_ptr<msg::Importer> imp = msg::Importer::create(rmsg.encoding, grepdata->import_opts);
			try {
				imp->from_rawmsg(rmsg, *parsed);
			} catch (error& e) {
				if (print_errors) print_parse_error(rmsg, e);
				parsed.release();
			}
			if (grepdata != NULL)
				match = grepdata->match_aof(rmsg, parsed.get());
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

//			if (op_verbose)
//				fprintf(stderr, "Reading message #%d...\n", index);

			if (grepdata && grepdata->match_index(index))
			{
				rmsg.index = index;
				process_input(*file, rmsg, grepdata, action);
			}
		}
	} while ((name = poptGetArg(optCon)) != NULL);
}

} // namespace cmdline
} // namespace dballe

/* vim:set ts=4 sw=4: */
