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

#include "db.h"
#include "dballe/core/file.h"

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace messages {

DB::DB(Encoding type, const std::string& fname, const msg::Importer::Options& options)
    : input(0), importer(0), current_msg_idx(0)
{
    input = File::create(type, fname, "rb").release();
    importer = msg::Importer::create(type, options).release();
}

DB::~DB()
{
    if (input) delete input;
    if (importer) delete importer;
}

bool DB::next_message()
{
    // Clear existing data
    // Do not use reset to preserve repinfo information
    memdb.clear();

    if (current_msg_idx >= current_msg.size())
    {
        // Read data
        Rawmsg rmsg;
        if (!input->read(rmsg))
            return false;

        // Parse and interpret data
        current_msg.clear();
        importer->from_rawmsg(rmsg, current_msg);

        // Move to the first message
        current_msg_idx = 0;
    }

    import_msg(*current_msg[current_msg_idx], NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_OVERWRITE);

    ++current_msg_idx;

    return true;
}

}
}
}
