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

#include "benchmark/bench.h"
#include <vector>
#include <iostream>
#include <config.h>

using namespace std;
using namespace dballe;
using namespace dballe::bench;

struct Progress : bench::Progress
{
    void start_benchmark(const Benchmark& b) override
    {
        printf("%s: starting... ", b.name.c_str());
        fflush(stdout);
    }
    void start_iteration(const Benchmark& b, unsigned cur, unsigned total) override
    {
        printf("\r%s: iteration %u/%u...    ", b.name.c_str(), cur + 1, total);
        fflush(stdout);
    }
    void end_iteration(const Benchmark& b, unsigned cur, unsigned total) override
    {
        printf("\r%s: iteration %u/%u done.", b.name.c_str(), cur + 1, total);
        fflush(stdout);
    }
    void end_benchmark(const Benchmark& b) override
    {
        printf("\r%s: done.                   \r", b.name.c_str());
        fflush(stdout);
    }
};

int main (int argc, const char* argv[])
{
    ::Progress progress;

    // Run all benchmarks
    for (auto& b: Registry::get().benchmarks)
    {
        try {
            b->run(progress);
        } catch (std::exception& e) {
            fprintf(stdout, "\n%s: benchmark failed: %s\n", b->name.c_str(), e.what());
            continue;
        }
        b->print_timings();
    }
}
