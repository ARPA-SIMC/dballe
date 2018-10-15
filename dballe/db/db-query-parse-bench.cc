#include "dballe/db/benchmark.h"
#include "dballe/file.h"
#include "dballe/core/query.h"
#include "dballe/msg/msg.h"
#include <vector>
#include <cstdlib>

using namespace dballe;
using namespace std;

namespace {

struct Task : public benchmark::DBTask
{
    core::Query query;

    Task(const std::string& name, const std::string& query)
        : benchmark::DBTask(name)
    {
        this->query.set_from_test_string(query);
    }
};

struct StationTask : public Task
{
    using Task::Task;

    void run_once() override
    {
        db->transaction()->query_stations(query);
    }
};

struct StationDataTask : public Task
{
    using Task::Task;

    void run_once() override
    {
        db->transaction()->query_station_data(query);
    }
};

struct DataTask : public Task
{
    using Task::Task;

    void run_once() override
    {
        db->transaction()->query_data(query);
    }
};

/*
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
*/

struct B : benchmark::Benchmark
{
    using Benchmark::Benchmark;

    void register_tasks() override
    {
        // d.generate({Q_ANA_ID, Q_LATLON, Q_MOBILE, Q_ANAFILTER, Q_BLOCKSTATION}, [&](const std::string& q) {
        throughput_tasks.emplace_back(new StationTask("station_ana_id", "ana_id=1"));
        throughput_tasks.emplace_back(new StationTask("station_coords", "latmin=40, latmax=50, lonmin=10, lonmax=20"));
        throughput_tasks.emplace_back(new StationTask("station_ident", "ident=test"));
        throughput_tasks.emplace_back(new StationTask("station_repmemo", "rep_memo=synop"));

        // d.generate({Q_ANA_ID, Q_LATLON, Q_MOBILE, Q_ANAFILTER, Q_BLOCKSTATION, Q_PRIO, Q_REPMEMO, Q_DATA_ID, Q_DATA_FILTER, Q_ATTR_FILTER}, [&](const std::string& q) {
        throughput_tasks.emplace_back(new StationTask("stationdata_ana_id", "ana_id=1"));
        throughput_tasks.emplace_back(new StationTask("stationdata_coords", "latmin=40, latmax=50, lonmin=10, lonmax=20"));
        throughput_tasks.emplace_back(new StationTask("stationdata_ident", "ident=test"));
        throughput_tasks.emplace_back(new StationTask("stationdata_repmemo", "rep_memo=synop"));

        // d.generate({Q_ANA_ID, Q_LATLON, Q_MOBILE, Q_ANAFILTER, Q_BLOCKSTATION, Q_PRIO, Q_REPMEMO, Q_VAR, Q_DATETIME, Q_LEVEL, Q_TRANGE, Q_QBEST, Q_DATA_ID, Q_DATA_FILTER, Q_ATTR_FILTER}, [&](const std::string& q) {
        throughput_tasks.emplace_back(new StationTask("data_ana_id", "ana_id=1"));
        throughput_tasks.emplace_back(new StationTask("data_coords", "latmin=40, latmax=50, lonmin=10, lonmax=20"));
        throughput_tasks.emplace_back(new StationTask("data_ident", "ident=test"));
        throughput_tasks.emplace_back(new StationTask("data_repmemo", "rep_memo=synop"));
        throughput_tasks.emplace_back(new StationTask("data_datetime", "yearmin=2010, yearmax=2012"));
    }
} test("db_query_parse");

}
