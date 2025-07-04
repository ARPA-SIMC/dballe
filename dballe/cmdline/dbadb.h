#ifndef DBALLE_CMDLINE_DBADB_H
#define DBALLE_CMDLINE_DBADB_H

#include <cstdio>
#include <dballe/cmdline/processor.h>
#include <dballe/core/query.h>
#include <dballe/db/fwd.h>
#include <dballe/file.h>
#include <list>

namespace dballe {
namespace cmdline {

class Dbadb
{
protected:
    DB& db;

public:
    Dbadb(DB& db) : db(db) {}

    /// Query data in the database and output results as arbitrary human
    /// readable text
    int do_dump(const Query& query, FILE* out);

    /// Query stations in the database and output results as arbitrary human
    /// readable text
    int do_stations(const Query& query, FILE* out);

    /// Export messages and dump their contents to the given file descriptor
    int do_export_dump(const Query& query, FILE* out);

    /// Import the given files
    int do_import(const std::list<std::string>& fnames, Reader& reader,
                  const DBImportOptions& opts);

    /// Import one file
    int do_import(const std::string& fname, Reader& reader,
                  const DBImportOptions& opts);

    /// Export messages writing them to the givne file
    int do_export(const Query& query, File& file,
                  const char* output_template = NULL,
                  const char* forced_repmemo  = NULL);
};

} // namespace cmdline
} // namespace dballe

#endif
