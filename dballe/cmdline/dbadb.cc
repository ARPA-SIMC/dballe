#include "dbadb.h"
#include "dballe/message.h"
#include "dballe/msg/msg.h"
#include "dballe/values.h"
#include "dballe/db/db.h"

#include <cstdlib>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

namespace {

struct Importer : public Action
{
    dballe::DB& db;
    const DBImportOptions& opts;
    std::shared_ptr<dballe::Transaction> transaction;

    Importer(dballe::DB& db, const DBImportOptions& opts) : db(db), opts(opts) {}

    virtual bool operator()(const cmdline::Item& item);
    void commit()
    {
        if (transaction.get())
            transaction->commit();
    }
};

bool Importer::operator()(const Item& item)
{
    if (!transaction.get())
        transaction = db.transaction();

    if (item.msgs == NULL)
    {
        fprintf(stderr, "Message #%d cannot be parsed: ignored\n", item.idx);
        return false;
    }
    try {
        transaction->import_messages(*item.msgs, opts);
    } catch (std::exception& e) {
        item.processing_failed(e);
    }
    return true;
}

}

/// Query data in the database and output results as arbitrary human readable text
int Dbadb::do_dump(const Query& query, FILE* out)
{
    auto tr = db.transaction();
    auto cursor = tr->query_data(query);
    for (unsigned i = 0; cursor->next(); ++i)
    {
        fprintf(out, "#%u: -----------------------\n", i);
        fprintf(out, "Station: "); cursor->get_station().print(out);
        fprintf(out, "Datetime: "); cursor->get_datetime().print(out);
        fprintf(out, "Level: "); cursor->get_level().print(out);
        fprintf(out, "Trange: "); cursor->get_trange().print(out);
        fprintf(out, "Var: "); cursor->get_var().print(out);
    }

    tr->rollback();
    return 0;
}

/// Query stations in the database and output results as arbitrary human readable text
int Dbadb::do_stations(const Query& query, FILE* out)
{
    auto tr = db.transaction();
    auto cursor = tr->query_stations(query);
    for (unsigned i = 0; cursor->next(); ++i)
    {
        fprintf(out, "#%u: -----------------------\n", i);
        fprintf(out, "Station: "); cursor->get_station().print(out);
        auto values = cursor->get_values();
        for (const auto& val: values)
        {
            fprintf(out, "Var: "); val->print(out);
        }
    }

    tr->rollback();
    return 0;
}

int Dbadb::do_export_dump(const Query& query, FILE* out)
{
    auto cursor = db.query_messages(query);
    while (cursor->next())
        cursor->get_message().print(out);
    return 0;
}

int Dbadb::do_import(const list<string>& fnames, Reader& reader, const DBImportOptions& opts)
{
    Importer importer(db, opts);
    reader.read(fnames, importer);
    importer.commit();
    if (reader.verbose)
        fprintf(stderr, "%u messages successfully imported, %u messages skipped\n", reader.count_successes, reader.count_failures);

    // As discussed in #101, if there are both successes and failures, return
    // success only if --rejected has been used, because in that case the
    // caller can check the size of the rejected file to detect the difference
    // between a mixed result and a complete success.
    // One can use --rejected=/dev/null to ignore partial failures.
    if (!reader.count_failures)
        return 0;
    if (!reader.count_successes)
        return 1;
    return reader.has_fail_file() ? 0 : 1;
}

int Dbadb::do_import(const std::string& fname, Reader& reader, const DBImportOptions& opts)
{
    list<string> fnames;
    fnames.push_back(fname);
    return do_import(fnames, reader, opts);
}

int Dbadb::do_export(const Query& query, File& file, const char* output_template, const char* forced_repmemo)
{
    impl::ExporterOptions opts;
    if (output_template && output_template[0] != 0)
        opts.template_name = output_template;

    if (forced_repmemo)
        forced_repmemo = forced_repmemo;
    auto exporter = Exporter::create(file.encoding(), opts);

    auto cursor = db.query_messages(query);
    while (cursor->next())
    {
        auto msg = cursor->detach_message();
        /* Override the message type if the user asks for it */
        if (forced_repmemo != NULL)
        {
            impl::Message& m = impl::Message::downcast(*msg);
            m.type = impl::Message::type_from_repmemo(forced_repmemo);
            m.set_rep_memo(forced_repmemo);
        }
        std::vector<std::shared_ptr<Message>> msgs;
        msgs.emplace_back(move(msg));
        file.write(exporter->to_binary(msgs));
    }
    return 0;
}

}
}
