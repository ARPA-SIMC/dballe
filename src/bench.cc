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

#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <wreport/bulletin.h>
#include <wibble/string.h>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sys/times.h>
#include <unistd.h>
#include <config.h>

using namespace std;
using namespace wibble;
using namespace wreport;
using namespace dballe;

static double ticks_per_sec = sysconf(_SC_CLK_TCK);

struct LogEntry
{
    std::string name;
    double utime;
    double stime;

    LogEntry(const std::string& name)
        : name(name), utime(0), stime(0) {}

    LogEntry(const std::string& name, double utime, double stime)
        : name(name), utime(utime), stime(stime) {}
};

struct Benchmark
{
    vector<LogEntry> log;
    string name;
    string desc;

    Benchmark(const std::string& name, const std::string& desc)
        : name(name), desc(desc) {}
    virtual ~Benchmark() {}

    void timeit(const std::string& name, std::function<void()> func)
    {
        struct tms tms_start, tms_end;
        times(&tms_start);
        func();
        times(&tms_end);

        clock_t utime = tms_end.tms_utime - tms_start.tms_utime;
        clock_t stime = tms_end.tms_stime - tms_start.tms_stime;

        log.push_back(LogEntry(name, (double)utime/ticks_per_sec, (double)stime/ticks_per_sec));
        print_log_entry(log.back());
    }

    void print_log_entry(const LogEntry& log)
    {
        cout << setprecision(2) << fixed
             << name << ":" << log.name << ": user: " << log.utime << " sys: " << log.stime << " total: " << (log.utime + log.stime) << endl;
    }

    virtual void run() = 0;
};

struct FileBenchmark : public Benchmark
{
    string fname;
    vector<Rawmsg*> raw_messages;
    vector<Bulletin*> bulletins;
    vector<Msgs*> messages;

    FileBenchmark(const std::string& name, const std::string& desc, const std::string& fname)
        : Benchmark(name, desc), fname(str::joinpath(BENCHDIR, fname))
    {
    }

    ~FileBenchmark()
    {
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

    virtual void run() override
    {
        timeit("read", [this] { read_file(); });
        timeit("decode", [this] { decode_bufr(); });
    }
};

struct Runner
{
    std::vector<Benchmark*> benchmarks;

    ~Runner()
    {
        for (auto i : benchmarks)
            delete i;
    }

    void add(Benchmark* b)
    {
        benchmarks.push_back(b);
    }

    void run()
    {
        for (auto i : benchmarks)
        {
            i->run();
        }
    }
};

int main (int argc, const char* argv[])
{
    Runner runner;
    runner.add(new FileBenchmark("airep", "mobile stations, many levels", "airep_20130615_20130620.bufr"));
    runner.run();
    return 0;
}
