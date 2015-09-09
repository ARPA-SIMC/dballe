#include "tests.h"
#include "bulletin.h"
#include <wreport/bulletin.h>
#include <wreport/utils/string.h>
#include <system_error>
#include <cerrno>
#include <cstdlib>

using namespace std;
using namespace wreport;
using namespace dballe;
using namespace dballe::tests;

namespace {

struct MemStream
{
    FILE* out = nullptr;
    char* buf = nullptr;
    size_t len = 0;

    MemStream()
        : out(open_memstream(&buf, &len))
    {
        if (!out)
            throw std::system_error(errno, std::system_category(), "cannot open an in-memory stream");
    }

    ~MemStream()
    {
        if (out) fclose(out);
        if (buf) free(buf);
    }

    void close()
    {
        fclose(out);
        out = nullptr;
    }

    operator FILE*() { return out; }
};

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("csv", []() {
            auto bulletin = BufrBulletin::create();
            bulletin->edition_number = 4;
            bulletin->originating_centre = 0;
            bulletin->originating_subcentre = 0;
            bulletin->data_category = 0;
            bulletin->data_subcategory = 1;
            bulletin->data_subcategory_local = 2;
            bulletin->master_table_version_number = 14;
            bulletin->master_table_version_number_local = 0;
            bulletin->rep_year = 2015;
            bulletin->rep_month = 4;
            bulletin->rep_day = 25;
            bulletin->rep_hour = 12;
            bulletin->rep_minute = 30;
            bulletin->rep_second = 45;
            bulletin->load_tables();
            // An integer
            bulletin->datadesc.push_back(WR_VAR(0, 1, 1));
            bulletin->obtain_subset(0).store_variable_i(WR_VAR(0, 1, 1), 14);
            // A string
            bulletin->datadesc.push_back(WR_VAR(0, 1, 6));
            bulletin->obtain_subset(0).store_variable_c(WR_VAR(0, 1, 6), "EZ1234");
            // A decimal
            bulletin->datadesc.push_back(WR_VAR(0, 1, 14));
            bulletin->obtain_subset(0).store_variable_d(WR_VAR(0, 1, 14), 3.14);

            MemStream out;

            // Write the message out
            msg::BulletinCSVWriter writer(out);
            writer.output_bulletin(*bulletin);
            fflush(out);

            // Read it back and check it
            vector<string> lines;
            {
                str::Split split(string(out.buf, out.len), "\n");
                std::copy(split.begin(), split.end(), std::back_inserter(lines));
            }
            wassert(actual(lines.size()) == 20u);

            wassert(actual(lines[ 0]) == R"("Field","Value")");
            wassert(actual(lines[ 1]) == R"("master_table_number",0)");
            wassert(actual(lines[ 2]) == R"("data_category",0)");
            wassert(actual(lines[ 3]) == R"("data_subcategory",1)");
            wassert(actual(lines[ 4]) == R"("data_subcategory_local",2)");
            wassert(actual(lines[ 5]) == R"("originating_centre",0)");
            wassert(actual(lines[ 6]) == R"("originating_subcentre",0)");
            wassert(actual(lines[ 7]) == R"("update_sequence_number",0)");
            wassert(actual(lines[ 8]) == R"("representative_time","2015-04-25 12:30:45")");
            wassert(actual(lines[ 9]) == R"("encoding","bufr")");
            wassert(actual(lines[10]) == R"("edition_number",4)");
            wassert(actual(lines[11]) == R"("master_table_version_number",14)");
            wassert(actual(lines[12]) == R"("master_table_version_number_local",0)");
            wassert(actual(lines[13]) == R"("compression","false")");
            wassert(actual(lines[14]) == R"("optional_section",)");
            wassert(actual(lines[15]) == R"("subset",1)");
            wassert(actual(lines[16]) == R"("B01001",14)");
            wassert(actual(lines[17]) == R"("B01006","EZ1234")");
            wassert(actual(lines[18]) == R"("B01014",3.14)");
            // Empty line because the last line ends with a newline and
            // str::Split sees it as a trailing empty token
            wassert(actual(lines[19]) == R"()");

            // Write the bulletin out again, to see that titles are not repeated
            writer.output_bulletin(*bulletin);
            fflush(out);

            lines.clear();
            {
                str::Split split(string(out.buf, out.len), "\n");
                std::copy(split.begin(), split.end(), std::back_inserter(lines));
            }

            wassert(actual(lines.size()) == 38u);

            wassert(actual(lines[ 0]) == R"("Field","Value")");
            wassert(actual(lines[ 1]) == R"("master_table_number",0)");
            wassert(actual(lines[ 2]) == R"("data_category",0)");
            wassert(actual(lines[ 3]) == R"("data_subcategory",1)");
            wassert(actual(lines[ 4]) == R"("data_subcategory_local",2)");
            wassert(actual(lines[ 5]) == R"("originating_centre",0)");
            wassert(actual(lines[ 6]) == R"("originating_subcentre",0)");
            wassert(actual(lines[ 7]) == R"("update_sequence_number",0)");
            wassert(actual(lines[ 8]) == R"("representative_time","2015-04-25 12:30:45")");
            wassert(actual(lines[ 9]) == R"("encoding","bufr")");
            wassert(actual(lines[10]) == R"("edition_number",4)");
            wassert(actual(lines[11]) == R"("master_table_version_number",14)");
            wassert(actual(lines[12]) == R"("master_table_version_number_local",0)");
            wassert(actual(lines[13]) == R"("compression","false")");
            wassert(actual(lines[14]) == R"("optional_section",)");
            wassert(actual(lines[15]) == R"("subset",1)");
            wassert(actual(lines[16]) == R"("B01001",14)");
            wassert(actual(lines[17]) == R"("B01006","EZ1234")");
            wassert(actual(lines[18]) == R"("B01014",3.14)");

            wassert(actual(lines[19]) == R"("master_table_number",0)");
            wassert(actual(lines[20]) == R"("data_category",0)");
            wassert(actual(lines[21]) == R"("data_subcategory",1)");
            wassert(actual(lines[22]) == R"("data_subcategory_local",2)");
            wassert(actual(lines[23]) == R"("originating_centre",0)");
            wassert(actual(lines[24]) == R"("originating_subcentre",0)");
            wassert(actual(lines[25]) == R"("update_sequence_number",0)");
            wassert(actual(lines[26]) == R"("representative_time","2015-04-25 12:30:45")");
            wassert(actual(lines[27]) == R"("encoding","bufr")");
            wassert(actual(lines[28]) == R"("edition_number",4)");
            wassert(actual(lines[29]) == R"("master_table_version_number",14)");
            wassert(actual(lines[30]) == R"("master_table_version_number_local",0)");
            wassert(actual(lines[31]) == R"("compression","false")");
            wassert(actual(lines[32]) == R"("optional_section",)");
            wassert(actual(lines[33]) == R"("subset",1)");
            wassert(actual(lines[34]) == R"("B01001",14)");
            wassert(actual(lines[35]) == R"("B01006","EZ1234")");
            wassert(actual(lines[36]) == R"("B01014",3.14)");
            // Empty line because the last line ends with a newline and
            // str::Split sees it as a trailing empty token
            wassert(actual(lines[37]) == R"()");
        });
    }
} test("msg_bulletin");

}
