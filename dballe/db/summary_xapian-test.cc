#define _DBALLE_TEST_CODE
#include "config.h"
#include "dballe/db/tests.h"
#include "summary.h"

#ifdef HAVE_XAPIAN
#include "dballe/db/summary_xapian.h"
#include <wreport/utils/sys.h>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

template <typename BACKEND> class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
};

Tests<SummaryXapian> tg1("db_summary_xapian");
Tests<DBSummaryXapian> tg2("db_summary_xapian");

void set_station_id(Station& station, int id) {}
void set_station_id(DBStation& station, int id) { station.id = id; }

template <typename BACKEND>
std::vector<std::string> get_reports(const BACKEND& summary)
{
    std::vector<std::string> res;
    summary.reports([&](const std::string& l) {
        res.emplace_back(l);
        return true;
    });
    return res;
}

template <typename BACKEND> void Tests<BACKEND>::register_tests()
{

    this->add_method("xapian_concurrency", [] {
#ifndef HAVE_XAPIAN
        throw TestSkipped();
#else
    typename BACKEND::station_type station;
    set_station_id(station, 1);
    station.report = "synop";
    station.coords = Coords(44.0, 11.0);
    summary::VarDesc vd;
    vd.level = Level(1);
    vd.trange = Trange::instant();
    vd.varcode = WR_VAR(0, 12, 101);

    sys::rmtree_ifexists("testfile");
    BACKEND summary1("testfile");
    BACKEND summary2("testfile");

    // Build the summary using explorer1
    summary1.add(station, vd, DatetimeRange(Datetime(2020, 1, 1), Datetime(2020, 2, 1)), 10);

    // explorer1 sees no data
    auto reports = get_reports(summary1);
    wassert(actual(reports.size()) == 0);
    // explorer2 sees no data
    reports = get_reports(summary2);
    wassert(actual(reports.size()) == 0);

    summary1.commit();

    // explorer1 sees data
    reports = get_reports(summary1);
    wassert(actual(reports.size()) == 1);
    // explorer2 does not see data without a reopen
    reports = get_reports(summary2);
    wassert(actual(reports.size()) == 0);
#endif
    });
}

} // namespace

#endif
