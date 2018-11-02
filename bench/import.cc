#include <dballe/db/db.h>
#include <dballe/file.h>
#include <dballe/core/benchmark.h>
#include <dballe/msg/msg.h>
#include <vector>

struct BenchmarkImport : public dballe::benchmark::Task
{
    dballe::benchmark::Messages messages;
    std::shared_ptr<dballe::db::DB> db;
    const char* m_name;
    const char* m_pathname;
    unsigned hours;
    unsigned minutes;

    BenchmarkImport(const char* name, const char* pathname, unsigned hours=24, unsigned minutes=1)
        : db(dballe::db::DB::connect_test()), m_name(name), m_pathname(pathname), hours(hours), minutes(minutes)
    {
    }

    const char* name() const override { return m_name; }

    void setup() override
    {
        db->reset();
        messages.load(m_pathname);

        // Multiply messages by changing their datetime
        size_t size = messages.size();
        for (unsigned year = 2016; year < 2018; ++year)
            for (unsigned month = 1; month <= 12; ++month)
                for (unsigned hour = 0; hour < hours; ++hour)
                    for (unsigned minute = 0; minute < minutes; ++minute)
                        messages.duplicate(size, dballe::Datetime(year, month, 1, hour, minute));
    }

    void run_once() override
    {
        auto tr = std::dynamic_pointer_cast<dballe::db::Transaction>(db->transaction());
        for (const auto& msgs: messages)
            tr->import_msgs(msgs, nullptr, 0);
        tr->commit();
    }

    void teardown() override
    {
        db->remove_all();
    }
};

int main(int argc, const char* argv[])
{
    using namespace dballe::benchmark;
    dballe::benchmark::Task* tasks[] = {
        new BenchmarkImport("synop", "extra/bufr/synop-rad1.bufr"),
        new BenchmarkImport("temp", "extra/bufr/temp-huge.bufr", 2),
        new BenchmarkImport("acars", "extra/bufr/gts-acars2.bufr", 24, 15),
    };

    Benchmark benchmark;
    dballe::benchmark::Whitelist whitelist(argc, argv);

    for (auto task: tasks)
        if (whitelist.has(task->name()))
            benchmark.timeit(*task);

    benchmark.print_timings();
    return 0;
}
