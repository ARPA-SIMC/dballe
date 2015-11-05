#include "dballe/db/bench.h"
#include "dballe/core/values.h"
#include "dballe/core/query.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/codec.h"

using namespace dballe;
using namespace std;
using namespace wreport;

namespace {

struct B : bench::DBBenchmark
{
    benchmark::Task by_latlon;
    benchmark::Task by_dt;
    benchmark::Task by_latlon_dt;
    benchmark::Task by_varcode;
    std::vector<Var> vars;

    B(const std::string& name)
        : bench::DBBenchmark::DBBenchmark(name),
          by_latlon(this, "by_latlon"),
          by_dt(this, "by_dt"),
          by_latlon_dt(this, "by_latlon_dt"),
          by_varcode(this, "by_varcode")
    {
        repetitions = 10;

        vars = {
            Var(varinfo(WR_VAR(0, 12, 101)), 280.15),
            Var(varinfo(WR_VAR(0, 12, 103)), 277.15),
            Var(varinfo(WR_VAR(0, 10,   4)), 1008.0),
            Var(varinfo(WR_VAR(0, 11,   1)), 42.0),
            Var(varinfo(WR_VAR(0, 11,   2)), 3.6),
        };
    }

    void setup_main()
    {
        bench::DBBenchmark::setup_main();

        /**
         * Insert the dataset used for query benchmarks
         */

        std::vector<std::string> reports { "synop", "metar" };

        for (int latlon = 0; latlon <= 25; ++latlon)
        {
            for (const auto& report: reports)
            {
                //fprintf(stderr, "INSERT LATLON %d REPORT %s\n", latlon, report.c_str());
                DataValues vals;
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
                            db->insert_data(vals, false, true);
                        }
                    }
                }
            }
        }
    }

    void main() override
    {
        by_latlon.collect([&]() {
            for (double latmin = 0; latmin < 25; latmin += 0.4)
                for (double lonmin = 0; lonmin < 25; lonmin += 0.4)
                {
                    core::Query query;
                    query.latrange = LatRange(latmin, 25.0);
                    query.lonrange = LonRange(lonmin, 25.0);
                    auto cur = db->query_data(query);
                    while (cur->next()) ;
                }
        });
        by_dt.collect([&]() {
            for (double yearmin = 2010; yearmin < 2015; ++yearmin)
                for (int monthmin = 1; monthmin <= 12; ++monthmin)
                    for (double yearmax = yearmin; yearmax < 2015; ++yearmax)
                    {
                        core::Query query;
                        query.datetime.set(
                                yearmin, monthmin, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT,
                                yearmax, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT);
                        auto cur = db->query_data(query);
                        while (cur->next()) ;
                    }
        });
        by_latlon_dt.collect([&]() {
            for (double latmin = 0; latmin < 25; latmin += 0.4)
                for (double yearmin = 2010; yearmin < 2015; ++yearmin)
                {
                    core::Query query;
                    query.latrange = LatRange(latmin, 25.0);
                    query.datetime.set(
                            yearmin, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT,
                            MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT);
                    auto cur = db->query_data(query);
                    while (cur->next()) ;
                }
        });
        by_varcode.collect([&]() {
            for (const auto& var: vars)
            {
                core::Query query;
                query.varcodes.insert(var.code());
                auto cur = db->query_data(query);
                while (cur->next()) ;
            }
        });
    }
} test("db_query");

}

