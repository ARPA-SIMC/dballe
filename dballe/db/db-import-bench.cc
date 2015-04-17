/*
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/db/bench.h"
#include "dballe/core/file.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/codec.h"

using namespace dballe;
using namespace std;

namespace {

struct MsgCollector : public vector<Msg*>
{
    ~MsgCollector()
    {
        for (iterator i = begin(); i != end(); ++i)
            delete *i;
    }
    void read(Encoding type, const std::string& fname)
    {
        dballe::msg::Importer::Options opts;
        std::unique_ptr<msg::Importer> importer = msg::Importer::create(type, opts);
        unique_ptr<File> f = File::open_test_data_file(type, fname);
        f->foreach([&](const Rawmsg& rmsg) {
            importer->foreach_decoded(rmsg, [&](unique_ptr<Msg> m) {
                push_back(m.release());
                return true;
            });
            return true;
        });
    }
};

struct B : bench::DBBenchmark
{
    bench::Task synop;
    bench::Task temp;
    bench::Task flight;
    MsgCollector samples_synop;
    MsgCollector samples_temp;
    MsgCollector samples_flight;

    B(const std::string& name)
        : bench::DBBenchmark::DBBenchmark(name),
          synop(this, "synop"), temp(this, "temp"), flight(this, "airplane")
    {
        repetitions = 10;
    }

    void setup_main()
    {
        bench::DBBenchmark::setup_main();

        // Read samples
        for (auto& fn : { "synop-cloudbelow.bufr", "synop-evapo.bufr", "synop-groundtemp.bufr", "synop-gtscosmo.bufr", "synop-longname.bufr" })
            samples_synop.read(BUFR, string("bufr/") + fn);
        for (auto& fn : { "temp-gts1.bufr", "temp-gts2.bufr", "temp-gts3.bufr", "temp-gtscosmo.bufr", "temp-timesig18.bufr" })
            samples_temp.read(BUFR, string("bufr/") + fn);
        for (auto& fn : { "gts-acars1.bufr", "gts-acars2.bufr", "gts-acars-us1.bufr", "gts-amdar1.bufr", "gts-amdar2.bufr" })
            samples_flight.read(BUFR, string("bufr/") + fn);
    }

    void main() override
    {
        synop.collect([&]() {
            for (auto& m: samples_synop)
                db->import_msg(*m, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
        });
        temp.collect([&]() {
            for (auto& m: samples_temp)
                db->import_msg(*m, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
        });
        flight.collect([&]() {
            for (auto& m: samples_flight)
                db->import_msg(*m, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
        });
    }
} test("db_import");

}
