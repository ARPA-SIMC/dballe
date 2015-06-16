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
#ifndef DBA_MEMDB_TESTS_H
#define DBA_MEMDB_TESTS_H

#include <dballe/core/tests.h>
#include <dballe/memdb/results.h>
#include <vector>
#include <iterator>

namespace dballe {
struct Memdb;

namespace memdb {
struct Value;
}

namespace tests {

template<typename T>
static inline std::vector<const T*> _get_results(WIBBLE_TEST_LOCPRM, memdb::Results<T>& res)
{
    using namespace wibble::tests;

    wassert(actual(res.is_select_all()).isfalse());
    wassert(actual(res.is_empty()).isfalse());
    std::vector<const T*> items;
    res.copy_valptrs_to(std::back_inserter(items));
    return items;
}

#define get_results(res) dballe::tests::_get_results(wibble_test_location.nest(wibble_test_location_info, __FILE__, __LINE__, "get_results(" #res ")"), res)

/*
std::vector<const memdb::Value*> _get_data_results(WIBBLE_TEST_LOCPRM, const Memdb& memdb, const Record& query);
#define get_data_results(memdb, query) _get_data_results(wibble_test_location.nest(wibble_test_location_info, __FILE__, __LINE__, "get_data_results(" #memdb ", " #query ")"), memdb, query)
*/

}
}

#endif
