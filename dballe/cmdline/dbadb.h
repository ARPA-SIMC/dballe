/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_CMDLINE_DBADB_H
#define DBALLE_CMDLINE_DBADB_H

#include <dballe/core/file.h>
#include <dballe/core/record.h>
#include <dballe/cmdline/processor.h>
#include <memory>
#include <cstdio>

namespace dballe {
struct DB;

namespace cmdline {

namespace dbadb {

struct Importer : public Action
{
    DB& db;
    int import_flags;
    const char* forced_repmemo;

    Importer(DB& db) : db(db), import_flags(0), forced_repmemo(0) {}

    virtual void operator()(const cmdline::Item& item);
};

const char* parse_op_report(DB& db, const char* name="");

}

class Dbadb
{
protected:
    DB& db;

public:
    Dbadb(DB& db) : db(db) {}

    /// Query data in the database and output results as arbitrary human readable text
    int do_dump(const Record& query, FILE* out);

    /// Query stations in the database and output results as arbitrary human readable text
    int do_stations(const Record& query, FILE* out);

    /// Export messages and dump their contents to the given file descriptor
    int do_export_dump(const Record& query, FILE* out);

    /// Export messages writing them to the givne file
    int do_export(const Record& query, std::auto_ptr<File> file, const char* output_template=NULL, const char* forced_repmemo=NULL);
};


}
}

#endif
