/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "bench/bench.h"
#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <dballe/core/record.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>
#include <dballe/db/db.h>
#include <wreport/bulletin.h>
#include <wibble/string.h>
#include <vector>
#include <config.h>

using namespace std;
using namespace wibble;
using namespace wreport;
using namespace dballe;
using namespace dballe::bench;

namespace {

// Set a record from a ", "-separated string of assignments
void set_record_from_string(Record& rec, const std::string& s)
{
     str::Split splitter(", ", s);
     for (str::Split::const_iterator i = splitter.begin(); i != splitter.end(); ++i)
         rec.set_from_string(i->c_str());
}

struct DBBenchmark : public Benchmark
{
    DB& db;
    Record query;
    unsigned repeat;

    DBBenchmark(const std::string& name, const std::string& desc, DB& db, unsigned repeat=1)
        : Benchmark(name, desc), db(db), repeat(repeat) {}

    void query_stations()
    {
        auto c = db.query_stations(query);
        c->test_iterate();
    }

    void query_data()
    {
        auto c = db.query_data(query);
        c->test_iterate();
    }

    void query_data_best()
    {
        Record rec(query);
        rec.set_from_string("query=best");
        auto c = db.query_data(rec);
        c->test_iterate();
    }

    void query_summary()
    {
        auto c = db.query_summary(query);
        c->test_iterate();
    }

    virtual void run(Runner& runner) override
    {
        runner.timeit(*this, "query_stations", [this] { query_stations(); }, repeat);
        runner.timeit(*this, "query_data", [this] { query_data(); }, repeat);
        runner.timeit(*this, "query_data_best", [this] { query_data_best(); }, repeat);
        runner.timeit(*this, "query_summary", [this] { query_summary(); }, repeat);
    }
};

struct FileBenchmark : public Benchmark
{
    string fname;
    string query_anaid{"ana_id=1"};
    string query_anall{"latmin=42, latmax=46, lonmin=9, lonmax=13"};
    string query_level{"leveltype1=1"};
    string query_report{"rep_memo=synop"};
    string query_datetime{"yearmin=2013 monthmin=6 daymin=20 yearmax=2013 monthmax=6 daymax=21"};
    string query_varcode{"var=B12101"};

    vector<Rawmsg*> raw_messages;
    vector<Bulletin*> bulletins;
    vector<Msgs*> messages;
    DB* db;

    FileBenchmark(const std::string& name, const std::string& desc, const std::string& fname)
        : Benchmark(name, desc), fname(str::joinpath(BENCHDIR, fname))
    {
        db = DB::connect_test().release();
        db->reset();
    }

    ~FileBenchmark()
    {
        delete db;
        for (auto i : messages) delete i;
        for (auto i : bulletins) delete i;
        for (auto i : raw_messages) delete i;
    }

    void read_file()
    {
        std::unique_ptr<File> f(File::create(BUFR, fname, "r").release());
        while (true)
        {
            unique_ptr<Rawmsg> rm(new Rawmsg);
            if (f->read(*rm))
                raw_messages.push_back(rm.release());
            else
                break;
        }
    }

    void decode_bufr()
    {
        for (auto rm : raw_messages)
        {
            unique_ptr<BufrBulletin> bulletin(BufrBulletin::create().release());
            bulletin->decode(*rm, rm->file.c_str(), rm->offset);
            bulletins.push_back(bulletin.release());
        }
    }

    void interpret_bulletins()
    {
        for (auto b : bulletins)
        {
            std::auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR);
            std::unique_ptr<Msgs> msgs(new Msgs);
            importer->from_bulletin(*b, *msgs);
            messages.push_back(msgs.release());
        }
    }

    void import()
    {
        for (auto m : messages)
        {
            db->import_msgs(*m, NULL, 0);
        }
    }

    virtual void run(Runner& runner) override
    {
        runner.timeit(*this, "read", [this] { read_file(); });
        runner.timeit(*this, "decode", [this] { decode_bufr(); });
        runner.timeit(*this, "interpret", [this] { interpret_bulletins(); });
        runner.timeit(*this, "import", [this] { import(); });

        DBBenchmark db_all(name + ".db_all", "Query with an empty filter", *db);
        db_all.run(runner);

        DBBenchmark db_anaid(name + ".db_anaid", "Query by station ID", *db, 10);
        set_record_from_string(db_anaid.query, query_anaid);
        db_anaid.run(runner);

        DBBenchmark db_anall(name + ".db_anall", "Query by station coordinates", *db, 10);
        set_record_from_string(db_anall.query, query_anall);
        db_anall.run(runner);

        DBBenchmark db_level(name + ".db_level", "Query by level", *db, 10);
        set_record_from_string(db_level.query, query_level);
        db_level.run(runner);

        DBBenchmark db_report(name + ".db_report", "Query by report", *db, 10);
        set_record_from_string(db_report.query, query_report);
        db_report.run(runner);

        DBBenchmark db_datetime(name + ".db_datetime", "Query by datetime", *db);
        set_record_from_string(db_datetime.query, query_datetime);
        db_datetime.run(runner);

        DBBenchmark db_varcode(name + ".db_varcode", "Query by varcode", *db, 10);
        set_record_from_string(db_varcode.query, query_varcode);
        db_varcode.run(runner);
    }
};

}

int main (int argc, const char* argv[])
{
    Runner runner;
    FileBenchmark* fb;
    runner.add(fb = new FileBenchmark("airep", "mobile stations, many levels", "airep_20130615_20130620.bufr"));
    runner.run();
    return 0;
}
