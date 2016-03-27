#include "transaction.h"
#include <cassert>

namespace dballe {
namespace db {
namespace v7 {

Transaction& Transaction::downcast(dballe::Transaction& transaction)
{
    v7::Transaction* t = dynamic_cast<v7::Transaction*>(&transaction);
    assert(t);
    return *t;
}

}
}
}
