#include "benchmark.h"
#include <sys/times.h>
#include <unistd.h>
#include <fnmatch.h>
#include <cmath>
#include <system_error>

using namespace std;

/*
namespace {
double ticks_per_sec = sysconf(_SC_CLK_TCK);
}
*/

namespace dballe {
namespace benchmark {

/*
void Task::collect(std::function<void()> f)
{
    run_count += 1;

    struct tms tms_start, tms_end;
    times(&tms_start);
    f();
    times(&tms_end);

    utime += tms_end.tms_utime - tms_start.tms_utime;
    stime += tms_end.tms_stime - tms_start.tms_stime;
}
*/

void Registry::add(Benchmark* b)
{
    const char* whitelist = getenv("BENCH_WHITELIST");
    const char* blacklist = getenv("BENCH_BLACKLIST");
    if (whitelist and fnmatch(whitelist, b->name.c_str(), 0) != 0) return;
    if (blacklist and fnmatch(blacklist, b->name.c_str(), 0) == 0) return;
    benchmarks.push_back(b);
}

Registry& Registry::get()
{
    static Registry* registry = 0;
    if (!registry)
        registry = new Registry();
    return *registry;
}

static void bench_getrusage(int who, struct rusage *usage)
{
    if (::getrusage(RUSAGE_SELF, usage) == -1)
        throw std::system_error(errno, std::system_category(), "getrusage failed");
}

static void bench_clock_gettime(clockid_t clk_id, struct timespec *res)
{
    if (::clock_gettime(clk_id, res) == -1)
        throw std::system_error(errno, std::system_category(), "clock_gettime failed");
}


void Timeit::run(Progress& progress)
{
    progress.start_timeit(*this);
    try {
        task->setup();

        bench_getrusage(RUSAGE_SELF, &res_at_end);
        bench_clock_gettime(CLOCK_MONOTONIC_RAW, &time_at_start);
        for (unsigned i = 0; i < repetitions; ++i)
            task->run_once();
        bench_clock_gettime(CLOCK_MONOTONIC_RAW, &time_at_end);
        bench_getrusage(RUSAGE_SELF, &res_at_end);
        /*
        run_count += 1;

        f();

        utime += tms_end.tms_utime - tms_start.tms_utime;
        stime += tms_end.tms_stime - tms_start.tms_stime;
        */
    } catch (std::exception& e) {
        progress.test_failed(*task, e);
    }
    task->teardown();
    progress.end_timeit(*this);
}

void Throughput::run(Progress& progress)
{
    progress.start_throughput(*this);
    try {
        task->setup();
        struct timespec time_at_start;
        bench_clock_gettime(CLOCK_MONOTONIC_RAW, &time_at_start);
        struct timespec time_at_end;
        time_at_end.tv_nsec = time_at_start.tv_nsec + ((long)floor(run_time * 1000000000.0) % 1000000000);
        time_at_end.tv_sec  = time_at_start.tv_sec + time_at_end.tv_nsec / 1000000000 + (long)floor(run_time);
        time_at_end.tv_nsec = time_at_end.tv_nsec % 1000000000;

        struct timespec time_cur;
        for ( ; true; ++times_run)
        {
            bench_clock_gettime(CLOCK_MONOTONIC_RAW, &time_cur);
            if (time_cur.tv_sec > time_at_end.tv_sec) break;
            if (time_cur.tv_sec == time_at_end.tv_sec && time_cur.tv_nsec > time_at_end.tv_nsec) break;
            task->run_once();
        }

        run_time = time_cur.tv_sec - time_at_start.tv_sec + (time_cur.tv_nsec - time_at_start.tv_nsec) / 1000000000.0;
    } catch (std::exception& e) {
        progress.test_failed(*task, e);
    }
    task->teardown();
    progress.end_throughput(*this);
}

Benchmark::Benchmark(const std::string& name)
    : name(name)
{
    Registry::get().add(this);
}
Benchmark::~Benchmark() {}

void Benchmark::run(Progress& progress)
{
    progress.start_benchmark(*this);
    setup();

    register_tasks();

    for (auto& t: timeit_tasks)
        t.run(progress);

    for (auto& t: throughput_tasks)
        t.run(progress);

    teardown();
    progress.end_benchmark(*this);
}

void Benchmark::print_timings(const std::string& prefix)
{
    for (auto& t: timeit_tasks)
        ; // TODO
    for (auto& t: throughput_tasks)
    {
        fprintf(stdout, "%s%s.%s,%.2f,%d\n", prefix.c_str(), name.c_str(), t.task->name.c_str(), t.run_time, t.times_run);
    }
    /*
    for (auto& t: tasks)
    {
        fprintf(stdout, "%s.%s: %d runs, user: %.2fs (%.1f%%), sys: %.2fs (%.1f%%), total: %.2fs (%.1f%%)\n",
                name.c_str(),
                t->name.c_str(),
                t->run_count,
                t->utime / ticks_per_sec,
                t->utime * 100.0 / task_main.utime,
                t->stime / ticks_per_sec,
                t->stime * 100.0 / task_main.stime,
                (t->utime + t->stime) / ticks_per_sec,
                (t->utime + t->stime) * 100.0 / (task_main.utime + task_main.stime));
    }
    */
}

BasicProgress::BasicProgress(const std::string& prefix, FILE* out, FILE* err)
    : prefix(prefix), out(out), err(err) {}

void BasicProgress::start_benchmark(const Benchmark& b)
{
    cur_benchmark = b.name;
    fprintf(out, "%s%s: starting...\n", prefix.c_str(), cur_benchmark.c_str());
}
void BasicProgress::end_benchmark(const Benchmark& b)
{
    fprintf(out, "%s%s: done.\n", prefix.c_str(), b.name.c_str());
}

void BasicProgress::start_timeit(const Timeit& t)
{
    fprintf(out, "%s%s.%s: starting...\n", prefix.c_str(), cur_benchmark.c_str(), t.task->name.c_str());
}

void BasicProgress::end_timeit(const Timeit& t)
{
    fprintf(out, "%s%s.%s: done.\n", prefix.c_str(), cur_benchmark.c_str(), t.task->name.c_str());
}

void BasicProgress::start_throughput(const Throughput& t)
{
    fprintf(out, "%s%s.%s: ", prefix.c_str(), cur_benchmark.c_str(), t.task->name.c_str());
    fflush(out);
}

void BasicProgress::end_throughput(const Throughput& t)
{
    fprintf(out, "%u times in %.2fs: %.2f/s.\n", t.times_run, t.run_time, (double)t.times_run/t.run_time);
}

void BasicProgress::test_failed(const Task& t, std::exception& e)
{
    fprintf(err, "%s%s.%s: failed: %s\n", prefix.c_str(), cur_benchmark.c_str(), t.name.c_str(), e.what());
}

void Registry::basic_run(int argc, const char* argv[])
{
    string prefix;
    if (argc > 1)
    {
        prefix = argv[1];
        prefix += '.';
    }

    BasicProgress progress(prefix, stderr, stderr);

    // Run all benchmarks
    for (auto& b: get().benchmarks)
        b->run(progress);

    for (auto& b: get().benchmarks)
        b->print_timings(prefix);
}

#if 0
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
