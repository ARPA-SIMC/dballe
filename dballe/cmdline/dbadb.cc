/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "dbadb.h"
#include <dballe/msg/msgs.h>
#include <dballe/db/db.h>

#include <cstdlib>

using namespace wreport;
using namespace std;

// extern int op_verbose;

namespace dballe {
namespace cmdline {

namespace dbadb {

const char* parse_op_report(DB& db, const char* name)
{
    if (name != 0 && name[0] != 0)
    {
        const char* s;
        int is_cod = 1;
        for (s = name; *s && is_cod; s++)
            if (!isdigit(*s))
                is_cod = 0;

        if (is_cod)
            return db.rep_memo_from_cod(strtoul(name, NULL, 0)).c_str();
        else
            return name;
    } else
        return NULL;
}

}

namespace {

struct Importer : public Action
{
    DB& db;
    int import_flags;
    const char* forced_repmemo;

    Importer(DB& db) : db(db), import_flags(0), forced_repmemo(0) {}

    virtual bool operator()(const cmdline::Item& item);
};

bool Importer::operator()(const Item& item)
{
    if (item.msgs == NULL)
    {
        fprintf(stderr, "Message #%d cannot be parsed: ignored\n", item.idx);
        return false;
    }
    for (size_t i = 0; i < item.msgs->size(); ++i)
    {
        Msg& msg = *(*item.msgs)[i];
        if (forced_repmemo == NULL && msg.type == MSG_GENERIC)
            /* Put generic messages in the generic rep_cod by default */
            db.import_msg(msg, NULL, import_flags);
        else
            db.import_msg(msg, forced_repmemo, import_flags);
    }
    return true;
}


struct MsgWriter : public MsgConsumer
{
    File& file;
    msg::Exporter* exporter;
    const char* forced_rep_memo;

    MsgWriter(File& file) : file(file), exporter(0), forced_rep_memo(0) {}
    ~MsgWriter()
    {
        if (exporter) delete exporter;
    }

    virtual void operator()(std::auto_ptr<Msg> msg)
    {
        /* Override the message type if the user asks for it */
        if (forced_rep_memo != NULL)
        {
            msg->type = Msg::type_from_repmemo(forced_rep_memo);
            msg->set_rep_memo(forced_rep_memo);
        }
        Rawmsg raw;
        Msgs msgs;
        msgs.acquire(msg);
        exporter->to_rawmsg(msgs, raw);
        file.write(raw);
    }
};

struct MsgDumper : public MsgConsumer
{
    FILE* out;
    MsgDumper(FILE* out=stdout) : out(out) {}

    virtual void operator()(std::auto_ptr<Msg> msg)
    {
        msg->print(out);
    }
};

}

/// Query data in the database and output results as arbitrary human readable text
int Dbadb::do_dump(const Record& query, FILE* out)
{
    auto_ptr<db::Cursor> cursor = db.query_data(query);

    Record res;
    for (unsigned i = 0; cursor->next(); ++i)
    {
        cursor->to_record(res);
        fprintf(out, "#%u: -----------------------\n", i);
        res.print(out);
    }

    return 0;
}

/// Query stations in the database and output results as arbitrary human readable text
int Dbadb::do_stations(const Record& query, FILE* out)
{
    auto_ptr<db::Cursor> cursor = db.query_stations(query);

    Record res;
    for (unsigned i = 0; cursor->next(); ++i)
    {
        cursor->to_record(res);
        fprintf(out, "#%u: -----------------------\n", i);
        res.print(out);
    }

    return 0;
}

int Dbadb::do_export_dump(const Record& query, FILE* out)
{
    MsgDumper dumper(out);
    db.export_msgs(query, dumper);
    return 0;
}

int Dbadb::do_import(const list<string>& fnames, Reader& reader, int import_flags, const char* forced_repmemo)
{
    Importer importer(db);
    importer.import_flags = import_flags;
    importer.forced_repmemo = forced_repmemo;
    reader.read(fnames, importer);
    return 0;
}

int Dbadb::do_import(const std::string& fname, Reader& reader, int import_flags, const char* forced_repmemo)
{
    list<string> fnames;
    fnames.push_back(fname);
    return do_import(fnames, reader, import_flags, forced_repmemo);
}

int Dbadb::do_export(const Record& query, File& file, const char* output_template, const char* forced_repmemo)
{
    msg::Exporter::Options opts;
    if (output_template && output_template[0] != 0)
        opts.template_name = output_template;

    MsgWriter writer(file);
    if (forced_repmemo)
        writer.forced_rep_memo = dbadb::parse_op_report(db, forced_repmemo);
    writer.exporter = msg::Exporter::create(file.type(), opts).release();

    db.export_msgs(query, writer);
    return 0;
}

}
}

/* vim:set ts=4 sw=4: */