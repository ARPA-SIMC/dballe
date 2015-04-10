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
#include "bench.h"
#include <iomanip>
#include <iostream>
#include <sys/times.h>
#include <unistd.h>

using namespace std;

namespace {
double ticks_per_sec = sysconf(_SC_CLK_TCK);

}

namespace dballe {
namespace bench {

Task::Task(Benchmark* parent, const std::string& name)
    : parent(parent), name(name)
{
    parent->tasks.push_back(this);
}

void Task::collect(std::function<void()> f)
{
    run_count += 1;

    struct tms tms_start, tms_end;
    times(&tms_start);
    f();
    times(&tms_end);

    utime += tms_end.tms_utime - tms_start.tms_utime;
    stime += tms_end.tms_stime - tms_start.tms_stime;
/*
    log.push_back(LogEntry(bench, name, (double)utime/ticks_per_sec, (double)stime/ticks_per_sec));
    log.back().print(cerr);
*/
}

void Registry::add(Benchmark* b)
{
    benchmarks.push_back(b);
}

Registry& Registry::get()
{
    static Registry* registry = 0;
    if (!registry)
        registry = new Registry();
    return *registry;
}

Benchmark::Benchmark(const std::string& name)
    : name(name), task_main(this, "main")
{
    Registry::get().add(this);
}
Benchmark::~Benchmark() {}

void Benchmark::run(Progress& progress)
{
    progress.start_benchmark(*this);
    setup_main();

    for (unsigned i = 0; i < repetitions; ++i)
    {
        progress.start_iteration(*this, i, repetitions);
        setup_iteration();
        task_main.collect([&]() { main(); });
        teardown_iteration();
        progress.end_iteration(*this, i, repetitions);
    }

    teardown_main();
    progress.end_benchmark(*this);
}

void Benchmark::print_timings()
{
    double ticks_per_second = (double)sysconf(_SC_CLK_TCK);

    for (auto t: tasks)
    {
        fprintf(stdout, "%s.%s: %d times, user: %.2fs, sys: %.2fs, total: %.2fs\n",
                name.c_str(),
                t->name.c_str(),
                t->run_count,
                t->utime / ticks_per_sec,
                t->stime / ticks_per_sec,
                (t->utime + t->stime) / ticks_per_sec);
    }
}

#if 0
LogEntry::LogEntry(const Benchmark& bmark, const std::string& name)
    : b_name(bmark.name), name(name) {}

LogEntry::LogEntry(const Benchmark& bmark, const std::string& name, double utime, double stime)
    : b_name(bmark.name), name(name), utime(utime), stime(stime) {}

void LogEntry::print(std::ostream& out)
{
    out << setprecision(2) << fixed
        << b_name << ":" << name << ": user: " << utime << " sys: " << stime << " total: " << (utime + stime) << endl;
}
#endif

#if 0
Runner::~Runner()
{
    for (auto i : benchmarks)
        delete i;
}

void Runner::add(Benchmark* b)
{
    benchmarks.push_back(b);
}

void Runner::timeit(const Benchmark& bench, const std::string& name, std::function<void()> func, unsigned repeat)
{
    struct tms tms_start, tms_end;
    times(&tms_start);
    for (unsigned i = 0; i < repeat; ++i)
        func();
    times(&tms_end);

    clock_t utime = tms_end.tms_utime - tms_start.tms_utime;
    clock_t stime = tms_end.tms_stime - tms_start.tms_stime;

    log.push_back(LogEntry(bench, name, (double)utime/ticks_per_sec, (double)stime/ticks_per_sec));
    log.back().print(cerr);
}

void Runner::run()
{
    for (auto i : benchmarks)
    {
        i->run(*this);
    }
}

void Runner::dump_csv(std::ostream& out)
{
    out << "Suite,Test,User,System" << endl;
    for (auto l : log)
    {
        out << l.b_name << "," << l.name << "," << l.utime << "," << l.stime << endl;
    }
}
#endif

}
}
