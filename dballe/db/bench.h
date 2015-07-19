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
#ifndef DBALLE_DB_BENCH_UTILS_H
#define DBALLE_DB_BENCH_UTILS_H

#include <wreport/benchmark.h>
#include <dballe/db/db.h>

namespace dballe {
namespace bench {

struct DBBenchmark : wreport::benchmark::Benchmark
{
    using wreport::benchmark::Benchmark::Benchmark;

    std::unique_ptr<DB> db;

    void setup_main() override
    {
        db = DB::connect_test();
        db->reset();
    }

    void teardown_main() override
    {
        db->disappear();
        db.reset(0);
    }

    void setup_iteration()
    {
        db->remove_all();
    }
};

}
}

#endif
