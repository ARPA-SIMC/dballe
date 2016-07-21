#include "dballe/db/benchmark.h"
#include "dballe/core/file.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/codec.h"

using namespace dballe;
using namespace std;
using namespace wreport;


namespace {

struct MessageTask : public benchmark::DBTask
{
    vector<Message*> msgs;

    MessageTask(const std::string& pfx, const std::string& fname)
        : benchmark::DBTask(pfx + fname)
    {
        dballe::msg::Importer::Options opts;
        std::unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR, opts);
        unique_ptr<File> f = core::File::open_test_data_file(File::BUFR, "bufr/" + fname);
        f->foreach([&](const BinaryMessage& rmsg) {
            importer->foreach_decoded(rmsg, [&](unique_ptr<Message>&& m) {
                msgs.push_back(m.release());
                return true;
            });
            return true;
        });
    }

    ~MessageTask()
    {
        for (auto& i: msgs) delete i;
    }
};

struct CreateTask : public MessageTask
{
    using MessageTask::MessageTask;

    void run_once() override
    {
        auto t = db->transaction();
        db->remove_all(*t);
        for (auto& m: msgs)
            db->import_msg(*t, *m, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
        t->commit();
    }
};

struct OverwriteTask : public MessageTask
{
    using MessageTask::MessageTask;

    void run_once() override
    {
        auto t = db->transaction();
        for (auto& m: msgs)
            db->import_msg(*t, *m, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
        t->commit();
    }
};


struct B : benchmark::Benchmark
{
    using Benchmark::Benchmark;

    void register_tasks() override
    {
        // Read samples
        for (auto& fn : {
                "synop-cloudbelow.bufr", "synop-evapo.bufr", "synop-groundtemp.bufr", "synop-gtscosmo.bufr", "synop-longname.bufr",
                "temp-gts1.bufr", "temp-gts2.bufr", "temp-gts3.bufr", "temp-gtscosmo.bufr",
                "gts-acars1.bufr", "gts-acars2.bufr", "gts-acars-us1.bufr"
                })
        {
            throughput_tasks.emplace_back(new CreateTask("create_", fn));
            throughput_tasks.emplace_back(new OverwriteTask("overwrite_", fn));
        }
    }
} test("db_import");

}
