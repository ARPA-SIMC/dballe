#include "db.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "cursor.h"
#include "dballe/core/query.h"
#include "dballe/core/values.h"
#include "dballe/types.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits.h>
#include <unistd.h>

using namespace std;
using namespace wreport;
using dballe::sql::Connection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {

// First part of initialising a dba_db
DB::DB(unique_ptr<Connection> conn)
    : conn(conn.release()),
      m_driver(v7::Driver::create(*this->conn).release())
{
    if (getenv("DBA_EXPLAIN") != NULL)
        explain_queries = true;

    if (const char* logdir = getenv("DBA_PROFILE"))
        trace = new CollectTrace(logdir);
    else if (Trace::in_test_suite())
        trace = new QuietCollectTrace;
    else
        trace = new NullTrace;

    auto trc = trace->trace_connect(this->conn->get_url());

    /* Set the connection timeout */
    /* SQLSetConnectAttr(pc.od_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0); */
}

DB::~DB()
{
    trace->save();
    delete m_driver;
    delete conn;
    delete trace;
}

v7::Driver& DB::driver()
{
    return *m_driver;
}

std::shared_ptr<dballe::Transaction> DB::transaction(bool readonly)
{
    auto res = conn->transaction(readonly);
    return make_shared<v7::Transaction>(dynamic_pointer_cast<v7::DB>(shared_from_this()), move(res));
}

std::shared_ptr<dballe::db::Transaction> DB::test_transaction(bool readonly)
{
    auto res = conn->transaction(readonly);
    return make_shared<v7::TestTransaction>(dynamic_pointer_cast<v7::DB>(shared_from_this()), move(res));
}

void DB::delete_tables()
{
    m_driver->delete_tables_v7();
}

void DB::disappear()
{
    // TODO: track open trasnsactions with weak pointers and roll them all
    // back, or raise errors if some of them have not been fired yet?
    m_driver->delete_tables_v7();
}

void DB::reset(const char* repinfo_file)
{
    auto trc = trace->trace_reset(repinfo_file);
    disappear();
    m_driver->create_tables_v7();

    // Populate the tables with values
    auto tr = dynamic_pointer_cast<db::Transaction>(transaction());
    int added, deleted, updated;
    tr->update_repinfo(repinfo_file, &added, &deleted, &updated);
    tr->commit();
}

void DB::vacuum()
{
    auto trc = trace->trace_vacuum();
    auto t = conn->transaction();
    driver().vacuum_v7();
    t->commit();
}

}
}
}
