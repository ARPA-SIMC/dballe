#include "transaction.h"
#include "db.h"
#include "driver.h"
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

void Transaction::remove_all()
{
    auto tr = db.trace.trace_remove_all();
    db.driver().remove_all_v7();
    clear_cached_state();
    tr->done();
}

}
}
}
