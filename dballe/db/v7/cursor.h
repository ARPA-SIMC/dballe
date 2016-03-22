#ifndef DBA_DB_V7_CURSOR_H
#define DBA_DB_V7_CURSOR_H

#include <dballe/db/db.h>
#include <memory>

namespace dballe {
namespace core {
struct Query;
}

namespace db {
namespace v7 {
namespace cursor {

std::unique_ptr<CursorStation> run_station_query(DB& db, const core::Query& query, bool explain);
std::unique_ptr<CursorStationData> run_station_data_query(DB& db, const core::Query& query, bool explain);
std::unique_ptr<CursorData> run_data_query(DB& db, const core::Query& query, bool explain);
std::unique_ptr<CursorSummary> run_summary_query(DB& db, const core::Query& query, bool explain);
void run_delete_query(DB& db, const core::Query& query, bool station_vars, bool explain);

}
}
}
}
#endif
