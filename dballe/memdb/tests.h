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
static inline std::vector<const T*> get_results(memdb::Results<T>& res)
{
    wassert(actual(res.is_select_all()).isfalse());
    wassert(actual(res.is_empty()).isfalse());
    std::vector<const T*> items;
    res.copy_valptrs_to(std::back_inserter(items));
    return items;
}

}
}

#endif
