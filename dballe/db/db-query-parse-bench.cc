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
#include "dballe/file.h"
#include "dballe/core/query.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/codec.h"
#include <vector>
#include <cstdlib>

using namespace dballe;
using namespace std;
using namespace wreport;

namespace {

#if 0
template<typename T>
struct Samples : public std::map<std::string, std::vector<T>>
{
    unsigned seed = 0;

    // Return a random number in the domain [min, max)
    int rnd(int min, int max)
    {
        int r = rand_r(&seed);
        return min + (int)((double)(max - min) * r / RAND_MAX);
    }

    // Add a sample value for a key
    void add(const std::string& key, const T& sample)
    {
        (*this)[key].push_back(sample);
    }

    // Return a random key
    const std::string& get_key() const
    {
        int r = rnd(0, size());
        return *(begin() + r);
    }

    // Return a random value for a key
    const T& get_val(const std::string& key) const
    {
        auto i = find(key);
        if (i == end())
            error_consistency::throwf("no samples registered for %s", key.c_str());
        return i->second[rnd(0, i->second.size())];
    }
};

struct Generator
{
    Samples<int> ints;
    Samples<std::string> strings;
    Samples<Coords> coords;
    Samples<Datetime> datetimes;
    Samples<Level> levels;
    Samples<Trange> tranges;

    Generator()
    {
        ints.add("ana_id", 1);
        ints.add("ana_id", 2);
        ints.add("ana_id", 3);
        ints.add("ana_id", 4);
        ints.add("ana_id", 5);
        ints.add("priority", 1);
        ints.add("priority", 10);
        ints.add("priority", 100);
    }

    int ana_id = MISSING_INT;
    int prio_min = MISSING_INT;
    int prio_max = MISSING_INT;
    std::string rep_memo;
    int mobile = MISSING_INT;
    bool has_ident = false;
    std::string ident;
    Coords coords_min;
    Coords coords_max;
    Datetime datetime_min;
    Datetime datetime_max;
    Level level;
    Trange trange;
    std::set<wreport::Varcode> varcodes;
    std::string query;
    std::string ana_filter;
    std::string data_filter;
    std::string attr_filter;
    int limit = MISSING_INT;
    int block = MISSING_INT;
    int station = MISSING_INT;
    int data_id = MISSING_INT;
    bool query_station_vars = false;

};
#endif

struct Data
{
    vector<vector<string>> data;
    const int Q_ANA_ID = 0;
    const int Q_LATLON = 1;
    const int Q_MOBILE = 2;
    const int Q_ANAFILTER = 3;
    const int Q_BLOCKSTATION = 4;
    const int Q_PRIO = 5;
    const int Q_REPMEMO = 6;
    const int Q_VAR = 7;
    const int Q_DATETIME = 8;
    const int Q_LEVEL = 9;
    const int Q_TRANGE = 10;
    const int Q_QBEST = 11;
    const int Q_DATA_ID = 12;
    const int Q_DATA_FILTER = 13;
    const int Q_ATTR_FILTER = 14;

    Data()
    {
        data.push_back({"ana_id=1"});
        data.push_back({
                "latmin=40, latmax=45",
                "lonmin=10, lonmax=15",
                "lonmin=15, lonmax=10",
                "latmin=40, latmax=45, lonmin=10, lonmax=15",
                "latmin=40, latmax=45, lonmin=15, lonmax=10",
                "lat=11, lon=45",
        });
        data.push_back({
                "mobile=0",
                "mobile=1, ident=antani",
        });
        data.push_back({"ana_filter=B07004=1000"});
        data.push_back({
                "block=16",
                "block=16, station=404",
        });
        data.push_back({
                "priority=100",
                "priomin=1, priomax=100",
        });
        data.push_back({"rep_memo=synop"});
        data.push_back({
                "var=B12101",
                "varlist=B11101,B12101,B12103",
        });
        data.push_back({
                "year=1945, month=4, day=25, hour=12, min=30, sec=45",
                "yearmin=1945, yearmax=2000",
        });
        data.push_back({"leveltype1=1, l1=1000, leveltype2=1, l2=2000"});
        data.push_back({"pindicator=1, p1=2, p2=3"});
        data.push_back({"query=best"});
        data.push_back({"context_id=42"});
        data.push_back({"data_filter=B12101>278.15"});
        data.push_back({"attr_filter=B33007>50"});
    }

    string build(const vector<int>& items, const vector<int>& cur) const
    {
        string res;
        for (unsigned i = 0; i < items.size(); ++i)
        {
            if (cur[i] == -1) continue;
            if (!res.empty()) res += ", ";
            res += data[items[i]][cur[i]];
        }
        return res;
    }

    // Return false if cur cannot be implemented anymore
    bool increment(const vector<int>& items, vector<int>& cur, unsigned pos=0) const
    {
        if (pos >= items.size()) return false;

        ++cur[pos];

        if (cur[pos] >= data[items[pos]].size())
        {
            cur[pos] = -1;
            return increment(items, cur, pos+1);
        } else {
            return true;
        }
    }

    void generate(const vector<int>& items, std::function<void(const std::string&)> dest)
    {
        vector<int> cur(items.size(), -1);
        while (true)
        {
            // Build a string with cur
            dest(build(items, cur));
            // Increment cur
            // Break if cur is the max
            if (!increment(items, cur))
                break;
        }
    }
};

struct B : bench::DBBenchmark
{
    vector<core::Query> queries_station;
    vector<core::Query> queries_sdata;
    vector<core::Query> queries_data;
    benchmark::Task station;
    benchmark::Task sdata;
    benchmark::Task data;

    B(const std::string& name)
        : bench::DBBenchmark::DBBenchmark(name),
          station(this, "station"), sdata(this, "sdata"), data(this, "data")
    {
        repetitions = 5;
    }

    void setup_main()
    {
        const int Q_ANA_ID = 0;
        const int Q_LATLON = 1;
        const int Q_MOBILE = 2;
        const int Q_ANAFILTER = 3;
        const int Q_BLOCKSTATION = 4;
        const int Q_PRIO = 5;
        const int Q_REPMEMO = 6;
        const int Q_VAR = 7;
        const int Q_DATETIME = 8;
        const int Q_LEVEL = 9;
        const int Q_TRANGE = 10;
        const int Q_QBEST = 11;
        const int Q_DATA_ID = 12;
        const int Q_DATA_FILTER = 13;
        const int Q_ATTR_FILTER = 14;
        unsigned count = 0;

        bench::DBBenchmark::setup_main();
        Data d;
        d.generate({Q_ANA_ID, Q_LATLON, Q_MOBILE, Q_ANAFILTER, Q_BLOCKSTATION}, [&](const std::string& q) {
            core::Query query;
            query.set_from_test_string(q);
            queries_station.push_back(query);
        });
        //fprintf(stderr, "%zd stations\n", queries_station.size());

        count = 0;
        d.generate({Q_ANA_ID, Q_LATLON, Q_MOBILE, Q_ANAFILTER, Q_BLOCKSTATION, Q_PRIO, Q_REPMEMO, Q_DATA_ID, Q_DATA_FILTER, Q_ATTR_FILTER}, [&](const std::string& q) {
            if (count++ % 7 != 0) return;
            core::Query query;
            query.set_from_test_string(q);
            queries_sdata.push_back(query);
        });
        //fprintf(stderr, "%zd sdata\n", queries_sdata.size());

        count = 0;
        d.generate({Q_ANA_ID, Q_LATLON, Q_MOBILE, Q_ANAFILTER, Q_BLOCKSTATION, Q_PRIO, Q_REPMEMO, Q_VAR, Q_DATETIME, Q_LEVEL, Q_TRANGE, Q_QBEST, Q_DATA_ID, Q_DATA_FILTER, Q_ATTR_FILTER}, [&](const std::string& q) {
            if (count++ % 149 != 0) return;
            core::Query query;
            query.set_from_test_string(q);
            queries_data.push_back(query);
        });
        //fprintf(stderr, "%zd data\n", queries_data.size());
    }

    void main() override
    {
        station.collect([&]() {
            for (auto& q: queries_station)
                db->query_stations(q);
        });
        sdata.collect([&]() {
            for (auto& q: queries_sdata)
                db->query_data(q);
        });
        data.collect([&]() {
            for (auto& q: queries_data)
                db->query_data(q);
        });
    }
} test("db_query_parse");

}
