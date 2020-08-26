#ifndef DBALLE_CORE_CURSOR_H
#define DBALLE_CORE_CURSOR_H

#include <dballe/cursor.h>
#include <dballe/core/enq.h>
#include <memory>

namespace dballe {
namespace impl {

/// Cursor iterating over stations
struct CursorStation : public dballe::CursorStation
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a shared_ptr pointer
    inline static std::shared_ptr<CursorStation> downcast(std::shared_ptr<dballe::CursorStation> c)
    {
        auto res = std::dynamic_pointer_cast<CursorStation>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }

    /// Create a CursorStation iterating on no results
    static std::shared_ptr<CursorStation> make_empty();
};

/// Cursor iterating over station data values
struct CursorStationData : public dballe::CursorStationData
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a shared_ptr pointer
    inline static std::shared_ptr<CursorStationData> downcast(std::shared_ptr<dballe::CursorStationData> c)
    {
        auto res = std::dynamic_pointer_cast<CursorStationData>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }

    /// Create a CursorStationData iterating on no results
    static std::shared_ptr<CursorStationData> make_empty();
};

/// Cursor iterating over data values
struct CursorData : public dballe::CursorData
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a shared_ptr pointer
    inline static std::shared_ptr<CursorData> downcast(std::shared_ptr<dballe::CursorData> c)
    {
        auto res = std::dynamic_pointer_cast<CursorData>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }

    /// Create a CursorData iterating on no results
    static std::shared_ptr<CursorData> make_empty();
};

/// Cursor iterating over summary entries
struct CursorSummary : public dballe::CursorSummary
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a shared_ptr pointer
    inline static std::shared_ptr<CursorSummary> downcast(std::shared_ptr<dballe::CursorSummary> c)
    {
        auto res = std::dynamic_pointer_cast<CursorSummary>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }

    /// Create a CursorSummary iterating on no results
    static std::shared_ptr<CursorSummary> make_empty();
};

/// Cursor iterating over messages
struct CursorMessage : public dballe::CursorMessage
{
    virtual void enq(Enq& enq) const {}

    /// Downcast a shared_ptr pointer
    inline static std::shared_ptr<CursorMessage> downcast(std::shared_ptr<dballe::CursorMessage> c)
    {
        auto res = std::dynamic_pointer_cast<CursorMessage>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }

    /// Create a CursorStation iterating on no results
    static std::shared_ptr<CursorMessage> make_empty();
};

}
}
#endif
