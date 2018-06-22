#ifndef DBALLE_DB_V7_TRACE_H
#define DBALLE_DB_V7_TRACE_H

#include <string>

namespace dballe {
namespace db {
namespace v7 {

struct Trace
{
    virtual ~Trace() {}

    virtual void trace_select(const std::string& query, unsigned rows=0) = 0;
    virtual void trace_insert(const std::string& query, unsigned rows=0) = 0;
    virtual void trace_update(const std::string& query, unsigned rows=0) = 0;
    virtual void trace_delete(const std::string& query) = 0;
    virtual void trace_select_row(unsigned count=1) = 0;
    virtual void trace_insert_row(unsigned count=1) = 0;
    virtual void trace_update_row(unsigned count=1) = 0;

    virtual void print(FILE* out) = 0;

    virtual void reset() = 0;
};

struct ProfileTrace : public Trace
{
    unsigned profile_count_select = 0;
    unsigned profile_count_insert = 0;
    unsigned profile_count_update = 0;
    unsigned profile_count_delete = 0;
    unsigned profile_count_select_rows = 0;
    unsigned profile_count_insert_rows = 0;
    unsigned profile_count_update_rows = 0;

    void trace_select(const std::string& query, unsigned rows=0) override { ++profile_count_select; profile_count_select_rows += rows; }
    void trace_insert(const std::string& query, unsigned rows=0) override { ++profile_count_insert; profile_count_insert_rows += rows; }
    void trace_update(const std::string& query, unsigned rows=0) override { ++profile_count_update; profile_count_update_rows += rows; }
    void trace_delete(const std::string& query) override { ++profile_count_delete; }
    void trace_select_row(unsigned count=1) override { profile_count_select_rows += count; }
    void trace_insert_row(unsigned count=1) override { profile_count_insert_rows += count; }
    void trace_update_row(unsigned count=1) override { profile_count_update_rows += count; }

    void print(FILE* out) override;
    void reset() override;
};

struct QuietProfileTrace : public ProfileTrace
{
    void print(FILE* out) override;
};

}
}
}

#endif
