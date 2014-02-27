/*
 * dballe/messages/db - Archive for point-based meteorological data, message-based DB
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_MESSAGES_H
#define DBA_DB_MESSAGES_H

#include <dballe/db/mem/db.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>

namespace dballe {
struct File;

namespace db {
namespace messages {

/**
 * DB-ALLe database connection
 */
class DB : public dballe::db::mem::DB
{
protected:
    File* input;
    msg::Importer* importer;
    Msgs current_msg;
    unsigned current_msg_idx;

public:
    /**
     * Create a DB reading messages from the given file
     *
     * @param fname: see the name argument of dballe::File::create
     * @param options: options used to customize the importer
     */
    DB(Encoding type, const std::string& fname, const msg::Importer::Options& options=msg::Importer::Options());
    virtual ~DB();

    db::Format format() const { return MESSAGES; }

    /**
     * Wipe the databaes and load the next message into it.
     *
     * @return
     *   true if there was another message to import, false on end of input.
     */
    bool next_message();

    friend class dballe::DB;
};

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
