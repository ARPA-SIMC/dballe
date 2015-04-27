/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "db/test-utils-db.h"
#include "db/mem/cursor.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct dbmem_cursor_shar
{
    dbmem_cursor_shar()
    {
    }

    ~dbmem_cursor_shar()
    {
    }
};
TESTGRP(dbmem_cursor);

template<> template<>
void to::test<1>()
{
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

    wassert(!(actual(k0) < k0));
    wassert(!(actual(k0) < kr));
    wassert(actual(k0) < kc);
    wassert(actual(k0) < km);
    wassert(actual(k0) < kd);
    wassert(actual(k0) < kl);
    wassert(actual(k0) < kt);
    wassert(actual(k0) < kv);

    // kr is the same as k0
    wassert(!(actual(kr) < k0));
    wassert(!(actual(kr) < kr));
    wassert(actual(kr) < kc);
    wassert(actual(kr) < km);
    wassert(actual(kr) < kd);
    wassert(actual(kr) < kl);
    wassert(actual(kr) < kt);
    wassert(actual(kr) < kv);

    wassert(!(actual(km) < k0));
    wassert(!(actual(km) < kr));
    wassert(actual(km) < kc);
    wassert(!(actual(km) < km));
    wassert(!(actual(km) < kd));
    wassert(!(actual(km) < kl));
    wassert(!(actual(km) < kt));
    wassert(!(actual(km) < kv));

    wassert(!(actual(kc) < k0));
    wassert(!(actual(kc) < kr));
    wassert(!(actual(kc) < kc));
    wassert(!(actual(kc) < km));
    wassert(!(actual(kc) < kd));
    wassert(!(actual(kc) < kl));
    wassert(!(actual(kc) < kt));
    wassert(!(actual(kc) < kv));

    wassert(!(actual(kd) < k0));
    wassert(!(actual(kd) < kr));
    wassert(actual(kd) < kc);
    wassert(actual(kd) < km);
    wassert(!(actual(kd) < kd));
    wassert(!(actual(kd) < kl));
    wassert(!(actual(kd) < kt));
    wassert(!(actual(kd) < kv));

    wassert(!(actual(kl) < k0));
    wassert(!(actual(kl) < kr));
    wassert(actual(kl) < kc);
    wassert(actual(kl) < km);
    wassert(actual(kl) < kd);
    wassert(!(actual(kl) < kl));
    wassert(!(actual(kl) < kt));
    wassert(!(actual(kl) < kv));

    wassert(!(actual(kt) < k0));
    wassert(!(actual(kt) < kr));
    wassert(actual(kt) < kc);
    wassert(actual(kt) < km);
    wassert(actual(kt) < kd);
    wassert(actual(kt) < kl);
    wassert(!(actual(kt) < kt));
    wassert(!(actual(kt) < kv));

    wassert(!(actual(kv) < k0));
    wassert(!(actual(kv) < kr));
    wassert(actual(kv) < kc);
    wassert(actual(kv) < km);
    wassert(actual(kv) < kd);
    wassert(actual(kv) < kl);
    wassert(actual(kv) < kt);
    wassert(!(actual(kv) < kv));
}

}
