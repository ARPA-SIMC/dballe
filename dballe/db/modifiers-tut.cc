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

#include "db/test-utils-db.h"
#include "modifiers.h"

using namespace dballe;
using namespace dballe::db;
using namespace wibble::tests;
using namespace wreport;
using namespace std;

namespace tut {

struct db_modifiers_shar
{
    db_modifiers_shar()
    {
    }

    ~db_modifiers_shar()
    {
    }
};
TESTGRP(db_modifiers);

template<> template<>
void to::test<1>()
{
    Record rec;
    rec.set("query", "best");
    wassert(actual(db::parse_modifiers(rec)) == DBA_DB_MODIFIER_BEST);
}

}
