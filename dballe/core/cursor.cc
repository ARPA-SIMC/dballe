#include "cursor.h"
#include "dballe/types.h"
#include <wreport/error.h>

namespace dballe {
namespace impl {

namespace {

template<class Interface>
struct EmptyCursor : public Interface
{
    bool has_value() const { return false; }
    void enq(Enq& enq) const {}
    int remaining() const override { return 0; }
    bool next() override { return false; }
    void discard() override {}

    DBStation get_station() const override { return DBStation(); }
};

struct EmptyCursorStation : public EmptyCursor<impl::CursorStation>
{
    DBValues get_values() const override { return DBValues(); }
};

struct EmptyCursorStationData : public EmptyCursor<impl::CursorStationData>
{
    wreport::Varcode get_varcode() const override { return 0; }
    wreport::Var get_var() const { throw wreport::error_consistency("cursor not on a result"); }
};

struct EmptyCursorData : public EmptyCursor<impl::CursorData>
{
    wreport::Varcode get_varcode() const override { return 0; }
    wreport::Var get_var() const { throw wreport::error_consistency("cursor not on a result"); }
    Level get_level() const override { return Level(); }
    Trange get_trange() const override { return Trange(); }
    Datetime get_datetime() const override { return Datetime(); }
};

struct EmptyCursorSummary : public EmptyCursor<impl::CursorSummary>
{
    Level get_level() const override { return Level(); }
    Trange get_trange() const override { return Trange(); }
    wreport::Varcode get_varcode() const override { return 0; }
    DatetimeRange get_datetimerange() const override { return DatetimeRange(); }
    size_t get_count() const override { return 0; }
};

struct EmptyCursorMessage : public EmptyCursor<impl::CursorMessage>
{
    std::shared_ptr<Message> get_message() const override { throw wreport::error_notfound("cannot retrieve a message from an empty result set"); }
};

}

std::shared_ptr<CursorStation> CursorStation::make_empty()
{
    return std::make_shared<EmptyCursorStation>();
}

std::shared_ptr<CursorStationData> CursorStationData::make_empty()
{
    return std::make_shared<EmptyCursorStationData>();
}

std::shared_ptr<CursorData> CursorData::make_empty()
{
    return std::make_shared<EmptyCursorData>();
}

std::shared_ptr<CursorSummary> CursorSummary::make_empty()
{
    return std::make_shared<EmptyCursorSummary>();
}

std::shared_ptr<CursorMessage> CursorMessage::make_empty()
{
    return std::make_shared<EmptyCursorMessage>();
}

}
}
