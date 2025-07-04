#ifndef DBALLE_DB_V7_TRACE_H
#define DBALLE_DB_V7_TRACE_H

#include <dballe/core/json.h>
#include <dballe/db/v7/fwd.h>
#include <dballe/fwd.h>
#include <sstream>
#include <string>
#include <vector>

namespace dballe {
namespace db {
namespace v7 {

namespace trace {

struct Aggregate
{
    unsigned count = 0;
    unsigned rows  = 0;
    clock_t ticks  = 0;
};

/**
 * One operation being traced
 */
class Step
{
protected:
    /// Parent operation in the operation stack
    Step* parent  = nullptr;
    /// First child operation in the operation stack
    Step* child   = nullptr;
    /// Next sibling operation in the operation stack
    Step* sibling = nullptr;
    /// Operation name
    std::string name;
    /// Optional details about the operation
    std::string detail;
    /// Number of database rows affected
    unsigned rows = 0;
    /// Timing start
    clock_t start = 0;
    /// Timing end
    clock_t end   = 0;

    template <typename T> void add_sibling(T* step)
    {
        if (!sibling)
        {
            sibling      = step;
            step->parent = parent;
        }
        else
            sibling->add_sibling(step);
    }

    Step* first_sibling(const std::string& name)
    {
        if (this->name == name)
            return this;
        if (!sibling)
            return nullptr;
        return sibling->first_sibling(name);
    }

    Step* last_sibling(const std::string& name, Step* last = nullptr)
    {
        if (this->name == name)
        {
            if (!sibling)
                return this;
            return sibling->last_sibling(name, this);
        }
        if (!sibling)
            return last;
        return sibling->last_sibling(name, last);
    }

    void _aggregate(const std::string& name, Aggregate& agg)
    {
        if (this->name == name)
        {
            ++agg.count;
            agg.rows += rows;
            agg.ticks += end - start;
        }
        if (sibling)
            sibling->_aggregate(name, agg);
        if (child)
            child->_aggregate(name, agg);
    }

public:
    Step(const std::string& name);
    Step(const std::string& name, const std::string& detail);
    ~Step();

    void done();
    unsigned elapsed_usec() const;

    void to_json(core::JSONWriter& writer) const;

    // Remove all children accumulated so far
    void clear()
    {
        delete child;
        child = nullptr;
    }

    Aggregate aggregate(const std::string& name)
    {
        Aggregate res;
        if (child)
            child->_aggregate(name, res);
        return res;
    }

    Step* first_child(const std::string& name)
    {
        if (!child)
            return nullptr;
        return child->first_sibling(name);
    }

    Step* last_child(const std::string& name)
    {
        if (!child)
            return nullptr;
        return child->last_sibling(name);
    }

    void add_row(unsigned amount = 1) { rows += amount; }

    template <typename T> T* add_child(T* step)
    {
        if (!child)
        {
            child        = step;
            step->parent = this;
        }
        else
            child->add_sibling(step);
        return step;
    }

    Step* trace_select(const std::string& query, unsigned rows = 0)
    {
        Step* res = add_child(new Step("select", query));
        res->rows = rows;
        return res;
    }

    Step* trace_insert(const std::string& query, unsigned rows = 0)
    {
        Step* res = add_child(new Step("insert", query));
        res->rows = rows;
        return res;
    }

    Step* trace_update(const std::string& query, unsigned rows = 0)
    {
        Step* res = add_child(new Step("update", query));
        res->rows = rows;
        return res;
    }

    Step* trace_delete(const std::string& query, unsigned rows = 0)
    {
        Step* res = add_child(new Step("delete", query));
        res->rows = rows;
        return res;
    }
};

class Transaction : public Step
{
public:
    Transaction() : Step("transaction") {}

    Tracer<> trace_query_stations(const Query& query);
    Tracer<> trace_query_station_data(const Query& query);
    Tracer<> trace_query_data(const Query& query);
    Tracer<> trace_query_summary(const Query& query);
    Tracer<> trace_import(unsigned count);
    Tracer<> trace_export_msgs(const Query& query);
    Tracer<> trace_insert_station_data();
    Tracer<> trace_insert_data();
    Tracer<> trace_add_station_vars();
    Tracer<> trace_func(const std::string& name);
    Tracer<> trace_remove_station_data(const Query& query);
    Tracer<> trace_remove_data(const Query& query);
    Tracer<> trace_remove_station_data_by_id(int id);
    Tracer<> trace_remove_data_by_id(int id);
};

} // namespace trace

struct Trace
{
    virtual ~Trace() {}

    virtual Tracer<> trace_connect(const std::string& url)     = 0;
    virtual Tracer<> trace_reset(const char* repinfo_file = 0) = 0;
    virtual Tracer<trace::Transaction> trace_transaction()     = 0;
    virtual Tracer<> trace_remove_all()                        = 0;
    virtual Tracer<> trace_vacuum()                            = 0;
    virtual void save()                                        = 0;

    static bool in_test_suite();
    static void set_in_test_suite();
};

struct NullTrace : public Trace
{
    Tracer<> trace_connect(const std::string& url) override
    {
        return Tracer<>(nullptr);
    }
    Tracer<> trace_reset(const char* repinfo_file = 0) override
    {
        return Tracer<>(nullptr);
    }
    Tracer<trace::Transaction> trace_transaction() override
    {
        return Tracer<trace::Transaction>(nullptr);
    }
    Tracer<> trace_remove_all() override { return Tracer<>(nullptr); }
    Tracer<> trace_vacuum() override { return Tracer<>(nullptr); }
    void save() override {}
};

class QuietCollectTrace : public Trace
{
protected:
    std::vector<trace::Step*> steps;

public:
    QuietCollectTrace()                                    = default;
    QuietCollectTrace(const QuietCollectTrace&)            = delete;
    QuietCollectTrace(QuietCollectTrace&&)                 = delete;
    QuietCollectTrace& operator=(const QuietCollectTrace&) = delete;
    QuietCollectTrace& operator=(QuietCollectTrace&&)      = delete;
    ~QuietCollectTrace();

    Tracer<> trace_connect(const std::string& url) override;
    Tracer<> trace_reset(const char* repinfo_file = 0) override;
    Tracer<trace::Transaction> trace_transaction() override;
    Tracer<> trace_remove_all() override;
    Tracer<> trace_vacuum() override;

    void save() override {}
};

class CollectTrace : public QuietCollectTrace
{
protected:
    std::string logdir;
    time_t start;

public:
    CollectTrace(const std::string& logdir);

    void save() override;
};

} // namespace v7
} // namespace db
} // namespace dballe

#endif
