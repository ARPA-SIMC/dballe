#include "processor.h"
#include "dballe/cmdline/cmdline.h"
#include "dballe/core/csv.h"
#include "dballe/core/match-wreport.h"
#include "dballe/file.h"
#include "dballe/message.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stack>
#include <wreport/bulletin.h>
#include <wreport/utils/string.h>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

void ProcessingException::initmsg(const std::string& fname, unsigned index,
                                  const char* msg)
{
    char buf[512];
    snprintf(buf, 512, "%s:#%u: %s", fname.c_str(), index, msg);
    this->msg = buf;
}

static void print_parse_error(const BinaryMessage& msg, error& e)
{
    fprintf(stderr, "Cannot parse %s message #%d: %s at offset %jd.\n",
            File::encoding_name(msg.encoding), msg.index, e.what(),
            (intmax_t)msg.offset);
}

Item::Item() : idx(0), rmsg(0), bulletin(0), msgs(0) {}

Item::~Item()
{
    if (msgs)
        delete msgs;
    if (bulletin)
        delete bulletin;
    if (rmsg)
        delete rmsg;
}

void Item::set_msgs(std::vector<std::shared_ptr<dballe::Message>>* new_msgs)
{
    if (msgs)
        delete msgs;
    msgs = new_msgs;
}

void Item::decode(Importer& imp, bool print_errors)
{
    if (!rmsg)
        return;

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
        case Encoding::BUFR:
            try
            {
                bulletin = BufrBulletin::decode(
                               rmsg->data, rmsg->pathname.c_str(), rmsg->offset)
                               .release();
            }
            catch (error& e)
            {
                if (print_errors)
                    print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case Encoding::CREX:
            try
            {
                bulletin = CrexBulletin::decode(
                               rmsg->data, rmsg->pathname.c_str(), rmsg->offset)
                               .release();
            }
            catch (error& e)
            {
                if (print_errors)
                    print_parse_error(*rmsg, e);
                delete bulletin;
                bulletin = 0;
            }
            break;
        case Encoding::JSON: break;
    }

    // Second step: decode to msgs
    switch (rmsg->encoding)
    {
        case Encoding::BUFR:
        case Encoding::CREX:
            if (bulletin)
            {
                msgs = new std::vector<std::shared_ptr<dballe::Message>>;
                try
                {
                    *msgs = dynamic_cast<const BulletinImporter*>(&imp)
                                ->from_bulletin(*bulletin);
                }
                catch (error& e)
                {
                    if (print_errors)
                        print_parse_error(*rmsg, e);
                    delete msgs;
                    msgs = 0;
                }
            }
            break;
        case Encoding::JSON:
            try
            {
                msgs = new std::vector<std::shared_ptr<dballe::Message>>(
                    imp.from_binary(*rmsg));
            }
            catch (error& e)
            {
                if (print_errors)
                    print_parse_error(*rmsg, e);
            }
    }
}

void Item::processing_failed(std::exception& e) const
{
    throw ProcessingException(rmsg ? rmsg->pathname : "(unknown)", idx, e);
}

void IndexMatcher::parse(const std::string& str)
{
    ranges.clear();
    str::Split parts(str, ",", true);
    for (const auto& s : parts)
    {
        size_t pos = s.find('-');
        if (pos == 0)
            ranges.push_back(make_pair(0, std::stoi(s.substr(pos + 1))));
        else if (pos == s.size() - 1)
            ranges.push_back(make_pair(std::stoi(s.substr(0, pos)),
                                       std::numeric_limits<int>::max()));
        else if (pos == string::npos)
        {
            int val = std::stoi(s);
            ranges.push_back(make_pair(val, val));
        }
        else
            ranges.push_back(make_pair(std::stoi(s.substr(0, pos)),
                                       std::stoi(s.substr(pos + 1))));
    }
}

bool IndexMatcher::match(int val) const
{
    if (ranges.empty())
        return true;

    for (const auto& range : ranges)
        if (val >= range.first && val <= range.second)
            return true;
    return false;
}

Filter::Filter() {}
Filter::Filter(const ReaderOptions& opts)
    : category(opts.category), subcategory(opts.subcategory),
      checkdigit(opts.checkdigit), unparsable(opts.unparsable),
      parsable(opts.parsable)
{
    if (opts.index_filter)
        imatcher.parse(opts.index_filter);
}

Filter::~Filter() { delete matcher; }

void Filter::set_index_filter(const std::string& val) { imatcher.parse(val); }

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

bool Filter::match_index(int idx) const { return imatcher.match(idx); }

bool Filter::match_common(
    const BinaryMessage&,
    const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (msgs == NULL && parsable)
        return false;
    if (msgs != NULL && unparsable)
        return false;
    return true;
}

bool Filter::match_bufrex(
    const BinaryMessage& rmsg, const Bulletin* rm,
    const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
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
        }
        else if (rm)
        {
            if (matcher->match(MatchedBulletin(*rm)) != matcher::MATCH_YES)
                return false;
        }
    }

    return true;
}

bool Filter::match_bufr(
    const BinaryMessage& rmsg, const Bulletin* rm,
    const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (!match_bufrex(rmsg, rm, msgs))
        return false;
    return true;
}

bool Filter::match_crex(
    const BinaryMessage& rmsg, const Bulletin* rm,
    const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
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

bool Filter::match_json(
    const BinaryMessage& rmsg,
    const std::vector<std::shared_ptr<dballe::Message>>* msgs) const
{
    if (!match_common(rmsg, msgs))
        return false;

    if (matcher)
    {
        if (msgs)
        {
            if (!match_msgs(*msgs))
                return false;
        }
    }

    return true;
}

bool Filter::match_msgs(
    const std::vector<std::shared_ptr<dballe::Message>>& msgs) const
{
    if (matcher &&
        matcher->match(impl::MatchedMessages(msgs)) != matcher::MATCH_YES)
        return false;

    return true;
}

bool Filter::match_item(const Item& item) const
{
    if (item.rmsg)
    {
        switch (item.rmsg->encoding)
        {
            case Encoding::BUFR:
                return match_bufr(*item.rmsg, item.bulletin, item.msgs);
            case Encoding::CREX:
                return match_crex(*item.rmsg, item.bulletin, item.msgs);
            case Encoding::JSON: return match_json(*item.rmsg, item.msgs);
            default:             return false;
        }
    }
    else if (item.msgs)
        return match_msgs(*item.msgs);
    else
        return false;
}

Reader::Reader(const ReaderOptions& opts)
    : input_type(opts.input_type), fail_file_name(opts.fail_file_name),
      filter(opts)
{
}

bool Reader::has_fail_file() const { return fail_file_name != nullptr; }

void Reader::read_csv(const std::list<std::string>& fnames, Action& action)
{
    // This cannot be implemented in dballe::File at the moment, since
    // dballe::File reads dballe::BinaryMessage strings, and here we read
    // dballe::Messages directly. We could split the input into several
    // BinaryMessage strings, but that would mean parsing the CSV twice: once to
    // detect the message boundaries and once to parse the BinaryMessage
    // strings.
    Item item;
    unique_ptr<CSVReader> csvin;

    list<string>::const_iterator name = fnames.begin();
    do
    {
        if (name != fnames.end())
        {
            csvin.reset(new CSVReader(*name));
            ++name;
        }
        else
        {
            // name = "(stdin)";
            csvin.reset(new CSVReader(cin));
        }

        while (true)
        {
            // Read input message
            auto msg = std::make_shared<impl::Message>();
            if (!msg->from_csv(*csvin))
                break;

            // Match against index matcher
            ++item.idx;
            if (!filter.match_index(item.idx))
                continue;

            // We want it: move it to the item
            unique_ptr<impl::Messages> msgs(new impl::Messages);
            msgs->emplace_back(move(msg));
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

        if (input_type == "auto")
        {
            if (name != fnames.end())
            {
                file = File::create(*name, "r");
                ++name;
            }
            else
            {
                file = File::create(stdin, false, "standard input");
            }
        }
        else
        {
            Encoding intype = string_to_encoding(input_type.c_str());
            if (name != fnames.end())
            {
                file = File::create(intype, *name, "r");
                ++name;
            }
            else
            {
                file = File::create(intype, stdin, false, "standard input");
            }
        }

        std::unique_ptr<Importer> imp =
            Importer::create(file->encoding(), import_opts);
        while (BinaryMessage bm = file->read())
        {
            Item item;
            item.rmsg      = new BinaryMessage(bm);
            item.idx       = bm.index;
            bool processed = false;

            try
            {
                //          if (op_verbose)
                //              fprintf(stderr, "Reading message #%d...\n",
                //              item.index);

                if (!filter.match_index(item.idx))
                    continue;

                try
                {
                    item.decode(*imp, print_errors);
                }
                catch (std::exception& e)
                {
                    // Convert decode errors into ProcessingException, to skip
                    // this item if it fails to decode. We can safely skip,
                    // because if file->read() returned successfully the next
                    // read should properly start at the next item
                    item.processing_failed(e);
                }

                // process_input(*file, rmsg, grepdata, action);

                if (!filter.match_item(item))
                    continue;

                processed = action(item);
            }
            catch (ProcessingException& pe)
            {
                // If ProcessingException has been raised, we can safely skip
                // to the next input
                processed = false;
                if (verbose)
                    fprintf(stderr, "%s\n", pe.what());
            }
            catch (std::exception& e)
            {
                if (verbose)
                    fprintf(stderr, "%s:#%d: %s\n", file->pathname().c_str(),
                            item.idx, e.what());
                throw;
            }

            // Output items that have not been processed successfully
            if (!processed && fail_file_name)
            {
                if (!fail_file.get())
                    fail_file =
                        File::create(file->encoding(), fail_file_name, "ab");
                fail_file->write(item.rmsg->data);
            }
            if (processed)
                ++count_successes;
            else
                ++count_failures;
        }
    } while (name != fnames.end());
}

void Reader::read(const std::list<std::string>& fnames, Action& action)
{
    if (input_type == "csv")
        read_csv(fnames, action);
    else
        read_file(fnames, action);
}

} // namespace cmdline
} // namespace dballe
