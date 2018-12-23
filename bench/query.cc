#include <dballe/db/db.h>
#include <dballe/file.h>
#include <dballe/core/benchmark.h>
#include <dballe/core/query.h>
#include <dballe/msg/msg.h>
#include <vector>

struct BenchmarkQuery : public dballe::benchmark::Task
{
    std::shared_ptr<dballe::db::DB> db;
    const char* m_name;
    const char* m_pathname;
    unsigned months;
    unsigned hours;
    unsigned minutes;

    BenchmarkQuery(const char* name, const char* pathname, unsigned months=12, unsigned hours=24, unsigned minutes=1)
        : m_name(name), m_pathname(pathname), months(months), hours(hours), minutes(minutes)
    {
        auto options = dballe::DBConnectOptions::test_create();
        db = dballe::db::DB::downcast(dballe::DB::connect(*options));
    }

    const char* name() const override { return m_name; }

    void setup() override
    {
        db->reset();
        dballe::benchmark::Messages messages;
        messages.load(m_pathname);

        // Multiply messages by changing their datetime
        size_t size = messages.size();
        for (unsigned year = 2016; year < 2018; ++year)
            for (unsigned month = 1; month <= months; ++month)
                for (unsigned hour = 0; hour < hours; ++hour)
                    for (unsigned minute = 0; minute < minutes; ++minute)
                        messages.duplicate(size, dballe::Datetime(year, month, 1, hour, minute));

        auto tr = db->transaction();
        for (const auto& msgs: messages)
            tr->import_messages(msgs);
        tr->commit();
    }

    void run_once() override
    {
        auto tr = std::dynamic_pointer_cast<dballe::db::Transaction>(db->transaction());
        dballe::core::Query query;
        auto cur = tr->query_data(query);
        while (cur->next())
            ;
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
        new BenchmarkQuery("synop", "extra/bufr/synop-rad1.bufr", 1, 24),
        new BenchmarkQuery("temp", "extra/bufr/temp-huge.bufr", 1, 1),
        new BenchmarkQuery("acars", "extra/bufr/gts-acars2.bufr", 12, 24, 10),
    };

    Benchmark benchmark;
    dballe::benchmark::Whitelist whitelist(argc, argv);

    for (auto task: tasks)
        if (whitelist.has(task->name()))
            benchmark.timeit(*task, 20);

    benchmark.print_timings();
    return 0;
}

