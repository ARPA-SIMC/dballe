#include "db/tests.h"
#include "db/mem/cursor.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("basic", []() {
            using namespace dballe::db::mem::cursor;

            Memdb memdb;
            size_t v0 = memdb.insert(Coords(11, 45), "", "synop", Level(1), Trange(254), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vr = memdb.insert(Coords(11, 45), "", "temp", Level(1), Trange(254), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vm = memdb.insert(Coords(11, 45), "LH1234", "synop", Level(1), Trange(254), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vc = memdb.insert(Coords(11, 46), "", "synop", Level(1), Trange(254), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vd = memdb.insert(Coords(11, 45), "", "synop", Level(1), Trange(254), Datetime(2013, 11, 1, 13), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vl = memdb.insert(Coords(11, 45), "", "synop", Level(2), Trange(254), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vt = memdb.insert(Coords(11, 45), "", "synop", Level(1), Trange(255), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 101), 15.0));
            size_t vv = memdb.insert(Coords(11, 45), "", "synop", Level(1), Trange(254), Datetime(2013, 11, 1, 12), newvar(WR_VAR(0, 12, 102), 15.0));

            DataBestKey k0(memdb.values, v0);
            DataBestKey km(memdb.values, vm);
            DataBestKey kc(memdb.values, vc);
            DataBestKey kr(memdb.values, vr);
            DataBestKey kl(memdb.values, vl);
            DataBestKey kt(memdb.values, vt);
            DataBestKey kd(memdb.values, vd);
            DataBestKey kv(memdb.values, vv);

            wassert(actual(k0 < k0).isfalse());
            wassert(actual(k0 < kr).isfalse());
            wassert(actual(k0) < kc);
            wassert(actual(k0) < km);
            wassert(actual(k0) < kd);
            wassert(actual(k0) < kl);
            wassert(actual(k0) < kt);
            wassert(actual(k0) < kv);

            // kr is the same as k0
            wassert(actual(kr < k0).isfalse());
            wassert(actual(kr < kr).isfalse());
            wassert(actual(kr) < kc);
            wassert(actual(kr) < km);
            wassert(actual(kr) < kd);
            wassert(actual(kr) < kl);
            wassert(actual(kr) < kt);
            wassert(actual(kr) < kv);

            wassert(actual(km < k0).isfalse());
            wassert(actual(km < kr).isfalse());
            wassert(actual(km) < kc);
            wassert(actual(km < km).isfalse());
            wassert(actual(km < kd).isfalse());
            wassert(actual(km < kl).isfalse());
            wassert(actual(km < kt).isfalse());
            wassert(actual(km < kv).isfalse());

            wassert(actual(kc < k0).isfalse());
            wassert(actual(kc < kr).isfalse());
            wassert(actual(kc < kc).isfalse());
            wassert(actual(kc < km).isfalse());
            wassert(actual(kc < kd).isfalse());
            wassert(actual(kc < kl).isfalse());
            wassert(actual(kc < kt).isfalse());
            wassert(actual(kc < kv).isfalse());

            wassert(actual(kd < k0).isfalse());
            wassert(actual(kd < kr).isfalse());
            wassert(actual(kd) < kc);
            wassert(actual(kd) < km);
            wassert(actual(kd < kd).isfalse());
            wassert(actual(kd < kl).isfalse());
            wassert(actual(kd < kt).isfalse());
            wassert(actual(kd < kv).isfalse());

            wassert(actual(kl < k0).isfalse());
            wassert(actual(kl < kr).isfalse());
            wassert(actual(kl) < kc);
            wassert(actual(kl) < km);
            wassert(actual(kl) < kd);
            wassert(actual(kl < kl).isfalse());
            wassert(actual(kl < kt).isfalse());
            wassert(actual(kl < kv).isfalse());

            wassert(actual(kt < k0).isfalse());
            wassert(actual(kt < kr).isfalse());
            wassert(actual(kt) < kc);
            wassert(actual(kt) < km);
            wassert(actual(kt) < kd);
            wassert(actual(kt) < kl);
            wassert(actual(kt < kt).isfalse());
            wassert(actual(kt < kv).isfalse());

            wassert(actual(kv < k0).isfalse());
            wassert(actual(kv < kr).isfalse());
            wassert(actual(kv) < kc);
            wassert(actual(kv) < km);
            wassert(actual(kv) < kd);
            wassert(actual(kv) < kl);
            wassert(actual(kv) < kt);
            wassert(actual(kv < kv).isfalse());
        });
    }
} test("dbmem_cursor");

}
