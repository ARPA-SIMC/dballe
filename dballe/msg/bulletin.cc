#include "bulletin.h"
#include "dballe/core/csv.h"
#include <wreport/bulletin.h>
#include <wreport/var.h>
#include <string>

using namespace std;
using namespace wreport;

namespace dballe {
namespace msg {

namespace {

/// Write CSV output to the given output stream
struct Writer : public CSVWriter
{
    FILE* out;
    Writer(FILE* out) : out(out) {}

    void flush_row() override
    {
        fputs(row.c_str(), out);
        putc('\n', out);
        row.clear();
    }

    void print_var(const Var& var, const Var* parent=0)
    {
        string code;
        if (parent)
        {
            code += varcode_format(parent->code());
            code += ".";
        }
        code += varcode_format(var.code());
        switch (var.info()->type)
        {
            case Vartype::Integer: add_keyval(code.c_str(), var.enqi()); break;
            case Vartype::Decimal:
                add_value(code.c_str());
                add_value_raw(var.format(""));
                flush_row();
                break;
            default: add_keyval(code.c_str(), var.format("")); break;
        }
    }

    void print_subsets(const Bulletin& braw)
    {
        for (size_t i = 0; i < braw.subsets.size(); ++i)
        {
            const Subset& s = braw.subsets[i];
            add_keyval("subset", i + 1);
            for (size_t i = 0; i < s.size(); ++i)
            {
                print_var(s[i]);
                for (const Var* a = s[i].next_attr(); a != NULL; a = a->next_attr())
                    print_var(*a, &(s[i]));
            }
        }
    }

    void add_keyval(const char* key, unsigned val)
    {
        add_value(key);
        add_value(val);
        flush_row();
    }

    void add_keyval(const char* key, const std::string& val)
    {
        add_value(key);
        add_value(val);
        flush_row();
    }

    void write_bulletin(const wreport::Bulletin& bul)
    {
        add_keyval("master_table_number", bul.master_table_number);
        add_keyval("data_category", bul.data_category);
        add_keyval("data_subcategory", bul.data_subcategory);
        add_keyval("data_subcategory_local", bul.data_subcategory_local);
        add_keyval("originating_centre", bul.originating_centre);
        add_keyval("originating_subcentre", bul.originating_subcentre);
        add_keyval("update_sequence_number", bul.update_sequence_number);
        char buf[30];
        snprintf(buf, 29, "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
                bul.rep_year, bul.rep_month, bul.rep_day,
                bul.rep_hour, bul.rep_minute, bul.rep_second);
        add_keyval("representative_time", buf);
        if (const BufrBulletin* b = dynamic_cast<const BufrBulletin*>(&bul))
        {
            add_keyval("encoding", "bufr");
            add_keyval("edition_number", b->edition_number);
            add_keyval("master_table_version_number", b->master_table_version_number);
            add_keyval("master_table_version_number_local", b->master_table_version_number_local);
            add_keyval("compression", b->compression ? "true" : "false");
            add_keyval("optional_section", b->optional_section);
        } else if (const CrexBulletin* b = dynamic_cast<const CrexBulletin*>(&bul)) {
            add_keyval("encoding", "crex");
            add_keyval("edition_number", b->edition_number);
            add_keyval("master_table_version_number", b->master_table_version_number);
            add_keyval("master_table_version_number_bufr", b->master_table_version_number_bufr);
            add_keyval("master_table_version_number_local", b->master_table_version_number_local);
            add_keyval("has_check_digit", b->has_check_digit ? "true" : "false");
        } else
            throw error_consistency("encoding not supported for CSV dump");
        print_subsets(bul);
    }
};

}

BulletinCSVWriter::BulletinCSVWriter(FILE* out)
    : out(out)
{
}

BulletinCSVWriter::~BulletinCSVWriter()
{
}

void BulletinCSVWriter::output_bulletin(const wreport::Bulletin& bulletin)
{
    Writer writer(out);

    // Print column titles at the first BUFR
    if (first)
    {
        writer.add_value("Field");
        writer.add_value("Value");
        writer.flush_row();
        first = false;
    }
    writer.write_bulletin(bulletin);
}

}
}
