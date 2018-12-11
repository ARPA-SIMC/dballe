#ifndef DBALLE_MSG_CURSOR_H
#define DBALLE_MSG_CURSOR_H

#include <dballe/cursor.h>
#include <dballe/types.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>

namespace dballe {
namespace impl {
namespace msg {

struct CursorStation : public dballe::CursorStation
{
    dballe::DBStation station;
    const Values& station_values;
    bool at_start = true;

    CursorStation(const impl::Message& msg)
        : station_values(msg.find_station_context())
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

    DBStation get_station() const override { return station; }

    DBValues get_values() const override
    {
        return DBValues(station_values);
    }
};


struct CursorStationData : public dballe::CursorStationData
{
    dballe::DBStation station;
    const Values& station_values;
    bool at_start = true;
    Values::const_iterator cur;

    CursorStationData(const impl::Message& msg)
        : station_values(msg.find_station_context())
    {
        station.report = msg.get_report();
        station.coords = msg.get_coords();
        station.ident = msg.get_ident();
    }

    int remaining() const override
    {
        if (at_start)
            return station_values.size();
        return station_values.end() - cur;
    }

    bool next() override
    {
        if (at_start)
        {
            at_start = false;
            cur = station_values.begin();
            return true;
        }
        else if (cur == station_values.end())
            return false;
        else
        {
            ++cur;
            return cur == station_values.end();
        }
    }

    void discard() override
    {
        at_start = false;
        cur = station_values.end();
    }

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;

    DBStation get_station() const override { return station; }

    wreport::Varcode get_varcode() const override { return (*cur)->code(); }
    wreport::Var get_var() const override { return **cur; }
};


struct CursorDataRow
{
    Level level;
    Trange trange;
    Values::const_iterator var;

    CursorDataRow(Values::const_iterator var)
        : var(var)
    {
    }

    CursorDataRow(const Level& level, const Trange& trange, Values::const_iterator var)
        : level(level), trange(trange), var(var)
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

    CursorData(const impl::Message& msg, bool merged=false)
    {
        station.report = msg.get_report();
        station.coords = msg.get_coords();
        station.ident = msg.get_ident();
        datetime = msg.get_datetime();

        for (const auto& ctx: msg.data)
            for (Values::const_iterator cur = ctx.values.begin(); cur != ctx.values.end(); ++cur)
                rows.emplace_back(ctx.level, ctx.trange, cur);

        if (merged)
            for (Values::const_iterator cur = msg.station_data.begin(); cur != msg.station_data.end(); ++cur)
                if (WR_VAR_X((*cur)->code()) < 4 || WR_VAR_X((*cur)->code()) > 6)
                    rows.emplace_back(cur);
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
        {
            return false;
        }
        else
        {
            ++cur;
            return cur != rows.end();
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

    DBStation get_station() const override { return station; }

    wreport::Varcode get_varcode() const override { return (*(cur->var))->code(); }
    wreport::Var get_var() const override { return **(cur->var); }
    Level get_level() const override { return cur->level; }
    Trange get_trange() const override { return cur->trange; }
    Datetime get_datetime() const override { return datetime; }
};



}
}
}

#endif
