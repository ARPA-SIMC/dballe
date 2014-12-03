/*
 * db/v6/repinfo - repinfo table management
 *
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

#ifndef DBALLE_DB_V6_REPINFO_H
#define DBALLE_DB_V6_REPINFO_H

#include <dballe/db/v5/repinfo.h>

namespace dballe {
namespace db {
namespace v6 {

class Repinfo : public v5::Repinfo
{
public:
    Repinfo(ODBCConnection* conn);

    /// Return how many time this ID is used in the database
    virtual int id_use_count(unsigned id, const char* name);
};


}
}
}

#endif
