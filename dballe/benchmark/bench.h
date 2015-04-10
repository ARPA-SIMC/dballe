/*
 * bench - Simple benchmark infrastructure
 *
 * Copyright (C) 2013--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#ifndef DBALLE_BENCHMARK_H
#define DBALLE_BENCHMARK_H

#include <string>
#include <vector>
#include <functional>

namespace dballe {
namespace bench {

struct Benchmark;

/// Collect timings for one task
struct Task
{
    // Unmanaged pointer to the benchmark we belong to
    Benchmark* parent;
    // Name of this task
    std::string name;
    // Number of time this task has run
    unsigned run_count = 0;
    // Total user time
    clock_t utime = 0;
    // Total system time
    clock_t stime = 0;

    Task(Benchmark* parent, const std::string& name);

    // Run the given function and collect timings for it
    void collect(std::function<void()> f);
};


/// Notify of progress during benchmark execution
struct Progress
{
    virtual ~Progress() {}

    virtual void start_benchmark(const Benchmark& b) = 0;
    virtual void end_benchmark(const Benchmark& b) = 0;
    virtual void start_iteration(const Benchmark& b, unsigned cur, unsigned total) = 0;
    virtual void end_iteration(const Benchmark& b, unsigned cur, unsigned total) = 0;
};


/**
 * Base class for all benchmarks.
 */
struct Benchmark
{
    // Name of this benchmark
    std::string name;
    // Number of repetitions
    unsigned repetitions = 10;
    // Unmanaged pointers to the tasks in this benchmark
    std::vector<Task*> tasks;
    // Main task, collecting timings for the toplevel run
    Task task_main;

    Benchmark(const std::string& name);
    virtual ~Benchmark();

    /**
     * Set up the environment for this benchmark.
     *
     * This is run outside of timings. By default it does nothing.
     */
    virtual void setup_main() {}

    /**
     * Tear down the environment for this benchmark.
     *
     * This is run outside of timings. By default it does nothing.
     */
    virtual void teardown_main() {}

    /**
     * Set up the environment for an iteration of this benchmark.
     *
     * This is run outside of timings. By default it does nothing.
     */
    virtual void setup_iteration() {}

    /**
     * Tear down the environment for an iteration of this benchmark.
     *
     * This is run outside of timings. By default it does nothing.
     */
    virtual void teardown_iteration() {}

    /// Run the benchmark and collect timings
    void run(Progress& progress);

    /// Print timings to stdout
    void print_timings();

    /// Main body of this benchmark
    virtual void main() = 0;
};

/// Collect all existing benchmarks
struct Registry
{
    std::vector<Benchmark*> benchmarks;

    void add(Benchmark* b);

    static Registry& get();
};

}
}

#endif
