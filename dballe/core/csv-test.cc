#include "tests.h"
#include "csv.h"
#include <sstream>
#include <iostream>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test CSV string escaping
        add_method("escape", []() {
            stringstream s;
            csv_output_quoted_string(s, "");
            wassert(actual(s.str()) == "");
            wassert(actual(CSVReader::unescape(s.str())) == "");

            s.str(std::string());
            csv_output_quoted_string(s, "1");
            wassert(actual(s.str()) == "1");
            wassert(actual(CSVReader::unescape(s.str())) == "1");

            s.str(std::string());
            csv_output_quoted_string(s, "12");
            wassert(actual(s.str()) == "12");
            wassert(actual(CSVReader::unescape(s.str())) == "12");

            s.str(std::string());
            csv_output_quoted_string(s, "123");
            wassert(actual(s.str()) == "123");
            wassert(actual(CSVReader::unescape(s.str())) == "123");

            s.str(std::string());
            csv_output_quoted_string(s, ",");
            wassert(actual(s.str()) == "\",\"");
            wassert(actual(CSVReader::unescape(s.str())) == ",");

            s.str(std::string());
            csv_output_quoted_string(s, "antani, blinda");
            wassert(actual(s.str()) == "\"antani, blinda\"");
            wassert(actual(CSVReader::unescape(s.str())) == "antani, blinda");

            s.str(std::string());
            csv_output_quoted_string(s, "antani, \"blinda\"");
            wassert(actual(s.str()) == "\"antani, \"\"blinda\"\"\"");
            wassert(actual(CSVReader::unescape(s.str())) == "antani, \"blinda\"");

            s.str(std::string());
            csv_output_quoted_string(s, "\"");
            wassert(actual(s.str()) == "\"\"\"\"");
            wassert(actual(CSVReader::unescape(s.str())) == "\"");

            s.str(std::string());
            csv_output_quoted_string(s, "\",\"");
            wassert(actual(s.str()) == "\"\"\",\"\"\"");
            wassert(actual(CSVReader::unescape(s.str())) == "\",\"");

            wassert(actual(CSVReader::unescape("\"")) == "\"");
            wassert(actual(CSVReader::unescape("\"\"")) == "");
            wassert(actual(CSVReader::unescape("\"\"\"")) == "\"");
            wassert(actual(CSVReader::unescape("\"\"\"\"")) == "\"");
            wassert(actual(CSVReader::unescape("\"\"\"\"\"")) == "\"\"");
            wassert(actual(CSVReader::unescape("a\"b")) == "a\"b");
        });

        // Test CSV reader
        add_method("reader", []() {
            {
                stringstream in("");
                CSVReader reader(in);
                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("\n");
                CSVReader reader(in);
                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 1);
                wassert(actual(reader.cols[0]) == "");
                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("\r\n");
                CSVReader reader(in);
                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 1);
                wassert(actual(reader.cols[0]) == "");
                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("1,2\r\n");
                CSVReader reader(in);
                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 2u);
                wassert(actual(reader.cols[0]) == "1");
                wassert(actual(reader.cols[1]) == "2");
                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("1\r\n2\r\n");
                CSVReader reader(in);
                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 1u);
                wassert(actual(reader.cols[0]) == "1");
                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 1u);
                wassert(actual(reader.cols[0]) == "2");
                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("1,2\n");
                CSVReader reader(in);
                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 2u);
                wassert(actual(reader.cols[0]) == "1");
                wassert(actual(reader.cols[1]) == "2");
                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in(
                        "1,\",\",2\n"
                        "antani,,blinda\n"
                        ",\n"
                );
                CSVReader reader(in);

                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 3u);
                wassert(actual(reader.cols[0]) == "1");
                wassert(actual(reader.cols[1]) == ",");
                wassert(actual(reader.cols[2]) == "2");

                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 3u);
                wassert(actual(reader.cols[0]) == "antani");
                wassert(actual(reader.cols[1]) == "");
                wassert(actual(reader.cols[2]) == "blinda");

                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 2u);
                wassert(actual(reader.cols[0]) == "");
                wassert(actual(reader.cols[1]) == "");

                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("1,2");
                CSVReader reader(in);

                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 2u);
                wassert(actual(reader.cols[0]) == "1");
                wassert(actual(reader.cols[1]) == "2");

                wassert(actual(reader.next()).isfalse());
            }

            {
                stringstream in("\"1\"\r\n");
                CSVReader reader(in);

                wassert(actual(reader.next()).istrue());
                wassert(actual(reader.cols.size()) == 1u);
                wassert(actual(reader.cols[0]) == "1");

                wassert(actual(reader.next()).isfalse());
            }
        });

        // Test write/read cycles
        add_method("writer", []() {
            MemoryCSVWriter out;
            out.add_value(1);
            out.add_value("\"");
            out.add_value("'");
            out.add_value(",");
            out.add_value("\n");
            out.flush_row();

            out.buf.seekg(0);
            CSVReader in(out.buf);
            wassert(actual(in.next()).istrue());
            wassert(actual(in.cols.size()) == 5);
            wassert(actual(in.as_int(0)) == 1);
            wassert(actual(in.cols[1]) == "\"");
            wassert(actual(in.cols[2]) == "'");
            wassert(actual(in.cols[3]) == ",");
            wassert(actual(in.cols[4]) == "\n");
            wassert(actual(in.next()).isfalse());
        });
    }
} test("core_csv");

}
