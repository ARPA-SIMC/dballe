#ifndef DBALLE_MSG_CURSOR_H
#define DBALLE_MSG_CURSOR_H

#include <dballe/cursor.h>
#include <dballe/types.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>

namespace dballe {
namespace msg {

struct CursorStation : public dballe::CursorStation
{
    dballe::DBStation station;
    const Context* station_ctx;
    bool at_start = true;

    CursorStation(const Msg& msg)
        : station_ctx(msg.find_station_context())
    {
        station.report = msg.get_report();
        station.coords = msg.get_coords();
        station.ident = msg.get_ident();
    }

    int remaining() const override
    {
        if (at_start)
            return 1;
        return 0;
    }

    bool next() override
    {
        if (at_start)
        {
            at_start = false;
            return true;
        }
        else
            return false;
    }

    void discard() override
    {
        at_start = false;
    }

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
    void to_record(Record& record) override {}

    DBStation get_station() const override { return station; }
};


struct CursorStationData : public dballe::CursorStationData
{
    dballe::DBStation station;
    const msg::Context* ctx;
    bool at_start = true;
    std::vector<wreport::Var*>::const_iterator cur;

    CursorStationData(const Msg& msg)
        : ctx(msg.find_station_context())
    {
        station.report = msg.get_report();
        station.coords = msg.get_coords();
        station.ident = msg.get_ident();
    }

    int remaining() const override
    {
        if (at_start)
            return ctx->data.size();
        return ctx->data.end() - cur;
    }

    bool next() override
    {
        if (at_start)
        {
            at_start = false;
            cur = ctx->data.begin();
            return true;
        }
        else if (cur == ctx->data.end())
            return false;
        else
        {
            ++cur;
            return cur == ctx->data.end();
        }
    }

    void discard() override
    {
        at_start = false;
        cur = ctx->data.end();
    }

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
    void to_record(Record& record) override {}

    DBStation get_station() const override { return station; }

    wreport::Varcode get_varcode() const override { return (*cur)->code(); }
    wreport::Var get_var() const override { return **cur; }
};


struct CursorDataRow
{
    const dballe::msg::Context* ctx;
    std::vector<wreport::Var*>::const_iterator var;

    CursorDataRow(const dballe::msg::Context* ctx, std::vector<wreport::Var*>::const_iterator var)
        : ctx(ctx), var(var)
    {
    }
};

struct CursorData : public dballe::CursorData
{
    dballe::DBStation station;
    Datetime datetime;
    std::vector<CursorDataRow> rows;
    std::vector<CursorDataRow>::const_iterator cur;
    bool at_start = true;

    CursorData(const Msg& msg, bool merged=false)
    {
        station.report = msg.get_report();
        station.coords = msg.get_coords();
        station.ident = msg.get_ident();
        datetime = msg.get_datetime();

        for (const auto& ctx: msg.data)
        {
            if (ctx->level.is_missing() && ctx->trange.is_missing())
            {
                if (!merged) continue;
                for (std::vector<wreport::Var*>::const_iterator cur = ctx->data.begin(); cur != ctx->data.end(); ++cur)
                    if (WR_VAR_X((*cur)->code()) < 4 || WR_VAR_X((*cur)->code()) > 6)
                        rows.emplace_back(ctx, cur);
            } else {
                for (std::vector<wreport::Var*>::const_iterator cur = ctx->data.begin(); cur != ctx->data.end(); ++cur)
                    rows.emplace_back(ctx, cur);
            }
        }
    }

    int remaining() const override
    {
        if (at_start)
            return rows.size();
        return rows.end() - cur;
    }

    bool next() override
    {
        if (at_start)
        {
            at_start = false;
            cur = rows.begin();
            return true;
        }
        else if (cur == rows.end())
            return false;
        else
        {
            ++cur;
            return cur == rows.end();
        }
    }

    void discard() override
    {
        at_start = false;
        cur = rows.end();
    }

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
    void to_record(Record& record) override {}

    DBStation get_station() const override { return station; }

    wreport::Varcode get_varcode() const override { return (*(cur->var))->code(); }
    wreport::Var get_var() const override { return **(cur->var); }
    Level get_level() const override { return cur->ctx->level; }
    Trange get_trange() const override { return cur->ctx->trange; }
    Datetime get_datetime() const override { return datetime; }
};



}
}

#endif
