#ifndef DBALLE_DB_BENCH_UTILS_H
#define DBALLE_DB_BENCH_UTILS_H

#include <dballe/core/benchmark.h>
#include <dballe/db/db.h>

namespace dballe {
namespace benchmark {

struct DBTask : Task
{
    std::shared_ptr<DB> db;

    using Task::Task;

    void setup() override
    {
        db = DB::connect_test();
        db->reset();
    }

    void teardown() override
    {
        db->disappear();
        db.reset();
    }
};

struct ExistingDBTask : Task
{
    DB& db;

    ExistingDBTask(DB& db, const std::string& name)
        : Task(name), db(db) {}
};

struct DBBenchmark : Benchmark
{
    std::shared_ptr<DB> db;

    using Benchmark::Benchmark;

    void setup() override
    {
        db = DB::connect_test();
        db->reset();
    }

    void teardown() override
    {
        db->disappear();
        db.reset();
    }
};

}
}
#endif
