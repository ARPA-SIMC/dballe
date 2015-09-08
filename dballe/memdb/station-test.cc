#include "memdb/tests.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "station.h"
#include "results.h"

using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("basic", []() {
            memdb::Stations stations;

            // Insert a fixed station and check that all data is there
            const memdb::Station& stf = *stations[stations.obtain_fixed(Coords(44.0, 11.0), "synop")];
            wassert(actual(stf.coords.dlat()) == 44.0);
            wassert(actual(stf.coords.dlon()) == 11.0);
            wassert(actual(stf.ident) == "");
            wassert(actual(stf.report) == "synop");

            // Insert a mobile station and check that all data is there
            const memdb::Station& stm = *stations[stations.obtain_mobile(Coords(44.0, 11.0), "LH1234", "airep")];
            wassert(actual(stm.coords.dlat()) == 44.0);
            wassert(actual(stm.coords.dlon()) == 11.0);
            wassert(actual(stm.ident) == "LH1234");
            wassert(actual(stm.report) == "airep");

            // Check that lookup returns the same element
            const memdb::Station& stf1 = *stations[stations.obtain_fixed(Coords(44.0, 11.0), "synop")];
            wassert(actual(&stf1) == &stf);
            const memdb::Station& stm1 = *stations[stations.obtain_mobile(Coords(44.0, 11.0), "LH1234", "airep")];
            wassert(actual(&stm1) == &stm);

            // Check again, looking up records
            core::Record sfrec;
            sfrec.set("lat", 44.0);
            sfrec.set("lon", 11.0);
            sfrec.set("rep_memo", "synop");
            const memdb::Station& stf2 = *stations[stations.obtain(sfrec)];
            wassert(actual(&stf2) == &stf);

            core::Record smrec;
            smrec.set("lat", 44.0);
            smrec.set("lon", 11.0);
            smrec.set("ident", "LH1234");
            smrec.set("rep_memo", "airep");
            const memdb::Station& stm2 = *stations[stations.obtain(smrec)];
            wassert(actual(&stm2) == &stm);
        });
        add_method("query_ana_id", []() {
            // Query by ana_id
            memdb::Stations stations;
            size_t pos = stations.obtain_fixed(Coords(44.0, 11.0), "synop");

            core::Query query;

            {
                query.ana_id = pos;
                memdb::Results<memdb::Station> res(stations);
                stations.query(query, res);
                auto items = get_results(res);
                wassert(actual(items.size()) == 1);
                wassert(actual(items[0]->id) == pos);
            }

            {
                query.ana_id = 100;
                memdb::Results<memdb::Station> res(stations);
                stations.query(query, res);
                wassert(actual(res.is_select_all()).isfalse());
                wassert(actual(res.is_empty()).istrue());
            }

            size_t pos1 = stations.obtain_fixed(Coords(45.0, 12.0), "synop");

            {
                query.ana_id = pos;
                memdb::Results<memdb::Station> res(stations);
                stations.query(query, res);
                auto items = get_results(res);
                wassert(actual(items.size()) == 1);
                wassert(actual(items[0]->id) == pos);
            }
        });
        add_method("query_latlon", []() {
            // Query by lat,lon
            memdb::Stations stations;
            size_t pos = stations.obtain_fixed(Coords(44.0, 11.0), "synop");
            stations.obtain_fixed(Coords(45.0, 12.0), "synop");

            memdb::Results<memdb::Station> res(stations);
            stations.query(tests::core_query_from_string("lat=44.0, lon=11.0"), res);

            auto items = get_results(res);
            wassert(actual(items.size()) == 1);
            wassert(actual(items[0]->id) == pos);
        });
        add_method("query_all", []() {
            // Query everything
            memdb::Stations stations;
            size_t pos1 = stations.obtain_fixed(Coords(44.0, 11.0), "synop");
            size_t pos2 = stations.obtain_fixed(Coords(45.0, 12.0), "synop");

            memdb::Results<memdb::Station> res(stations);
            stations.query(core::Query(), res);

            wassert(actual(res.is_select_all()).istrue());
            wassert(actual(res.is_empty()).isfalse());
        });
        add_method("query_multi_latitudes", []() {
            // Query latitudes matching multiple index entries
            memdb::Stations stations;
            size_t pos1 = stations.obtain_fixed(Coords(44.0, 11.0), "synop");
            size_t pos2 = stations.obtain_fixed(Coords(45.0, 11.0), "synop");
            size_t pos3 = stations.obtain_fixed(Coords(46.0, 11.0), "synop");

            memdb::Results<memdb::Station> res(stations);
            stations.query(tests::core_query_from_string("latmin=45.0"), res);

            auto items = get_results(res);
            wassert(actual(items.size()) == 2);
            wassert(actual(items[0]->id) == pos2);
            wassert(actual(items[1]->id) == pos3);
        });
    }
} test("memdb_station");

}

#include "results.tcc"
