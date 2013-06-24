/*
 * bench - Simple benchmark infrastructure
 *
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
#ifndef DBA_BENCHMARK_H
#define DBA_BENCHMARK_H

#include <string>
#include <vector>
#include <functional>

namespace dballe {
namespace bench {

struct Runner;

struct Benchmark
{
    std::string name;
    std::string desc;

    Benchmark(const std::string& name, const std::string& desc);
    virtual ~Benchmark();

    virtual void run(Runner& runner) = 0;
};

struct LogEntry
{
    std::string b_name;
    std::string b_desc;
    std::string name;
    double utime = 0;
    double stime = 0;

    LogEntry(const Benchmark& bmark, const std::string& name);
    LogEntry(const Benchmark& bmark, const std::string& name, double utime, double stime);

    void print(std::ostream& out);
};

struct Runner
{
    std::vector<Benchmark*> benchmarks;
    std::vector<LogEntry> log;

    ~Runner();

    /// Add a benchmark to this runner
    void add(Benchmark* b);

    /// Time the execution of a function, logging it with the given benchmark
    /// and function name
    void timeit(const Benchmark& bench, const std::string& name, std::function<void()> func, unsigned repeat=1);

    /// Run all benchmarks that have been added to this runner
    void run();
};


}
}

#endif
