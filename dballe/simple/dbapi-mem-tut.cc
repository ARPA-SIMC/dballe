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

#include "dbapi-tut.h"
#include "dballe/db/db.h"

namespace tut {

struct dbapi_mem_shar : public dballe::tests::dbapi_tests
{
    dbapi_mem_shar() : dballe::tests::dbapi_tests(dballe::db::MEM) {}

    void test_vars() { throw wreport::error_unimplemented("TODO"); }
    void test_attrs() { throw wreport::error_unimplemented("TODO"); }
    void test_attrs_prendilo() { throw wreport::error_unimplemented("TODO"); }
    void test_prendilo_anaid() { throw wreport::error_unimplemented("TODO"); }
};
TESTGRP(dbapi_mem);

#define TUT_TEST_BODY
#include "dbapi-tut.cc"

}
