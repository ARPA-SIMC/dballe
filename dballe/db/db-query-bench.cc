#include "dballe/db/benchmark.h"
#include "dballe/core/values.h"
#include "dballe/core/query.h"
#include "dballe/msg/msg.h"

using namespace dballe;
using namespace std;
using namespace wreport;

namespace {

struct Task : public benchmark::ExistingDBTask
{
    std::function<void(core::Query&)> make_query;

    Task(DB& db, const std::string& name, std::function<void(core::Query&)> make_query)
        : benchmark::ExistingDBTask(db, name), make_query(make_query)
    {
    }
};

struct StationTask : public Task
{
    using Task::Task;

    void run_once() override
    {
        core::Query query;
        make_query(query);
        auto tr = db.transaction();
        auto cur = tr->query_stations(query);
        while (cur->next()) ;
        tr->rollback();
    }
};

struct StationDataTask : public Task
{
    using Task::Task;

    void run_once() override
    {
        core::Query query;
        make_query(query);
        auto tr = db.transaction();
        auto cur = tr->query_station_data(query);
        while (cur->next()) ;
        tr->rollback();
    }
};

struct DataTask : public Task
{
    using Task::Task;

    void run_once() override
    {
        core::Query query;
        make_query(query);
        auto tr = db.transaction();
        auto cur = tr->query_data(query);
        while (cur->next()) ;
        tr->rollback();
    }
};


struct B : benchmark::DBBenchmark
{
    std::vector<Var> vars;

    using DBBenchmark::DBBenchmark;

    void setup()
    {
        benchmark::DBBenchmark::setup();

        /**
         * Insert the dataset used for benchmarking queries
         */

        std::vector<std::string> reports { "synop", "metar" };

        vars = std::vector<Var> {
            Var(varinfo(WR_VAR(0, 12, 101)), 280.15),
            Var(varinfo(WR_VAR(0, 12, 103)), 277.15),
            Var(varinfo(WR_VAR(0, 10,   4)), 1008.0),
            Var(varinfo(WR_VAR(0, 11,   1)), 42.0),
            Var(varinfo(WR_VAR(0, 11,   2)), 3.6),
        };

        auto t = db->transaction();
        for (int latlon = 0; latlon <= 25; ++latlon)
        {
            for (const auto& report: reports)
            {
                //fprintf(stderr, "INSERT LATLON %d REPORT %s\n", latlon, report.c_str());
                core::Data vals;
                vals.info.coords = Coords((double)latlon, (double)latlon);
                vals.info.report = report;
                for (int year = 2010; year < 2015; ++year)
                {
                    for (int month = 1; month <= 12; ++month)
                    {
                        vals.info.datetime = Datetime(year, month, 1);
                        for (int levtr = 0; levtr < 5; ++levtr)
                        {
                            vals.info.level = Level(levtr);
                            vals.info.trange = Trange(levtr);
                            vals.values.clear_ids();
                            for (const auto& var: vars)
                            {
                                vals.values.set(var);
                            }
                            t->insert_data(vals, false, true);
                        }
                    }
                }
            }
        }
        t->commit();
    }

    void register_tasks() override
    {
        unsigned i = 0;
        throughput_tasks.emplace_back(new StationTask(*db, "station_coords", [&i](core::Query& query) {
            query.latrange = LatRange((i % 25), 25);
            query.lonrange = LonRange((i % 25), 25);
        }));

        throughput_tasks.emplace_back(new StationDataTask(*db, "stationdata_coords", [&i](core::Query& query) {
            query.latrange = LatRange((i % 25), 25);
            query.lonrange = LonRange((i % 25), 25);
        }));

        throughput_tasks.emplace_back(new DataTask(*db, "data_coords", [&i](core::Query& query) {
            query.latrange = LatRange((i % 25), 25);
            query.lonrange = LonRange((i % 25), 25);
        }));

        throughput_tasks.emplace_back(new DataTask(*db, "data_dt", [&i](core::Query& query) {
            query.latrange = LatRange((i % 25), 25);
            query.lonrange = LonRange((i % 25), 25);
            query.datetime.set(
                    2010 + (i % 5), i % 12, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT,
                    2015, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT);
        }));

        throughput_tasks.emplace_back(new DataTask(*db, "data_varcode", [&i, this](core::Query& query) {
            query.varcodes.insert(vars[i % vars.size()].code());
        }));
    }
} test("db_query");

}
