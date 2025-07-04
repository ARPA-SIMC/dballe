#ifndef DBALLE_DB_V7_FWD_H
#define DBALLE_DB_V7_FWD_H

namespace dballe {
namespace db {
namespace v7 {

struct Transaction;
struct QueryBuilder;
struct StationQueryBuilder;
struct DataQueryBuilder;
struct SummaryQueryBuilder;
struct IdQueryBuilder;
struct DB;
struct Repinfo;
struct Station;
struct LevTr;
struct LevTrEntry;
struct SQLTrace;
struct Driver;

namespace cursor {
struct Stations;
struct StationData;
struct Data;
struct Summary;
} // namespace cursor

namespace batch {
struct Station;
struct StationDatum;
struct MeasuredDatum;
} // namespace batch

namespace trace {
struct Step;
struct Transaction;
} // namespace trace

/**
 * Smart pointer for trace::Step objects, which calls done() when going out of
 * scope
 */
template <typename Step = trace::Step> class Tracer
{
protected:
    Step* step;

public:
    Tracer() : step(nullptr) {}
    Tracer(Step* step) : step(step) {}
    Tracer(const Tracer&) = delete;
    Tracer(Tracer&& o) : step(o.step) { o.step = nullptr; }
    Tracer& operator=(const Tracer&) = delete;
    Tracer& operator=(Tracer&&)      = delete;
    ~Tracer()
    {
        if (step)
            step->done();
    }
    void reset(Step* step) { this->step = step; }
    void done()
    {
        if (step)
            step->done();
        step = nullptr;
    }
    Step* operator->() { return step; }
    operator bool() const { return step; }
};

} // namespace v7
} // namespace db
} // namespace dballe

#endif
