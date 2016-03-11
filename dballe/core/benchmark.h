#ifndef WREPORT_BENCHMARK_H
#define WREPORT_BENCHMARK_H

/** @file
 * Simple benchmark infrastructure.
 */

#include <string>
#include <vector>
#include <functional>
#include <cstdio>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

namespace dballe {
namespace benchmark {

struct Benchmark;

/// One task to be measured.
struct Task
{
    // Name of this task
    std::string name;

    Task() {}
    Task(const std::string& name) : name(name) {}
    Task(const Task&) = delete;
    Task(Task&&) = delete;
    virtual ~Task() {}
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;

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

struct TaskHolder
{
    Task* task = nullptr;

    TaskHolder(Task* task) : task(task) {}
    TaskHolder(TaskHolder&& o) : task(o.task) { o.task = nullptr; }
    TaskHolder(const TaskHolder&) = delete;
    TaskHolder& operator=(TaskHolder&& o)
    {
        if (task == o.task) return *this;
        if (task) delete task;
        task = o.task;
        o.task = nullptr;
        return *this;
    }
    TaskHolder& operator=(const TaskHolder&) = delete;
    ~TaskHolder() { delete task; }
};

struct Timeit : TaskHolder
{
    /// How many times to repeat the task for measuring how long it takes
    unsigned repetitions;
    struct timespec time_at_start;
    struct timespec time_at_end;
    struct rusage res_at_start;
    struct rusage res_at_end;

    Timeit(Task* task, int repetitions=10) : TaskHolder(task), repetitions(repetitions) {}

    void run(Progress& progress);
};

struct Throughput : TaskHolder
{
    /// How many seconds to run the task to see how many times per second it runs
    double run_time;
    unsigned times_run = 0;

    Throughput(Task* task, double run_time=0.5) : TaskHolder(task), run_time(run_time) {}

    void run(Progress& progress);
};


/// Notify of progress during benchmark execution
struct Progress
{
    virtual ~Progress() {}

    virtual void start_benchmark(const Benchmark& b) = 0;
    virtual void end_benchmark(const Benchmark& b) = 0;

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
    std::string prefix;
    FILE* out;
    FILE* err;
    std::string cur_benchmark;

    BasicProgress(const std::string& prefix, FILE* out=stdout, FILE* err=stderr);

    void start_benchmark(const Benchmark& b) override;
    void end_benchmark(const Benchmark& b) override;

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
    /// Name of this benchmark
    std::string name;

    /// Tasks for which we time their duration
    std::vector<Timeit> timeit_tasks;

    /// Tasks for which we time their throughput
    std::vector<Throughput> throughput_tasks;


    Benchmark(const std::string& name);
    virtual ~Benchmark();

    virtual void setup() {}
    virtual void teardown() {}

    /// Run the benchmark and collect timings
    void run(Progress& progress);

    /// Print timings to stdout
    void print_timings(const std::string& prefix);

    /// Register tasks to run on this benchmark
    virtual void register_tasks() = 0;
};

/// Collect all existing benchmarks
struct Registry
{
    std::vector<Benchmark*> benchmarks;

    /// Add a benchmark to this registry
    void add(Benchmark* b);

    /**
     * Get the static instance of the registry
     */
    static Registry& get();

    /**
     * Basic implementation of a main function that runs all benchmarks linked
     * into the program. This allows to make a benchmark runner tool with just
     * this code:
     *
     * \code
     * #include <wreport/benchmark.h>
     *
     * int main (int argc, const char* argv[])
     * {
     *     wreport::benchmark::Registry::basic_run(argc, argv);
     * }
     * \endcode
     *
     * If you need different logic in your benchmark running code, you can use
     * the source code of basic_run as a template for writing your own.
     */
    static void basic_run(int argc, const char* argv[]);
};

}
}

#endif
