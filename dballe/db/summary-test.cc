#define _DBALLE_TEST_CODE
#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/summary_memory.h"
#include "summary.h"
#include "config.h"
#ifdef HAVE_XAPIAN
#include "dballe/db/summary_xapian.h"
#endif

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

template<typename DB, typename BACKEND>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB, SummaryMemory> tg1("db_summary_memory_v7_sqlite_summary", "SQLITE");
Tests<V7DB, DBSummaryMemory> tg2("db_summary_memory_v7_sqlite_dbsummary", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB, SummaryMemory> tg3("db_summary_memory_v7_postgresql_summary", "POSTGRESQL");
Tests<V7DB, DBSummaryMemory> tg4("db_summary_memory_v7_postgresql_dbsummary", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB, SummaryMemory> tg5("db_summary_memory_v7_mysql_summary", "MYSQL");
Tests<V7DB, DBSummaryMemory> tg6("db_summary_memory_v7_mysql_dbsummary", "MYSQL");
#endif

std::unique_ptr<Summary> other_summary(const DBSummaryMemory&) { return std::unique_ptr<Summary>(new SummaryMemory); }
std::unique_ptr<DBSummary> other_summary(const SummaryMemory&) { return std::unique_ptr<DBSummary>(new DBSummaryMemory); }

#ifdef HAVE_XAPIAN
Tests<V7DB, SummaryXapian> tg7("db_summary_xapian_v7_sqlite_summary", "SQLITE");
Tests<V7DB, DBSummaryXapian> tg8("db_summary_xapian_v7_sqlite_dbsummary", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB, SummaryXapian> tg9("db_summary_xapian_v7_postgresql_summary", "POSTGRESQL");
Tests<V7DB, DBSummaryXapian> tg10("db_summary_xapian_v7_postgresql_dbsummary", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB, SummaryXapian> tg11("db_summary_xapian_v7_mysql_summary", "MYSQL");
Tests<V7DB, DBSummaryXapian> tg12("db_summary_xapian_v7_mysql_dbsummary", "MYSQL");
#endif

std::unique_ptr<Summary> other_summary(const DBSummaryXapian&) { return std::unique_ptr<Summary>(new SummaryXapian); }
std::unique_ptr<DBSummary> other_summary(const SummaryXapian&) { return std::unique_ptr<DBSummary>(new DBSummaryXapian); }
#endif


void station_id_isset(const Station& station) {}
void station_id_isset(const DBStation& station) { wassert(actual(station.id) != MISSING_INT); }

Station other_station(const DBStation& station)
{
    Station res(station);
    return res;
}
DBStation other_station(const Station& station)
{
    DBStation res;
    res.report = station.report;
    res.coords = station.coords;
    res.ident = station.ident;
    return res;
}

void set_query_station(core::Query& query, const Station& station)
{
    query.report = station.report;
    query.set_latrange(LatRange(station.coords.lat, station.coords.lat));
    query.set_lonrange(LonRange(station.coords.lon, station.coords.lon));
    query.ident = station.ident;
}
void set_query_station(core::Query& query, const DBStation& station)
{
    query.ana_id = station.id;
}

template<typename BACKEND>
std::vector<typename BACKEND::station_type> get_stations(const BACKEND& summary)
{
    std::vector<typename BACKEND::station_type> res;
    summary.stations([&](const typename BACKEND::station_type& s) { res.emplace_back(s); return true; });
    return res;
}

template<typename BACKEND>
std::vector<std::string> get_reports(const BACKEND& summary)
{
    std::vector<std::string> res;
    summary.reports([&](const std::string& l) { res.emplace_back(l); return true; });
    return res;
}

template<typename BACKEND>
std::vector<Level> get_levels(const BACKEND& summary)
{
    std::vector<Level> res;
    summary.levels([&](const Level& l) { res.emplace_back(l); return true; });
    return res;
}

template<typename BACKEND>
std::vector<Trange> get_tranges(const BACKEND& summary)
{
    std::vector<Trange> res;
    summary.tranges([&](const Trange& tr) { res.emplace_back(tr); return true; });
    return res;
}

template<typename BACKEND>
std::vector<wreport::Varcode> get_varcodes(const BACKEND& summary)
{
    std::vector<wreport::Varcode> res;
    summary.varcodes([&](const wreport::Varcode& v) { res.emplace_back(v); return true; });
    return res;
}

template<typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& vec)
{
    bool first = true;
    o << "[";
    for (const auto& v: vec)
    {
        if (first)
            first = false;
        else
            o << ", ";
        o << v;
    }
    o << "]";
    return o;
}

template<typename DB, typename BACKEND>
void Tests<DB, BACKEND>::register_tests()
{

this->add_method("summary", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    OldDballeTestDataSet test_data;
    wassert(f.populate(test_data));

    BACKEND s;
    wassert_true(s.datetime_min().is_missing());
    wassert_true(s.datetime_max().is_missing());
    wassert(actual(s.data_count()) == 0u);

    // Build the whole db summary
    core::Query query;
    query.query = "details";
    auto cur = f.tr->query_summary(query);
    unsigned expected_remaining = 4;
    wassert(actual(cur->remaining()) == expected_remaining);
    while (cur->next())
    {
        --expected_remaining;
        wassert(actual(cur->remaining()) == expected_remaining);
        s.add_cursor(*cur);
    }

    // Check its contents
    wassert(actual(get_stations(s).size()) == 2);
    wassert(station_id_isset(get_stations(s).front()));
    wassert(actual(get_levels(s).size()) == 1);
    wassert(actual(get_tranges(s).size()) == 2);
    wassert(actual(get_varcodes(s).size()) == 2);
    wassert(actual(s.datetime_min()) == Datetime(1945, 4, 25, 8));
    wassert(actual(s.datetime_max()) == Datetime(1945, 4, 25, 8, 30));
    wassert(actual(s.data_count()) == 4);
    set_query_station(query, get_stations(s).front());
    BACKEND s1;
    s1.add_filtered(s, query);
    wassert(actual(get_stations(s1).size()) == 1);
    // wassert(actual(s1.stations().begin()->station.id) == query.ana_id);

    BACKEND s2;
    cur = s.query_summary(query);
    while (cur->next())
        s2.add_cursor(*cur);
    wassert(actual(get_stations(s2).size()) == 1);
});

this->add_method("summary_msg", [](Fixture& f) {
    BACKEND s;

    // Summarise a message
    impl::Messages msgs = dballe::tests::read_msgs("bufr/synop-rad1.bufr", Encoding::BUFR, "accurate");
    s.add_messages(msgs);

    // Check its contents
    wassert(actual(get_stations(s).size()) == 25);
    wassert(actual(get_levels(s).size()) == 37);
    wassert(actual(get_tranges(s).size()) == 9);
    wassert(actual(get_varcodes(s).size()) == 39);
    wassert(actual(s.datetime_min()) == Datetime(2015, 3, 5, 3));
    wassert(actual(s.datetime_max()) == Datetime(2015, 3, 5, 3));
    wassert(actual(s.data_count()) == 1095);
});

this->add_method("merge_entries", [](Fixture& f) {
    typename BACKEND::station_type station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    BACKEND summary;
    wassert(actual(summary.data_count()) == 0u);

    summary.add(station, vd, dtrange, 12u);
    wassert(actual(summary.data_count()) == 12u);

    summary.add(station, vd, dtrange, 12u);
    wassert(actual(summary.data_count()) == 24u);

    wassert(actual(get_stations(summary).size()) == 1);
    //wassert(actual(summary.stations().begin()->size()) == 1);

    summary.add(station, vd, dtrange, 12u);
    station.report = "test1";
    summary.add(station, vd, dtrange, 12u);
    summary.add(station, vd, dtrange, 12u);
    wassert(actual(get_stations(summary).size()) == 2);
    //wassert(actual(summary.stations().begin()->size()) == 1);
    //wassert(actual(summary.stations().rbegin()->size()) == 1);
    wassert(actual(summary.data_count()) == 36u + 24u);
});

this->add_method("merge_summaries", [](Fixture& f) {
    BACKEND summary;

    typename BACKEND::station_type station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    summary.add(station, vd, dtrange, 12u);

    BACKEND summary1;
    summary1.add(station, vd, dtrange, 3u);

    auto summary2 = other_summary(summary);
    summary2->add(other_station(station), vd, dtrange, 2u);

    summary.add_summary(summary1);
    summary.add_summary(*summary2);

    wassert(actual(summary.data_count()) == 17u);
});

this->add_method("json_summary", [](Fixture& f) {
    typename BACKEND::station_type station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    core::Query query;
    query.report = "synop";
    BACKEND summary;
    summary.add(station, vd, dtrange, 12);
    vd.varcode = WR_VAR(0, 1, 113);
    summary.add(station, vd, dtrange, 12);

    std::stringstream json;
    core::JSONWriter writer(json);
    summary.to_json(writer);

    wassert(actual(json.str()) == R"({"e":[{"s":{"r":"test","c":[4450000,1150000]},"v":[{"l":[1,null,null,null],"t":[254,0,0],"v":368,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12},{"l":[1,null,null,null],"t":[254,0,0],"v":369,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12}]}]})");

    json.seekg(0);
    core::json::Stream in(json);
    BACKEND summary1;
    wassert(summary1.load_json(in));
    wassert(actual(get_stations(summary1)) == get_stations(summary));
    wassert(actual(get_reports(summary1)) == get_reports(summary));
    wassert(actual(get_levels(summary1)) == get_levels(summary));
    wassert(actual(get_tranges(summary1)) == get_tranges(summary));
    wassert(actual(get_varcodes(summary1)) == get_varcodes(summary));
    wassert_true(summary.datetime_min() == summary1.datetime_min());
    wassert_true(summary.datetime_max() == summary1.datetime_max());
    wassert_true(summary.data_count() == summary1.data_count());

    // Check that load does merge
    json.seekg(0);
    core::json::Stream in1(json);
    wassert(summary1.load_json(in1));
    wassert(actual(get_stations(summary1).size()) == get_stations(summary).size());
    wassert(actual(get_reports(summary1)) == get_reports(summary));
    wassert(actual(get_levels(summary1)) == get_levels(summary));
    wassert(actual(get_tranges(summary1)) == get_tranges(summary));
    wassert(actual(get_varcodes(summary1)) == get_varcodes(summary));
    wassert_true(summary.datetime_min() == summary1.datetime_min());
    wassert_true(summary.datetime_max() == summary1.datetime_max());
    wassert_true(summary.data_count() * 2 == summary1.data_count());
});

this->add_method("datetime_intersect", [](Fixture& f) {
    BACKEND s;

    DBStation station;
    station.id = 1;
    station.report = "synop";
    station.coords = Coords(44.0, 11.0);
    summary::VarDesc vd;
    vd.level = Level(1);
    vd.trange = Trange::instant();
    vd.varcode = WR_VAR(0, 12, 101);
    s.add(station, vd, DatetimeRange(Datetime(2020, 1, 1), Datetime(2020, 2, 1)), 10);

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.min = Datetime(2020, 1, 1);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.min = Datetime(2020, 1, 15);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.min = Datetime(2020, 1, 15);
        query.dtrange.max = Datetime(2020, 1, 16);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.max = Datetime(2020, 1, 16);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.max = Datetime(2020, 2, 1);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.max = Datetime(2020, 2, 2);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.max = Datetime(2020, 1, 1);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 10);
    }

    {
        BACKEND s1;
        core::Query query;
        query.dtrange.max = Datetime(2019, 12, 31);
        s1.add_filtered(s, query);
        wassert(actual(s1.data_count()) == 0);
    }
});

this->add_method("issue218", [](Fixture& f) {
    BACKEND summary;

    typename BACKEND::station_type station;
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    station.report = "test1";
    station.coords = Coords(44.5, 11.5);
    summary.add(station, vd, dtrange, 12);

    station.coords = Coords(45.5, 11.5);
    summary.add(station, vd, dtrange, 12);

    station.report = "test2";
    station.coords = Coords(44.5, 11.5);
    summary.add(station, vd, dtrange, 12);

    station.coords = Coords(45.5, 11.5);
    summary.add(station, vd, dtrange, 12);


    auto reports = get_reports(summary);
    wassert(actual(reports.size()) == 2);
    wassert(actual(reports[0]) == "test1");
    wassert(actual(reports[1]) == "test2");

    {
        core::Query query;
        query.report = "test1";
        BACKEND s1;
        s1.add_filtered(summary, query);

        reports = get_reports(s1);
        wassert(actual(reports.size()) == 1);
        wassert(actual(reports[0]) == "test1");
    }

    {
        core::Query query;
        query.varcodes.insert(WR_VAR(0, 1, 112));
        query.varcodes.insert(WR_VAR(0, 1, 113));
        BACKEND s1;
        s1.add_filtered(summary, query);

        reports = get_reports(s1);
        wassert(actual(reports.size()) == 2);
        wassert(actual(reports[0]) == "test1");
        wassert(actual(reports[1]) == "test2");
    }
});

this->add_method("xapian_filtering_regression1", [](Fixture& f) {
    std::string json_sample = R"({"e":[
        {"s":{"r":"synop","c":[1234560,7654320]},"v":[
            {"l":[10,11,15,22],"t":[20,111,222],"v":267,"d":[[1945,4,25,8,0,0],[1945,4,25,8,0,0]],"c":1},
            {"l":[10,11,15,22],"t":[20,111,222],"v":268,"d":[[1945,4,25,8,0,0],[1945,4,25,8,0,0]],"c":1}
        ]},
        {"s":{"r":"amdar","c":[1234560,7654320],"i":"foo"},"v":[
            {"l":[10,11,15,22],"t":[20,111,223],"v":268,"d":[[1945,4,25,12,0,0],[1945,4,25,12,0,0]],"c":1}
        ]}]})";

    std::stringstream json(json_sample);
    core::json::Stream in(json);
    BACKEND summary;
    wassert(summary.load_json(in));

    BACKEND summary1;
    core::Query query;
    query.report = "amdar";
    summary1.add_filtered(summary, query);

    auto reports = get_reports(summary1);
    wassert(actual(reports.size()) == 1);
    wassert(actual(reports[0]) == "amdar");

    auto tranges = get_tranges(summary1);
    wassert(actual(tranges.size()) == 1);
    wassert(actual(tranges[0]) == Trange(20, 111, 223));
});

}

}
