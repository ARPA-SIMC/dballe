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

#include "tests.h"
#include "memdb.h"

using namespace dballe::memdb;
using namespace wibble;
using namespace std;

namespace dballe {
namespace tests {

/*
std::vector<const Value*> _get_data_results(WIBBLE_TEST_LOCPRM, const Memdb& memdb, const Record& query)
{
    using namespace wibble::tests;

    Results<Value> res(memdb.values);
    memdb.query_data(query, res);
    std::vector<const Value*> items;
    res.copy_valptrs_to(std::back_inserter(items));
    return items;
}
*/

}
}

//#include "query.tcc"
