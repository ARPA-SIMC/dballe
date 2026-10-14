#include <cstdio>
#include <cstring>
#include <dballe/db/db.h>
#include <dballe/file.h>
#include <dballe/importer.h>
#include <vector>

using namespace dballe;
using namespace std;

struct Scenario
{
    virtual ~Scenario() {}

    virtual const char* name() const = 0;

    virtual int run(int argc, const char* argv[]) = 0;
};

struct ImportSynopOneStation : public Scenario
{
    std::vector<std::vector<std::shared_ptr<Message>>> input;

    const char* name() const override { return "import_synop_one_station"; }

    void read_input()
    {
        unique_ptr<File> f =
            File::create(Encoding::BUFR, "extra/bufr/cdfin_synop.bufr", "r");
        std::unique_ptr<Importer> importer = Importer::create(Encoding::BUFR);
        f->foreach ([&](const BinaryMessage& msg) {
            input.emplace_back(importer->from_binary(msg));
            return true;
        });
    }

    void import()
    {
        auto options = DBConnectOptions::test_create();
        auto db      = db::DB::downcast(DB::connect(*options));
        db->reset();
        auto t                  = db->transaction();
        auto opts               = DBImportOptions::create();
        opts->report            = "synop";
        opts->import_attributes = true;
        opts->overwrite         = true;
        for (const auto& msgs : input)
            t->import_messages(msgs, *opts);
        t->commit();
    }

    int run(int argc, const char* argv[]) override
    {
        read_input();
        import();
        return 0;
    }
};

struct ImportSynopManyTimes : public Scenario
{
    std::vector<std::shared_ptr<Message>> messages;

    const char* name() const override { return "import_synop_many_times"; }

    void read_input()
    {
        unique_ptr<File> f = File::create(
            Encoding::BUFR, "extra/bufr/synop-groundtemp.bufr", "r");
        std::unique_ptr<Importer> importer = Importer::create(Encoding::BUFR);
        f->foreach ([&](const BinaryMessage& msg) {
            messages = importer->from_binary(msg);
            return true;
        });
    }

    void import()
    {
        auto options = DBConnectOptions::test_create();
        auto db      = db::DB::downcast(DB::connect(*options));
        db->reset();
        auto opts               = DBImportOptions::create();
        opts->report            = "synop";
        opts->import_attributes = true;
        opts->overwrite         = true;
        {
            auto t = db->transaction();
            t->import_messages(messages, *opts);
            t->commit();
        }
        {
            auto t = db->transaction();
            t->import_messages(messages, *opts);
            t->commit();
        }
        {
            auto t = db->transaction();
            t->import_messages(messages, *opts);
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

    Profile(const char* argv0) : argv0(argv0)
    {
        profiles.push_back(new ImportSynopOneStation);
        profiles.push_back(new ImportSynopManyTimes);
    }

    ~Profile()
    {
        for (auto i : profiles)
            delete i;
    }

    void usage(FILE* out)
    {
        fprintf(out, "Usage: %s {", argv0);
        for (auto i = profiles.begin(); i != profiles.end(); ++i)
        {
            if (i != profiles.begin())
                putc('|', out);
            fputs((*i)->name(), out);
        }
        fputs("} [args...]\n", out);
    }

    int run(const char* name, int argc, const char* argv[])
    {
        for (auto& scen : profiles)
        {
            if (strcmp(name, scen->name()) != 0)
                continue;
            return scen->run(argc, argv);
        }

        fprintf(stderr, "Unrecognised scenario name: %s\n", name);
        usage(stderr);
        return 1;
    }
};

int main(int argc, const char* argv[])
{
    Profile profile(argv[0]);

    if (argc < 2)
    {
        profile.usage(stderr);
        return 1;
    }

    return profile.run(argv[1], argc - 2, argv + 2);
}
