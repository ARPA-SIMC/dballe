#ifndef DBALLE_CORE_BENCHMARK_H
#define DBALLE_CORE_BENCHMARK_H

/** @file
 * Simple benchmark infrastructure.
 */

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdio>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dballe/message.h>
#include <dballe/file.h>

namespace dballe {
namespace benchmark {

struct Benchmark;

/// One task to be measured.
struct Task
{
    Task() {}
    Task(const Task&) = delete;
    Task(Task&&) = delete;
    virtual ~Task() {}
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;

    virtual const char* name() const = 0;

    /// Set up the environment for running run_once()
    virtual void setup() {}

    /**
     * Run the task once.
     *
     * It can be called multiple times bewteen setup and teardown in order to
     * perform repeated measurements.
     */
    virtual void run_once() = 0;

    /// Clean up after the task has been measured.
    virtual void teardown() {}
};

struct Progress;

struct Timeit
{
    std::string task_name;
    /// How many times to repeat the task for measuring how long it takes
    unsigned repetitions = 1;
    struct timespec time_at_start;
    struct timespec time_at_end;
    struct rusage res_at_start;
    struct rusage res_at_end;

    void run(Progress& progress, Task& task);
};

struct Throughput
{
    std::string task_name;
    /// How many seconds to run the task to see how many times per second it runs
    double run_time = 0.5;
    unsigned times_run = 0;

    void run(Progress& progress, Task& task);
};


/// Notify of progress during benchmark execution
struct Progress
{
    virtual ~Progress() {}

    virtual void start_timeit(const Timeit& t) = 0;
    virtual void end_timeit(const Timeit& t) = 0;

    virtual void start_throughput(const Throughput& t) = 0;
    virtual void end_throughput(const Throughput& t) = 0;

    virtual void test_failed(const Task& t, std::exception& e) = 0;
};


/**
 * Basic progress implementation writing progress information to the given
 * output stream
 */
struct BasicProgress : Progress
{
    FILE* out;
    FILE* err;

    BasicProgress(FILE* out=stdout, FILE* err=stderr);

    void start_timeit(const Timeit& t) override;
    void end_timeit(const Timeit& t) override;

    void start_throughput(const Throughput& t) override;
    void end_throughput(const Throughput& t) override;

    void test_failed(const Task& t, std::exception& e) override;
};


/**
 * Base class for all benchmarks.
 */
struct Benchmark
{
    /// Progress indicator
    std::shared_ptr<Progress> progress;

    /// Tasks for which we time their duration
    std::vector<Timeit> timeit_tasks;

    /// Tasks for which we time their throughput
    std::vector<Throughput> throughput_tasks;


    Benchmark();
    virtual ~Benchmark();

    /// Run the benchmark and collect timings
    void timeit(Task& task, unsigned repetitions=1);

    /// Run the benchmark and collect timings
    void throughput(Task& task, double run_time=0.5);

    /// Print timings to stdout
    void print_timings();
};


/**
 * Container for parsed messages used for benchmarking
 */
struct Messages : public std::vector<dballe::Messages>
{
    void load(const std::string& pathname, dballe::File::Encoding encoding=dballe::File::BUFR, const char* codec_options="accurate");

    // Copy the first \a size messages, change their datetime, and append them
    // to the vector
    void duplicate(size_t size, const Datetime& datetime);
};

struct Whitelist : protected std::vector<std::string>
{
    Whitelist(int argc, const char* argv[]);

    bool has(const std::string& val);
};

}
}

#endif
