/* For %zd */
#define _ISOC99_SOURCE

#include "dballe/message.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include "dballe/msg/bulletin.h"
#include "dballe/file.h"
#include "dballe/core/query.h"
#include "dballe/core/matcher.h"
#include "dballe/core/csv.h"
#include "dballe/core/json.h"
#include "dballe/core/var.h"
#include "dballe/cmdline/cmdline.h"
#include "dballe/cmdline/processor.h"
#include "dballe/cmdline/conversion.h"
#include <wreport/bulletin.h>
#include <wreport/vartable.h>
#include <wreport/dtable.h>
#include <wreport/subset.h>
#include <wreport/notes.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <cctype>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sstream>

using namespace wreport;
using namespace dballe;
using namespace dballe::cmdline;
using namespace std;

static int op_dump_interpreted = 0;
static int op_dump_text = 0;
static int op_dump_csv = 0;
static int op_dump_json = 0;
static int op_dump_dds = 0;
static int op_dump_structured = 0;
static int op_precise_import = 0;
static int op_bufr2netcdf_categories = 0;
static const char* op_output_type = "bufr";
static const char* op_output_template = "";
static const char* op_output_file = nullptr;
static const char* op_report = "";
static const char* op_bisect_cmd = nullptr;
int op_verbose = 0;

struct cmdline::ReaderOptions readeropts;

struct poptOption grepTable[] = {
    { "category", 0, POPT_ARG_INT, &readeropts.category, 0,
        "match messages with the given data category", "num" },
    { "subcategory", 0, POPT_ARG_INT, &readeropts.subcategory, 0,
        "match BUFR messages with the given data subcategory", "num" },
    { "check-digit", 0, POPT_ARG_INT, &readeropts.checkdigit, 0,
        "match CREX messages with check digit (if 1) or without check digit (if 0)", "num" },
    { "unparsable", 0, 0, &readeropts.unparsable, 0,
        "match only messages that cannot be parsed", 0 },
    { "parsable", 0, 0, &readeropts.parsable, 0,
        "match only messages that can be parsed", 0 },
    { "index", 0, POPT_ARG_STRING, &readeropts.index_filter, 0,
        "match messages with the index in the given range (ex.: 1-5,9,22-30)", "expr" },
    POPT_TABLEEND
};

/// Write CSV output to the given output stream
struct FileCSV : CSVWriter
{
    FILE* out;
    FileCSV(FILE* out) : out(out) {}

    void flush_row() override
    {
        fputs(row.c_str(), out);
        putc('\n', out);
        row.clear();
    }
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
        if (raw[i].isset())
            count++;
    return count;
}

static void dump_common_header(const BinaryMessage& rmsg, const Bulletin& braw)
{
    printf("Message %d\n", rmsg.index);
    printf("Size: %zd\n", rmsg.data.size());
    printf("Master table number: %hhu\n", braw.master_table_number);
    printf("Origin: %hu:%hu\n", braw.originating_centre, braw.originating_subcentre);
    printf("Category: %hhu:%hhu:%hhu\n", braw.data_category, braw.data_subcategory, braw.data_subcategory_local);
    printf("Update sequence number: %hhu\n", braw.update_sequence_number);
    printf("Datetime: %04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu\n",
            braw.rep_year, braw.rep_month, braw.rep_day,
            braw.rep_hour, braw.rep_minute, braw.rep_second);
    printf("B Table: %s\n", braw.tables.btable ? braw.tables.btable->pathname().c_str() : "(none)");
    printf("D Table: %s\n", braw.tables.dtable ? braw.tables.dtable->pathname().c_str() : "(none)");
}

static void dump_bufr_header(const BinaryMessage& rmsg, const BufrBulletin& braw)
{
    dump_common_header(rmsg, braw);
    printf("BUFR edition: %hhu\n", braw.edition_number);
    printf("Table version: %hhu:%hhu\n", braw.master_table_version_number, braw.master_table_version_number_local);
    printf("Compression: %s\n", braw.compression ? "yes" : "no");
    printf("Optional section length: %zd\n", braw.optional_section.size());
    printf("Subsets: %zd\n\n", braw.subsets.size());
}

static void dump_crex_header(const BinaryMessage& rmsg, const CrexBulletin& braw)
{
    dump_common_header(rmsg, braw);
    printf("CREX edition: %hhu\n", braw.edition_number);
    printf("Table version: %hhu/%hhu:%hhu\n",
            braw.master_table_version_number,
            braw.master_table_version_number_bufr,
            braw.master_table_version_number_local);
    printf("Check digit: %s\n\n", braw.has_check_digit ? "yes" : "no");
}

static void print_bulletin_header(const Bulletin& braw)
{
    printf(", origin %hu:%hu, category %hhu %hhu:%hhu:%hhu",
            braw.originating_centre, braw.originating_subcentre,
            braw.master_table_number,
            braw.data_category, braw.data_subcategory, braw.data_subcategory_local);
}

static void print_bufr_header(const BufrBulletin& braw)
{
    print_bulletin_header(braw);
    printf(", bufr edition %hhu, tables %hhu:%hhu",
            braw.edition_number,
            braw.master_table_version_number,
            braw.master_table_version_number_local);
    printf(", subsets %zd, values:", braw.subsets.size());
    for (size_t i = 0; i < braw.subsets.size(); ++i)
        printf(" %d/%zd", count_nonnulls(braw.subsets[i]), braw.subsets[i].size());
}

static void print_crex_header(const CrexBulletin& braw)
{
    print_bulletin_header(braw);
    printf(", crex edition %hhu, tables %hhu/%hhu:%hhu",
            braw.edition_number,
            braw.master_table_version_number,
            braw.master_table_version_number_bufr,
            braw.master_table_version_number_local);
    printf(", subsets %zd, values:", braw.subsets.size());
    for (size_t i = 0; i < braw.subsets.size(); ++i)
        printf(" %d/%zd", count_nonnulls(braw.subsets[i]), braw.subsets[i].size());
}

static void print_item_header(const Item& item)
{
    printf("#%d", item.idx);

    if (item.rmsg)
    {
        printf(" %s message: %zd bytes", File::encoding_name(item.rmsg->encoding), item.rmsg->data.size());

        switch (item.rmsg->encoding)
        {
            case Encoding::BUFR:
                if (item.bulletin != NULL)
                    print_bufr_header(*dynamic_cast<const BufrBulletin*>(item.bulletin));
                break;
            case Encoding::CREX:
                if (item.bulletin != NULL)
                    print_crex_header(*dynamic_cast<const CrexBulletin*>(item.bulletin));
                break;
            case Encoding::JSON:
                throw error_unimplemented("print_item_header json");
        }
    } else if (item.msgs) {
        printf(" message: %zd subsets:", item.msgs->size());
        string old_type;
        unsigned count = 0;
        for (const auto& i: *item.msgs)
        {
            auto m = impl::Message::downcast(i);
            string new_type = format_message_type(m->type);
            if (old_type.empty())
            {
                old_type = new_type;
                count = 1;
            } else if (old_type != new_type) {
                printf(" %u %s", count, old_type.c_str());
                old_type = new_type;
                count = 1;
            } else
                ++count;
        }
        printf(" %u %s", count, old_type.c_str());
    }
}

struct Summarise : public cmdline::Action
{
    virtual bool operator()(const cmdline::Item& item)
    {
        print_item_header(item);
        puts(".");
        return true;
    }
};

struct Head : public cmdline::Action
{
    virtual bool operator()(const cmdline::Item& item)
    {
        if (!item.rmsg) return false;
        switch (item.rmsg->encoding)
        {
            case Encoding::BUFR:
                if (item.bulletin == NULL) return true;
                dump_bufr_header(*item.rmsg, *dynamic_cast<const BufrBulletin*>(item.bulletin)); puts(".");
                break;
            case Encoding::CREX:
                if (item.bulletin == NULL) return true;
                dump_crex_header(*item.rmsg, *dynamic_cast<const CrexBulletin*>(item.bulletin)); puts(".");
                break;
            case Encoding::JSON:
                throw error_unimplemented("Head json");
        }
        return true;
    }
};

static void dump_dba_vars(const Subset& msg)
{
	for (size_t i = 0; i < msg.size(); ++i)
		msg[i].print(stdout);
}

/**
 * Print a bulletin in CSV format
 */
struct CSVBulletin : public cmdline::Action
{
    msg::BulletinCSVWriter writer;

    CSVBulletin() : writer(stdout) {}

    virtual bool operator()(const cmdline::Item& item)
    {
        if (!item.rmsg) return false;
        if (!item.bulletin) return false;
        writer.output_bulletin(*item.bulletin);
        return true;
    }
};

/**
 * Print a impl::Message in CSV format
 */

struct CSVMsgs : public cmdline::Action
{
    bool first;
    FileCSV writer;

    CSVMsgs() : first(true), writer(stdout) {}

    virtual bool operator()(const cmdline::Item& item)
    {
        if (!item.msgs) return false;

        if (first)
        {
            impl::Message::csv_header(writer);
            first = false;
        }

        for (const auto& mi: *item.msgs)
            impl::Message::downcast(mi)->to_csv(writer);
        return true;
    }
};

/**
 * Print a Msgs in JSON format
 */
struct JSONMsgs : public cmdline::Action
{
    core::JSONWriter json;

    JSONMsgs() : json(cout) {}
    ~JSONMsgs() { cout << flush; }

    virtual bool operator()(const cmdline::Item& item)
    {
        if (!item.msgs) return false;

        for (const auto& mi: *item.msgs) {
            auto msg = impl::Message::downcast(mi);
            json.start_mapping();
            json.add("version");
            json.add(DBALLE_JSON_VERSION);
            json.add("network");
            json.add(msg->get_rep_memo_var() ? msg->get_rep_memo_var()->enqc() : dballe::impl::Message::repmemo_from_type(msg->type));
            json.add("ident");
            if (msg->get_ident_var() != NULL)
                json.add(msg->get_ident_var()->enqc());
            else
                json.add_null();
            json.add("lon");
            json.add_int(msg->get_longitude_var()->enqi());
            json.add("lat");
            json.add_int(msg->get_latitude_var()->enqi());
            json.add("date");
            std::stringstream ss;
            msg->get_datetime().to_stream_iso8601(ss, 'T', "Z");
            json.add(ss.str().c_str());
            json.add("data");
            json.start_list();
                json.start_mapping();
                json.add("vars");
                json.add(msg->station_data);
                json.end_mapping();
                for (const auto& ctx: msg->data) {
                    json.start_mapping();
                    json.add("timerange");
                    json.add(ctx.trange);
                    json.add("level");
                    json.add(ctx.level);
                    json.add("vars");
                    json.add(ctx.values);
                    json.end_mapping();
                }
                json.end_list();
            json.end_mapping();
            json.add_break();
        }
        return true;
    }
};

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

    virtual bool operator()(const cmdline::Item& item)
    {
        print_item_header(item);
        if (!item.rmsg)
        {
            puts(": no low-level information available");
            return true;
        }
        puts(":");
        switch (item.rmsg->encoding)
        {
            case Encoding::BUFR:
                {
                    if (item.bulletin == NULL) return true;
                    print_subsets(*item.bulletin);
                    break;
                }
            case Encoding::CREX:
                {
                    if (item.bulletin == NULL) return true;
                    print_subsets(*item.bulletin);
                    break;
                }
            case Encoding::JSON:
                throw error_unimplemented("DumpMessage json");
        }
        return true;
    }
};

struct DumpCooked : public cmdline::Action
{
    virtual bool operator()(const cmdline::Item& item)
    {
        if (item.msgs == NULL) return false;
        for (size_t i = 0; i < item.msgs->size(); ++i)
        {
            printf("#%d[%zd] ", item.idx, i);
            (*item.msgs)[i]->print(stdout);
        }
        return true;
    }
};

static void print_var(const Var& var)
{
    string formatted = var.format("");
    printf("%01d%02d%03d %s\n", WR_VAR_FXY(var.code()), formatted.c_str());
}

struct DumpText : public cmdline::Action
{
    void add_keyval(const char* key, unsigned val)
    {
        printf("%s: %u\n", key, val);
    }

    void add_keyval(const char* key, const std::string& val)
    {
        printf("%s: %s\n", key, val.c_str());
    }

    virtual bool operator()(const cmdline::Item& item)
    {
		if (item.bulletin == NULL)
			throw error_consistency("source is not a BUFR or CREX message");

        const Bulletin& bul = *item.bulletin;
        add_keyval("master_table_number", bul.master_table_number);
        add_keyval("data_category", bul.data_category);
        add_keyval("data_subcategory", bul.data_subcategory);
        add_keyval("data_subcategory_local", bul.data_subcategory_local);
        add_keyval("originating_centre", bul.originating_centre);
        add_keyval("originating_subcentre", bul.originating_subcentre);
        add_keyval("update_sequence_number", bul.update_sequence_number);
        char buf[30];
        snprintf(buf, 29, "%hu-%hhu-%hhu %hhu:%hhu:%hhu",
                bul.rep_year, bul.rep_month, bul.rep_day,
                bul.rep_hour, bul.rep_minute, bul.rep_second);
        add_keyval("representative_time", buf);
        if (const BufrBulletin* b = dynamic_cast<const BufrBulletin*>(item.bulletin))
        {
            add_keyval("encoding", "bufr");
            add_keyval("edition_number", b->edition_number);
            add_keyval("master_table_version_number", b->master_table_version_number);
            add_keyval("master_table_version_number_local", b->master_table_version_number_local);
            add_keyval("compression", b->compression ? "true" : "false");
            add_keyval("optional_section", b->optional_section);
        } else if (const CrexBulletin* b = dynamic_cast<const CrexBulletin*>(item.bulletin)) {
            add_keyval("encoding", "crex");
            add_keyval("edition_number", b->edition_number);
            add_keyval("master_table_version_number", b->master_table_version_number);
            add_keyval("master_table_version_number_bufr", b->master_table_version_number_bufr);
            add_keyval("master_table_version_number_local", b->master_table_version_number_local);
            add_keyval("has_check_digit", b->has_check_digit ? "true" : "false");
        } else
            throw error_consistency("encoding not supported for CSV dump");

        printf("descriptors:");
        for (const auto& desc: bul.datadesc)
            printf(" %01d%02d%03d", WR_VAR_FXY(desc));
        printf("\n");

        for (size_t i = 0; i < bul.subsets.size(); ++i)
        {
            const Subset& subset = bul.subsets[i];
            printf("subset %zd:\n", i + 1);
			for (size_t j = 0; j < subset.size(); ++j)
			{
				const Var& var = subset[j];
				printf(" ");
				print_var(var);
				for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
				{
					printf(" *");
					print_var(*attr);
				}
			}
		}
        return true;
    }
};

struct DumpStructured : public cmdline::Action
{
    virtual bool operator()(const cmdline::Item& item)
    {
        print_item_header(item);
        if (!item.rmsg)
        {
            puts(": no low-level information available");
            return true;
        }
        if (!item.bulletin)
        {
            puts(": no bulletin information available");
            return true;
        }
        puts(":");
        item.bulletin->print_structured(stdout);
        return true;
    }
};

struct DumpDDS : public cmdline::Action
{
    virtual bool operator()(const cmdline::Item& item)
    {
        print_item_header(item);
        if (!item.rmsg)
        {
            puts(": no low-level information available");
            return true;
        }
        if (!item.bulletin)
        {
            puts(": no bulletin information available");
            return true;
        }
        puts(":");
        item.bulletin->print_datadesc(stdout);
        return false;
    }
};

struct WriteRaw : public cmdline::Action
{
    File* file;
    WriteRaw() : file(0) {}
    ~WriteRaw() { if (file) delete file; }

    virtual bool operator()(const cmdline::Item& item)
    {
        if (!item.rmsg) return false;
        if (!file) file = File::create(item.rmsg->encoding, stdout, false, "(stdout)").release();
        file->write(item.rmsg->data);
        return true;
    }
};

struct Scan : public cmdline::Subcommand
{
    Scan()
    {
        names.push_back("scan");
        usage = "scan [options] [filter] filename [filename [...]]";
        desc = "Summarise the contents of a file with meteorological data";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
                "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);

        core::Query query;
        cmdline::Reader reader(readeropts);
        if (dba_cmdline_get_query(optCon, query) > 0)
            reader.filter.matcher_from_record(query);
        Summarise s;
        reader.read(get_filenames(optCon), s);
        return 0;
    }
};

struct HeadCmd : public cmdline::Subcommand
{
    HeadCmd()
    {
        names.push_back("head");
        usage = "head [options] [filter] filename [filename [...]]";
        desc = "Dump the contents of the header of a file with meteorological data";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
                "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);
        cmdline::Reader reader(readeropts);

        core::Query query;
        if (dba_cmdline_get_query(optCon, query) > 0)
            reader.filter.matcher_from_record(query);

        Head head;
        reader.read(get_filenames(optCon), head);
        return 0;
    }
};

struct Dump : public cmdline::Subcommand
{
    Dump()
    {
        names.push_back("dump");
        usage = "dump [options] [filter] filename [filename [...]]";
        desc = "Dump the contents of a file with meteorological data";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ "interpreted", 0, 0, &op_dump_interpreted, 0,
            "dump the message as understood by the importer", 0 });
        opts.push_back({ "precise", 0, 0, &op_precise_import, 0,
            "import messages using precise contexts instead of standard ones", 0 });
        opts.push_back({ "text", 0, 0, &op_dump_text, 0,
            "dump as text that can be processed by dbamsg makebufr", 0 });
        opts.push_back({ "csv", 0, 0, &op_dump_csv, 0,
            "dump in machine readable CSV format", 0 });
        opts.push_back({ "json", 0, 0, &op_dump_json, 0,
            "dump in machine readable JSON format", 0 });
        opts.push_back({ "dds", 0, 0, &op_dump_dds, 0,
            "dump structure of data description section", 0 });
        opts.push_back({ "structured", 0, 0, &op_dump_structured, 0,
            "structured dump of the message contents", 0 });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        unique_ptr<cmdline::Action> action;
        if (op_dump_csv)
        {
            if (op_dump_interpreted)
                action.reset(new CSVMsgs);
            else
                action.reset(new CSVBulletin);
        }
        else if (op_dump_json)
            action.reset(new JSONMsgs);
        else if (op_dump_interpreted)
            action.reset(new DumpCooked);
        else if (op_dump_text)
            action.reset(new DumpText);
        else if (op_dump_structured)
            action.reset(new DumpStructured);
        else if (op_dump_dds)
            action.reset(new DumpDDS);
        else 
            action.reset(new DumpMessage);

        /* Throw away the command name */
        poptGetArg(optCon);
        cmdline::Reader reader(readeropts);
        if (op_precise_import) reader.import_opts.simplified = false;

        core::Query query;
        if (dba_cmdline_get_query(optCon, query) > 0)
            reader.filter.matcher_from_record(query);

        reader.read(get_filenames(optCon), *action);
        return 0;
    }
};

struct Cat : public cmdline::Subcommand
{
    Cat()
    {
        names.push_back("cat");
        usage = "cat [options] [filter] filename [filename [...]]";
        desc = "Dump the raw data of a file with meteorological data";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);
        cmdline::Reader reader(readeropts);

        core::Query query;
        if (dba_cmdline_get_query(optCon, query) > 0)
            reader.filter.matcher_from_record(query);

        WriteRaw wraw;
        reader.read(get_filenames(optCon), wraw);
        return 0;
    }
};

#if 0
struct StoreMessages : public cmdline::Action, public vector<BinaryMessage>
{
    void operator()(const BinaryMessage& rmsg, const wreport::Bulletin*, const impl::Messages*) override
    {
        push_back(rmsg);
    }
};
#endif

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

struct Bisect : public cmdline::Subcommand
{
    Bisect()
    {
        names.push_back("bisect");
        usage = "bisect [options] --test=testscript filename";
        desc = "Bisect filename and output the minimum subsequence found for which testscript fails.";
        longdesc = "Run testscript passing parts of filename on its stdin and checking the return code.  Then divide the input in half and try on each half.  Keep going until testscript does not fail in any portion of the file.  Output to stdout the smallest portion for which testscript fails.  This is useful to isolate the few messages in a file that cause problems";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "test", 0, POPT_ARG_STRING, &op_bisect_cmd, 0,
            "command to run to test a message group", "cmd" });
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
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
                    dba_cmdline_stringToMsgType(reader.input_type, optCon),
                    &reader.filter, store_messages, &vec));
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
};

struct Convert : public cmdline::Subcommand
{
    Convert()
    {
        names.push_back("convert");
        names.push_back("conv");
        usage = "convert [options] [filter] filename [filename [...]]";
        desc = "Convert meteorological data between different formats";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the input data ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "dest", 'd', POPT_ARG_STRING, &op_output_type, 0,
            "format of the data in output ('bufr', 'crex')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ "template", 0, POPT_ARG_STRING, &op_output_template, 0,
            "template of the data in output (autoselect if not specified, 'list' gives a list)", "name" });
        opts.push_back({ "report", 'r', POPT_ARG_STRING, &op_report, 0,
            "force output data to be of this type of report", "rep_memo" });
        opts.push_back({ "precise", 0, 0, &op_precise_import, 0,
            "import messages using precise contexts instead of standard ones", 0 });
        opts.push_back({ "bufr2netcdf-categories", 0, 0, &op_bufr2netcdf_categories, 0,
            "recompute data categories and subcategories according to message contents, for use as input to bufr2netcdf", 0 });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
        opts.push_back({ "output", 'o', POPT_ARG_STRING, &op_output_file, 0,
            "destination file. Default: stdandard output", "fname" });
    }

    int main(poptContext optCon) override
    {
        impl::ExporterOptions opts;
        cmdline::Converter conv;
        cmdline::Reader reader(readeropts);
        reader.verbose = op_verbose;

        /* Throw away the command name */
        poptGetArg(optCon);

        if (strcmp(op_output_template, "list") == 0)
        {
            list_templates();
            return 0;
        }

        core::Query query;
        if (dba_cmdline_get_query(optCon, query) > 0)
            reader.filter.matcher_from_record(query);

        if (op_precise_import) reader.import_opts.simplified = false;

        if (op_report[0] != 0)
            conv.dest_rep_memo = op_report;
        else
            conv.dest_rep_memo = NULL;

        if (op_output_template[0] != 0)
        {
            conv.dest_template = op_output_template;
            opts.template_name = op_output_template;
        }

        conv.bufr2netcdf_categories = op_bufr2netcdf_categories != 0;

        if (strcmp(op_output_type, "auto") == 0) {
            if (op_output_file == NULL)
                conv.file = File::create(stdout, false, "stdout").release();
            else
                conv.file = File::create(op_output_file, "w").release();
        } else {
            if (op_output_file == NULL)
                conv.file = File::create(string_to_encoding(op_output_type), stdout, false, "stdout").release();
            else
                conv.file = File::create(string_to_encoding(op_output_type), op_output_file, "w").release();
        }

        conv.exporter = Exporter::create(conv.file->encoding(), opts).release();

        reader.read(get_filenames(optCon), conv);

        return 0;
    }
};


struct Compare : public cmdline::Subcommand
{
    Compare()
    {
        names.push_back("compare");
        names.push_back("cmp");
        usage = "compare [options] filename1 [filename2]";
        desc = "Compare two files with meteorological data";
    }

    void add_to_optable(std::vector<poptOption>& opts) const override
    {
        Subcommand::add_to_optable(opts);
        opts.push_back({ "type1", 't', POPT_ARG_STRING, &readeropts.input_type, 0,
            "format of the first file to compare ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "type2", 'd', POPT_ARG_STRING, &op_output_type, 0,
            "format of the second file to compare ('bufr', 'crex', 'json', 'csv')", "type" });
        opts.push_back({ "rejected", 0, POPT_ARG_STRING, &readeropts.fail_file_name, 0,
            "write unprocessed data to this file", "fname" });
        opts.push_back({ NULL, 0, POPT_ARG_INCLUDE_TABLE, &grepTable, 0,
            "Options used to filter messages", 0 });
    }

    int main(poptContext optCon) override
    {
        /* Throw away the command name */
        poptGetArg(optCon);
        cmdline::Reader reader(readeropts);

        /* Read the file names */
        const char* file1_name = poptGetArg(optCon);
        if (file1_name == NULL)
            dba_cmdline_error(optCon, "input file needs to be specified");

        const char* file2_name = poptGetArg(optCon);
        if (file2_name == NULL)
            file2_name = "(stdin)";

        unique_ptr<File> file1;
        unique_ptr<File> file2;
        if (strcmp(readeropts.input_type, "auto") == 0)
            file1 = File::create(file1_name, "r");
        else
            file1 = File::create(string_to_encoding(readeropts.input_type), file1_name, "r");
        if (strcmp(op_output_type, "auto") == 0)
            file2 = File::create(file2_name, "r");
        else
            file2 = File::create(string_to_encoding(op_output_type), file2_name, "r");
        std::unique_ptr<Importer> importer1 = Importer::create(file1->encoding());
        std::unique_ptr<Importer> importer2 = Importer::create(file2->encoding());
        size_t idx = 0;
        for ( ; ; ++idx)
        {
            ++idx;

            BinaryMessage msg1 = file1->read();
            BinaryMessage msg2 = file2->read();
            bool found1 = msg1;
            bool found2 = msg2;

            if (found1 != found2)
                throw error_consistency("The files contain a different number of messages");
            if (!found1 && !found2)
                break;

            impl::Messages msgs1 = importer1->from_binary(msg1);
            impl::Messages msgs2 = importer2->from_binary(msg2);

            notes::Collect c(cerr);
            int diffs = impl::msg::messages_diff(msgs1, msgs2);
            if (diffs > 0)
                error_consistency::throwf("Messages #%zd contain %d differences", idx, diffs);
        }
        if (idx == 0)
            throw error_consistency("The files do not contain messages");
        return 0;
    }
};

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

struct MakeBUFR : public cmdline::Subcommand
{
    MakeBUFR()
    {
        names.push_back("makebufr");
        names.push_back("mkbufr");
        usage = "makebufr [options] filename [filename1 [...]]]";
        desc = "Read a simple description of a BUFR file and output the BUFR file.";
        longdesc = "Read a simple description of a BUFR file and output the BUFR file.  This only works for simple BUFR messages without attributes encoded with data present bitmaps";
    }

    int main(poptContext optCon) override
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
};

int main (int argc, const char* argv[])
{
    Command dbamsg;
    dbamsg.name = "dbamsg";
    dbamsg.desc = "Work with encoded meteorological data";
    dbamsg.longdesc =
        "Examine, dump and convert files containing meteorological data. "
        "It supports observations encoded in BUFR or CREX formats";
    dbamsg.manpage_examples_section = R"(
Here are some example invocations of \\fBdbamsg\\fP:
.P
.nf
  # Convert a BUFR message to CREX
  dbamsg convert file.bufr -d crex > file.crex

  # Convert BUFR messages to CREX, but skip all those not in january 2010
  dbamsg convert year=2010 month=1 file.bufr -d crex > file.crex

  # Dump the content of a message, as they are in the message
  dbamsg dump file.bufr

  # Dump the content of a message, interpreted as physical quantities
  dbamsg dump --interpreted file.bufr
.fi
)";

    dbamsg.add_subcommand(new Scan);
    dbamsg.add_subcommand(new HeadCmd);
    dbamsg.add_subcommand(new Dump);
    dbamsg.add_subcommand(new Cat);
    dbamsg.add_subcommand(new Bisect);
    dbamsg.add_subcommand(new Convert);
    dbamsg.add_subcommand(new Compare);
    dbamsg.add_subcommand(new MakeBUFR);

    return dbamsg.main(argc, argv);
}
