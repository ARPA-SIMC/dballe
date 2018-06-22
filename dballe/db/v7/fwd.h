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
struct SQLTrace;
struct Driver;

namespace batch {
struct Station;
struct StationDatum;
struct MeasuredDatum;
}

namespace trace {
struct Step;
struct Transaction;
}

/**
 * Smart pointer for trace::Step objects, which calls done() when going out of
 * scope
 */
template<typename Step=trace::Step>
class Tracer
{
protected:
    Step* step;

public:
    Tracer(Step* step) : step(step) {}
    Tracer(const Tracer&) = delete;
    Tracer(Tracer&& o)
        : step(o.step)
    {
        o.step = nullptr;
    }
    Tracer& operator=(const Tracer&) = delete;
    Tracer& operator=(Tracer&&) = delete;
    ~Tracer()
    {
        if (step) step->done();
    }

    Step* operator->() { return step; }
    operator bool() const { return step; }
};

}
}
}

#endif
