#include "query.h"
#include "core/query.h"

using namespace std;

namespace dballe {

std::unique_ptr<Query> Query::create()
{
    return unique_ptr<Query>(new core::Query);
}

}
