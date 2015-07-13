/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/file.h"
#include "dballe/message.h"
#include "dballe/msg/msg.h"
#include "dballe/core/csv.h"
#include "dballe/core/match-wreport.h"
#include "dballe/msg/aof_codec.h"
#include "dballe/cmdline/cmdline.h"
#include "dballe/core/vasprintf.h"

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

void ProcessingException::initmsg(const std::string& fname, unsigned index, const char* msg)
{
    char *c;
    asprintf(&c, "%s:#%u: %s", fname.c_str(), index, msg);
    msg = c;
    free(c);
}

static void print_parse_error(const BinaryMessage& msg, error& e)
{
    fprintf(stderr, "Cannot parse %s message #%d: %s at offset %ld.\n",
            File::encoding_name(msg.encoding), msg.index, e.what(), msg.offset);
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

void Item::set_msgs(Messages* new_msgs)
{
    if (msgs) delete msgs;
    msgs = new_msgs;
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
        case File::BUFR:
            try {
                bulletin = BufrBulletin::decode(rmsg->data, rmsg->pathname.c_str(), rmsg->offset).release();
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case File::CREX:
            try {
                bulletin = CrexBulletin::decode(rmsg->data, rmsg->pathname.c_str(), rmsg->offset).release();
            } catch (error& e) {
                if (print_errors) print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case File::AOF:
            // Nothing to do for AOF
            break;
    }

    // Second step: decode to msgs
    switch (rmsg->encoding)
    {
        case File::BUFR:
        case File::CREX:
            if (bulletin)
            {
                msgs = new Messages;
                try {
                    *msgs = imp.from_bulletin(*bulletin);
                } catch (error& e) {
                    if (print_errors) print_parse_error(*rmsg, e);
                    delete msgs;
                    msgs = 0;
                }
            }
            break;
        case File::AOF:
            msgs = new Messages;
            try {
                *msgs = imp.from_binary(*rmsg);
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

void Filter::matcher_reset()
{
    if (matcher)
    {
        delete matcher;
        matcher = 0;
    }
}

void Filter::matcher_from_record(const Query& query)
{
    if (matcher)
    {
        delete matcher;
        matcher = 0;
    }
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

bool Filter::match_common(const BinaryMessage&, const Messages* msgs) const
{
    if (msgs == NULL && parsable)
        return false;
    if (msgs != NULL && unparsable)
        return false;
    return true;
}

bool Filter::match_bufrex(const BinaryMessage& rmsg, const Bulletin* rm, const Messages* msgs) const
{
    if (!match_common(rmsg, msgs))
        return false;

    if (category != -1)
        if (category != rm->data_category)
            return false;

    if (subcategory != -1)
        if (subcategory != rm->data_subcategory)
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

bool Filter::match_bufr(const BinaryMessage& rmsg, const Bulletin* rm, const Messages* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
        return false;
    return true;
}

bool Filter::match_crex(const BinaryMessage& rmsg, const Bulletin* rm, const Messages* msgs) const
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

bool Filter::match_aof(const BinaryMessage& rmsg, const Messages* msgs) const
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

bool Filter::match_msgs(const Messages& msgs) const
{
    if (matcher && matcher->match(MatchedMessages(msgs)) != matcher::MATCH_YES)
        return false;

    return true;
}

bool Filter::match_item(const Item& item) const
{
    if (item.rmsg)
    {
        switch (item.rmsg->encoding)
        {
            case File::BUFR: return match_bufr(*item.rmsg, item.bulletin, item.msgs);
            case File::CREX: return match_crex(*item.rmsg, item.bulletin, item.msgs);
            case File::AOF: return match_aof(*item.rmsg, item.msgs);
            default: return false;
        }
    } else if (item.msgs)
        return match_msgs(*item.msgs);
    else
        return false;
}

Reader::Reader()
    : input_type("auto"), verbose(false), fail_file_name(0)
{
}

void Reader::read_csv(const std::list<std::string>& fnames, Action& action)
{
    // This cannot be implemented in dballe::File at the moment, since
    // dballe::File reads dballe::BinaryMessage strings, and here we read dballe::Messages
    // directly. We could split the input into several BinaryMessage strings, but that
    // would mean parsing the CSV twice: once to detect the message boundaries
    // and once to parse the BinaryMessage strings.
    Item item;
    unique_ptr<CSVReader> csvin;

    list<string>::const_iterator name = fnames.begin();
    do
    {
        if (name != fnames.end())
        {
            csvin.reset(new CSVReader(*name));
            ++name;
        } else {
            // name = "(stdin)";
            csvin.reset(new CSVReader(cin));
        }

        while (true)
        {
            // Read input message
            unique_ptr<Msg> msg(new Msg);
            if (!msg->from_csv(*csvin))
                break;

            // Match against index matcher
            ++item.idx;
            if (!filter.match_index(item.idx))
                continue;

            // We want it: move it to the item
            unique_ptr<Messages> msgs(new Messages);
            msgs->append(move(msg));
            item.set_msgs(msgs.release());

            if (!filter.match_item(item))
                continue;

            action(item);
        }
    } while (name != fnames.end());
}

void Reader::read_file(const std::list<std::string>& fnames, Action& action)
{
    bool print_errors = !filter.unparsable;
    std::unique_ptr<File> fail_file;

    list<string>::const_iterator name = fnames.begin();
    do
    {
        unique_ptr<File> file;

        if (strcmp(input_type, "auto") == 0)
        {
            if (name != fnames.end())
            {
                file = File::create(*name, "r");
                ++name;
            } else {
                file = File::create(stdin, false, "standard input");
            }
        } else {
            File::Encoding intype = string_to_encoding(input_type);
            if (name != fnames.end())
            {
                file = File::create(intype, *name, "r");
                ++name;
            } else {
                file = File::create(intype, stdin, false, "standard input");
            }
        }


        std::unique_ptr<msg::Importer> imp = msg::Importer::create(file->encoding(), import_opts);
        while (BinaryMessage bm = file->read())
        {
            Item item;
            item.rmsg = new BinaryMessage(bm);
            item.idx = bm.index;
            bool processed = false;

            try {
    //          if (op_verbose)
    //              fprintf(stderr, "Reading message #%d...\n", item.index);

                if (!filter.match_index(item.idx))
                    continue;

                item.decode(*imp, print_errors);

                //process_input(*file, rmsg, grepdata, action);

                if (!filter.match_item(item))
                    continue;

                processed = action(item);
            } catch (ProcessingException& pe) {
                // If ProcessingException has been raised, we can safely skip
                // to the next input
                processed = false;
                if (verbose)
                    fprintf(stderr, "%s\n", pe.what());
            } catch (std::exception& e) {
                if (verbose)
                    fprintf(stderr, "%s:#%d: %s\n", file->pathname().c_str(), item.idx, e.what());
                throw;
            }

            // Output items that have not been processed successfully
            if (!processed && fail_file_name)
            {
                if (!fail_file.get())
                    fail_file = File::create(file->encoding(), fail_file_name, "ab");
                fail_file->write(item.rmsg->data);
            }
        }
    } while (name != fnames.end());
}

void Reader::read(const std::list<std::string>& fnames, Action& action)
{
    if (strcmp(input_type, "csv") == 0)
        read_csv(fnames, action);
    else
        read_file(fnames, action);
}

} // namespace cmdline
} // namespace dballe

/* vim:set ts=4 sw=4: */
