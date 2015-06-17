#include "query.h"
#include "core/query.h"

using namespace std;

namespace dballe {

std::unique_ptr<Query> Query::create()
{
    return unique_ptr<Query>(new core::Query);
}

std::unique_ptr<Query> Query::from_record(const Record& rec)
{
    unique_ptr<Query> res(new core::Query);
    res->set_from_record(rec);
    return res;
}

}
