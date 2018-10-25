#include "benchmark.h"
#include <sys/times.h>
#include <unistd.h>
#include <fnmatch.h>
#include <cmath>
#include <system_error>
#include <algorithm>
#include "dballe/msg/msg.h"
#include "dballe/importer.h"

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

std::string format_clockdiff(const struct timespec& begin, const struct timespec& until)
{
    unsigned long secs = 0;
    unsigned long nsecs = 0;
    if (begin.tv_nsec <= until.tv_nsec)
    {
        secs = until.tv_sec - begin.tv_sec;
        nsecs = until.tv_nsec - begin.tv_nsec;
    } else {
        secs = until.tv_sec - begin.tv_sec - 1;
        nsecs = 1000000000 + until.tv_nsec - begin.tv_nsec;
    }
    char buf[32];
    if (secs > 0)
        snprintf(buf, 32, "%lu.%03lus", secs, nsecs / 1000000);
    else if (nsecs > 1000000)
        snprintf(buf, 32, "%lums", nsecs / 1000000);
    else if (nsecs > 1000)
        snprintf(buf, 32, "%luµs", nsecs / 1000);
    else
        snprintf(buf, 32, "%luns", nsecs);
    return buf;
}


void Timeit::run(Progress& progress, Task& task)
{
    task_name = task.name();
    progress.start_timeit(*this);
    try {
        task.setup();

        bench_getrusage(RUSAGE_SELF, &res_at_start);
        bench_clock_gettime(CLOCK_MONOTONIC_RAW, &time_at_start);
        for (unsigned i = 0; i < repetitions; ++i)
            task.run_once();
        bench_clock_gettime(CLOCK_MONOTONIC_RAW, &time_at_end);
        bench_getrusage(RUSAGE_SELF, &res_at_end);
    } catch (std::exception& e) {
        progress.test_failed(task, e);
    }
    task.teardown();
    progress.end_timeit(*this);
}

void Throughput::run(Progress& progress, Task& task)
{
    task_name = task.name();
    progress.start_throughput(*this);
    try {
        task.setup();
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
            task.run_once();
        }

        run_time = time_cur.tv_sec - time_at_start.tv_sec + (time_cur.tv_nsec - time_at_start.tv_nsec) / 1000000000.0;
    } catch (std::exception& e) {
        progress.test_failed(task, e);
    }
    task.teardown();
    progress.end_throughput(*this);
}

Benchmark::Benchmark()
    : progress(make_shared<BasicProgress>())
{
}

Benchmark::~Benchmark() {}

void Benchmark::timeit(Task& task, unsigned repetitions)
{
    timeit_tasks.emplace_back(Timeit());
    timeit_tasks.back().repetitions = repetitions;
    timeit_tasks.back().run(*progress, task);
}

void Benchmark::throughput(Task& task, double run_time)
{
    throughput_tasks.emplace_back(Throughput());
    throughput_tasks.back().run_time = run_time;
    throughput_tasks.back().run(*progress, task);
}

void Benchmark::print_timings()
{
    for (auto& t: timeit_tasks)
    {
        string time = format_clockdiff(t.time_at_start, t.time_at_end);
        fprintf(stdout, "%s,%u,%s\n", t.task_name.c_str(), t.repetitions, time.c_str());
    }
    for (auto& t: throughput_tasks)
    {
        fprintf(stdout, "%s,%.2f,%d\n", t.task_name.c_str(), t.run_time, t.times_run);
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

BasicProgress::BasicProgress(FILE* out, FILE* err)
    : out(out), err(err) {}

void BasicProgress::start_timeit(const Timeit& t)
{
    fprintf(out, "%s: starting...\n", t.task_name.c_str());
}

void BasicProgress::end_timeit(const Timeit& t)
{
    fprintf(out, "%s: done.\n", t.task_name.c_str());
}

void BasicProgress::start_throughput(const Throughput& t)
{
    fprintf(out, "%s: ", t.task_name.c_str());
    fflush(out);
}

void BasicProgress::end_throughput(const Throughput& t)
{
    fprintf(out, "%u times in %.2fs: %.2f/s.\n", t.times_run, t.run_time, (double)t.times_run/t.run_time);
}

void BasicProgress::test_failed(const Task& t, std::exception& e)
{
    fprintf(err, "%s: failed: %s\n", t.name(), e.what());
}

void Messages::load(const std::string& pathname, dballe::Encoding encoding, const char* codec_options)
{
    auto importer = Importer::create(Encoding::BUFR, ImporterOptions::from_string(codec_options));
    auto in = File::create(encoding, pathname, "rb");
    in->foreach([&](const BinaryMessage& rmsg) {
        emplace_back(importer->from_binary(rmsg));
        return true;
    });
}

void Messages::duplicate(size_t size, const Datetime& datetime)
{
    for (size_t i = 0; i < size; ++i)
        emplace_back((*this)[i]);
}


Whitelist::Whitelist(int argc, const char* argv[])
{
    for (int i = 1; i < argc; ++i)
        emplace_back(argv[i]);
}

bool Whitelist::has(const std::string& val)
{
    if (empty()) return true;
    return std::find(begin(), end(), val) != end();
}

}
}
