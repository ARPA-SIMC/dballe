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
#include "db/mem/db.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace std;

namespace tut {

struct db_mem_shar : public dballe::tests::DB_test_base
{
    db_mem_shar() : dballe::tests::DB_test_base(db::MEM)
    {
    }

    ~db_mem_shar()
    {
    }
};
TESTGRP(db_mem);

template<> template<>
void to::test<1>()
{
}

}

/* vim:set ts=4 sw=4: */
