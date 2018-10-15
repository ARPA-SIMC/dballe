#include <dballe/file.h>
#include <dballe/importer.h>
#include <dballe/db/db.h>
#include <vector>
#include <cstring>
#include <cstdio>

using namespace dballe;
using namespace std;

struct Scenario
{
    virtual ~Scenario() {}

    virtual const char* name() const = 0;

    virtual int run(int argc, const char* argv[]) = 0;
};


struct ImportSynopOneStation: public Scenario
{
    std::vector<std::vector<std::shared_ptr<Message>>> input;

    const char* name() const override { return "import_synop_one_station"; }

    void read_input()
    {
        unique_ptr<File> f = File::create(File::BUFR, "extra/bufr/cdfin_synop.bufr", "r");
        std::unique_ptr<Importer> importer = Importer::create(File::BUFR);
        f->foreach([&](const BinaryMessage& msg) {
            input.emplace_back(importer->from_binary(msg));
            return true;
        });
    }

    void import()
    {
        auto db = DB::connect_test();
        db->reset();
        auto t = db->transaction();
        for (const auto& msgs: input)
            for (const auto& msg: msgs)
                t->import_msg(*msg, "synop", DBA_IMPORT_ATTRS | DBA_IMPORT_OVERWRITE);
        t->commit();
    }

    int run(int argc, const char* argv[]) override
    {
        read_input();
        import();
        return 0;
    }
};


struct ImportSynopManyTimes: public Scenario
{
    std::vector<std::shared_ptr<Message>> messages;

    const char* name() const override { return "import_synop_many_times"; }

    void read_input()
    {
        unique_ptr<File> f = File::create(File::BUFR, "extra/bufr/synop-groundtemp.bufr", "r");
        std::unique_ptr<Importer> importer = Importer::create(File::BUFR);
        f->foreach([&](const BinaryMessage& msg) {
            messages = importer->from_binary(msg);
            return true;
        });
    }

    void import()
    {
        auto db = DB::connect_test();
        db->reset();
        {
            auto t = db->transaction();
            for (const auto& msg: messages)
                t->import_msg(*msg, "synop", DBA_IMPORT_ATTRS | DBA_IMPORT_OVERWRITE);
            t->commit();
        }
        {
            auto t = db->transaction();
            for (const auto& msg: messages)
                t->import_msg(*msg, "synop", DBA_IMPORT_ATTRS | DBA_IMPORT_OVERWRITE);
            t->commit();
        }
        {
            auto t = db->transaction();
            for (const auto& msg: messages)
                t->import_msg(*msg, "synop", DBA_IMPORT_ATTRS | DBA_IMPORT_OVERWRITE);
            t->commit();
        }
    }

    int run(int argc, const char* argv[]) override
    {
        read_input();
        import();
        return 0;
    }
};


struct Profile
{
    vector<Scenario*> profiles;
    const char* argv0 = nullptr;

    Profile(const char* argv0)
        : argv0(argv0)
    {
        profiles.push_back(new ImportSynopOneStation);
        profiles.push_back(new ImportSynopManyTimes);
    }

    ~Profile()
    {
        for (auto i: profiles)
            delete i;
    }

    void usage(FILE* out)
    {
        fprintf(out, "Usage: %s {", argv0);
        for (auto i = profiles.begin(); i != profiles.end(); ++i)
        {
            if (i != profiles.begin()) putc('|', out);
            fputs((*i)->name(), out);
        }
        fputs("} [args...]\n", out);
    }

    int run(const char* name, int argc, const char* argv[])
    {
        for (auto& scen: profiles)
        {
            if (strcmp(name, scen->name()) != 0) continue;
            return scen->run(argc, argv);
        }

        fprintf(stderr, "Unrecognised scenario name: %s\n", name);
        usage(stderr);
        return 1;
    }
};

int main (int argc, const char* argv[])
{
    Profile profile(argv[0]);

    if (argc < 2)
    {
        profile.usage(stderr);
        return 1;
    }

    return profile.run(argv[1], argc - 2, argv + 2);
}
