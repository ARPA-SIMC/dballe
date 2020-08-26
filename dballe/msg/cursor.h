#ifndef DBALLE_MSG_CURSOR_H
#define DBALLE_MSG_CURSOR_H

#include <dballe/core/cursor.h>
#include <dballe/types.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>

namespace dballe {
namespace impl {
namespace msg {

struct CursorStation : public impl::CursorStation
{
    std::shared_ptr<const impl::Message> msg;
    dballe::DBStation station;
    const Values& station_values;
    bool at_start = true;

    CursorStation(std::shared_ptr<const impl::Message> msg)
        : msg(msg), station_values(msg->find_station_context())
    {
        station.report = msg->get_report();
        station.coords = msg->get_coords();
        station.ident = msg->get_ident();
    }
    ~CursorStation();

    bool has_value() const override
    {
        return !at_start;
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

    void enq(Enq& enq) const override;

    DBStation get_station() const override { return station; }

    DBValues get_values() const override
    {
        return DBValues(station_values);
    }

    /// Downcast a unique_ptr pointer
    inline static std::shared_ptr<CursorStation> downcast(std::shared_ptr<dballe::CursorStation> c)
    {
        auto res = std::dynamic_pointer_cast<CursorStation>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }
};


struct CursorStationData : public impl::CursorStationData
{
    std::shared_ptr<const impl::Message> msg;
    dballe::DBStation station;
    const Values& station_values;
    bool at_start = true;
    Values::const_iterator cur;

    CursorStationData(std::shared_ptr<const impl::Message> msg)
        : msg(msg), station_values(msg->find_station_context())
    {
        station.report = msg->get_report();
        station.coords = msg->get_coords();
        station.ident = msg->get_ident();
    }
    ~CursorStationData();

    bool has_value() const override
    {
        return !at_start && cur != station_values.end();
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
            return cur != station_values.end();
        }
        else if (cur == station_values.end())
            return false;
        else
        {
            ++cur;
            return cur != station_values.end();
        }
    }

    void discard() override
    {
        at_start = false;
        cur = station_values.end();
    }

    void enq(Enq& enq) const override;

    DBStation get_station() const override { return station; }

    wreport::Varcode get_varcode() const override { return (*cur)->code(); }
    wreport::Var get_var() const override { return **cur; }

    /// Downcast a unique_ptr pointer
    inline static std::shared_ptr<CursorStationData> downcast(std::shared_ptr<dballe::CursorStationData> c)
    {
        auto res = std::dynamic_pointer_cast<CursorStationData>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }
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

struct CursorData : public impl::CursorData
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
    ~CursorData();

    bool has_value() const override
    {
        return !at_start && cur != rows.end();
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
            return cur != rows.end();
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

    void enq(Enq& enq) const override;

    DBStation get_station() const override { return station; }

    wreport::Varcode get_varcode() const override { return (*(cur->var))->code(); }
    wreport::Var get_var() const override { return **(cur->var); }
    Level get_level() const override { return cur->level; }
    Trange get_trange() const override { return cur->trange; }
    Datetime get_datetime() const override { return datetime; }

    /// Downcast a unique_ptr pointer
    inline static std::shared_ptr<CursorData> downcast(std::shared_ptr<dballe::CursorData> c)
    {
        auto res = std::dynamic_pointer_cast<CursorData>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }
};


}
}
}

#endif
