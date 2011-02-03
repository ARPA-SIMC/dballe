/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <dballe/core/csv.h>
#include <dballe/core/record.h>
#include <dballe/core/match-wreport.h>
#include <dballe/msg/aof_codec.h>
#include <dballe/msg/msgs.h>
#include <dballe/cmdline/cmdline.h>

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

static void print_parse_error(const Rawmsg& msg, error& e)
{
    fprintf(stderr, "Cannot parse %s message #%d: %s at offset %ld.\n",
            encoding_name(msg.encoding), msg.index, e.what(), msg.offset);
}


Item::Item()
    : idx(0), rmsg(0), bulletin(0), msgs(0)
{
}

Item::~Item()
{
    if (msgs) delete msgs;
    if (bulletin) delete bulletin;
    if (rmsg) delete rmsg;
}

void Item::decode(msg::Importer& imp, bool print_errors)
{
    if (!rmsg) return;

    if (bulletin)
    {
        delete bulletin;
        bulletin = 0;
    }

    if (msgs)
    {
        delete msgs;
        msgs = 0;
    }

    // First step: decode raw message to bulletin
    switch (rmsg->encoding)
    {
        case BUFR:
            bulletin = new BufrBulletin;
            try {
                bulletin->decode(*rmsg, rmsg->file.c_str(), rmsg->offset);
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case CREX:
            bulletin = new CrexBulletin;
            try {
                bulletin->decode(*rmsg, rmsg->file.c_str(), rmsg->offset);
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case AOF:
            // Nothing to do for AOF
            break;
    }

    // Second step: decode to msgs
    switch (rmsg->encoding)
    {
        case BUFR:
        case CREX:
            if (bulletin)
            {
                msgs = new Msgs;
                try {
                    imp.from_bulletin(*bulletin, *msgs);
                } catch (error& e) {
                    if (print_errors) print_parse_error(*rmsg, e);
                    delete msgs;
                    msgs = 0;
                }
            }
            break;
        case AOF:
            msgs = new Msgs;
            try {
                imp.from_rawmsg(*rmsg, *msgs);
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete msgs;
                msgs = 0;
            }
            break;
    }
}


Filter::Filter()
    : category(-1), subcategory(-1), checkdigit(-1), unparsable(0), parsable(0),
      index(0), matcher(0)
{
}

Filter::~Filter()
{
    if (matcher) delete matcher;
}

void Filter::matcher_from_args(poptContext optCon)
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

bool Filter::match_index(int idx) const
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

bool Filter::match_common(const Rawmsg&, const Msgs* msgs) const
{
    if (msgs == NULL && parsable)
        return false;
    if (msgs != NULL && unparsable)
        return false;
    return true;
}

bool Filter::match_bufrex(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs) const
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
            if (!match_msgs(*msgs))
                return false;
        } else if (rm) {
            if (matcher->match(MatchedBulletin(*rm)) != matcher::MATCH_YES)
                return false;
        }
    }

    return true;
}

bool Filter::match_bufr(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
        return false;
    return true;
}

bool Filter::match_crex(const Rawmsg& rmsg, const Bulletin* rm, const Msgs* msgs) const
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

bool Filter::match_aof(const Rawmsg& rmsg, const Msgs* msgs) const
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

    if (msgs) return match_msgs(*msgs);

    return true;
}

bool Filter::match_msgs(const Msgs& msgs) const
{
    if (matcher && matcher->match(MatchedMsgs(msgs)) != matcher::MATCH_YES)
        return false;

    return true;
}

bool Filter::match_item(const Item& item) const
{
    if (item.rmsg)
    {
        switch (item.rmsg->encoding)
        {
            case BUFR: return match_bufr(*item.rmsg, item.bulletin, item.msgs);
            case CREX: return match_crex(*item.rmsg, item.bulletin, item.msgs);
            case AOF: return match_aof(*item.rmsg, item.msgs);
            default: return false;
        }
    } else if (item.msgs)
        return match_msgs(*item.msgs);
    else
        return false;
}

#if 0
static void process_input(File& file, const Rawmsg& rmsg, struct Filter* grepdata, Action& action)
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

	action(rmsg.index, &rmsg, br.get(), parsed.get());
}
#endif

#if 0
/**
 * Given a fresh csv reader, seek to start of data
 *
 * @returns true if there is data to read, false if we reached EOF
 */
static bool seek_to_start(CSVReader& in)
{
    while (true)
    {
        if (!in.next()) return false;
        if (in.cols.empty()) continue;
        if (in.cols[0].empty())
            error_consistency::throwf("The first column in line \"%s\" is empty", in.line.c_str());
        // First valid line should have a latitude
        if (isdigit(in.cols[0][0])) return true;
    }
}
#endif

#if 0
void process_csv(poptContext optCon,
        struct Filter* grepdata,
        Action& action)
{
    const char* name = poptGetArg(optCon);
    Rawmsg rmsg;
    int index = 0;
    auto_ptr<istream> in;
    auto_ptr<CSVReader> csvin;

    do
    {
        if (name != NULL)
        {
            in.reset(new ifstream(name));
            csvin.reset(new IstreamCSVReader(*in));
        } else
            csvin.reset(new IstreamCSVReader(cin));

        if (!seek_to_start(*csvin)) continue;

        while (true)
        {
            Msgs msgs;
            auto_ptr<Msg> msg(new Msg);
            if (!msg->from_csv(*csvin))
                break;
            msgs.acquire(msg);
            ++index;

            bool match = true;
            if (grepdata)
            {
                if (!grepdata->match_index(index))
                    match = false;
                else
                    match = grepdata->match_msgs(msgs);
            }

            if (match)
                action(rmsg, NULL, *msgs);
        }
    } while ((name = poptGetArg(optCon)) != NULL);
}
#endif

Reader::Reader()
    : input_type("auto")
{
}

void Reader::read(poptContext optCon, Action& action)
{
    const char* name = poptGetArg(optCon);
    Item item;
    item.rmsg = new Rawmsg;

    if (name == NULL)
        name = "(stdin)";

    bool print_errors = !filter.unparsable;
    Encoding intype = dba_cmdline_stringToMsgType(input_type, optCon);

    do
    {
        auto_ptr<File> file(File::create(intype, name, "r"));
        std::auto_ptr<msg::Importer> imp = msg::Importer::create(file->type(), import_opts);

        while (file->read(*item.rmsg))
        {
            ++item.idx;

//          if (op_verbose)
//              fprintf(stderr, "Reading message #%d...\n", item.index);

            if (!filter.match_index(item.idx))
                continue;

            item.rmsg->index = item.idx;
            item.decode(*imp, print_errors);
            //process_input(*file, rmsg, grepdata, action);

            if (!filter.match_item(item))
                continue;

            action(item);
        }
    } while ((name = poptGetArg(optCon)) != NULL);
}

} // namespace cmdline
} // namespace dballe

/* vim:set ts=4 sw=4: */
