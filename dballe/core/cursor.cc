#include "cursor.h"
#include "dballe/types.h"
#include <wreport/error.h>

namespace dballe {
namespace impl {

namespace {

template<class Interface>
struct EmptyCursor : public Interface
{
    int remaining() const override { return 0; }
    bool next() override { return false; }
    void discard() override {}

    bool enqi(const char* key, unsigned len, int& res) const override { return false; }
    bool enqd(const char* key, unsigned len, double& res) const override { return false; }
    bool enqs(const char* key, unsigned len, std::string& res) const override { return false; }
    bool enqf(const char* key, unsigned len, std::string& res) const override { return false; }

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
    const Message& get_message() const override { throw wreport::error_notfound("cannot retrieve a message from an empty result set"); }
    std::unique_ptr<Message> detach_message() override { throw wreport::error_notfound("cannot retrieve a message from an empty result set"); }
};

}

std::unique_ptr<CursorStation> CursorStation::make_empty()
{
    return std::unique_ptr<CursorStation>(new EmptyCursorStation);
}

std::unique_ptr<CursorStationData> CursorStationData::make_empty()
{
    return std::unique_ptr<CursorStationData>(new EmptyCursorStationData);
}

std::unique_ptr<CursorData> CursorData::make_empty()
{
    return std::unique_ptr<CursorData>(new EmptyCursorData);
}

std::unique_ptr<CursorSummary> CursorSummary::make_empty()
{
    return std::unique_ptr<CursorSummary>(new EmptyCursorSummary);
}

std::unique_ptr<CursorMessage> CursorMessage::make_empty()
{
    return std::unique_ptr<CursorMessage>(new EmptyCursorMessage);
}

bool CursorMessage::enqi(const char* key, unsigned len, int& res) const { return false; }
bool CursorMessage::enqd(const char* key, unsigned len, double& res) const { return false; }
bool CursorMessage::enqs(const char* key, unsigned len, std::string& res) const { return false; }
bool CursorMessage::enqf(const char* key, unsigned len, std::string& res) const { return false; }


}
}
