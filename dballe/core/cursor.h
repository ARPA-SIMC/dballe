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

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorStation> downcast(std::unique_ptr<dballe::CursorStation> c)
    {
        return std::unique_ptr<CursorStation>(dynamic_cast<CursorStation*>(c.release()));
    }

    /// Create a CursorStation iterating on no results
    static std::unique_ptr<CursorStation> make_empty();
};

/// Cursor iterating over station data values
struct CursorStationData : public dballe::CursorStationData
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorStationData> downcast(std::unique_ptr<dballe::CursorStationData> c)
    {
        return std::unique_ptr<CursorStationData>(dynamic_cast<CursorStationData*>(c.release()));
    }

    /// Create a CursorStationData iterating on no results
    static std::unique_ptr<CursorStationData> make_empty();
};

/// Cursor iterating over data values
struct CursorData : public dballe::CursorData
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorData> downcast(std::unique_ptr<dballe::CursorData> c)
    {
        return std::unique_ptr<CursorData>(dynamic_cast<CursorData*>(c.release()));
    }

    /// Create a CursorData iterating on no results
    static std::unique_ptr<CursorData> make_empty();
};

/// Cursor iterating over summary entries
struct CursorSummary : public dballe::CursorSummary
{
    virtual void enq(Enq& enq) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorSummary> downcast(std::unique_ptr<dballe::CursorSummary> c)
    {
        return std::unique_ptr<CursorSummary>(dynamic_cast<CursorSummary*>(c.release()));
    }

    /// Create a CursorSummary iterating on no results
    static std::unique_ptr<CursorSummary> make_empty();
};

/// Cursor iterating over messages
struct CursorMessage : public dballe::CursorMessage
{
    virtual void enq(Enq& enq) const {}

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorMessage> downcast(std::unique_ptr<dballe::CursorMessage> c)
    {
        return std::unique_ptr<CursorMessage>(dynamic_cast<CursorMessage*>(c.release()));
    }

    /// Create a CursorStation iterating on no results
    static std::unique_ptr<CursorMessage> make_empty();
};

}
}
#endif
