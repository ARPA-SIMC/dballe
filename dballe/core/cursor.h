#ifndef DBALLE_CORE_CURSOR_H
#define DBALLE_CORE_CURSOR_H

#include <dballe/cursor.h>
#include <memory>

namespace dballe {
namespace impl {

/// Cursor iterating over stations
struct CursorStation : public dballe::CursorStation
{
    /// Query the content of the cursor, as an int. Returns false if the value is unset
    virtual bool enqi(const char* key, unsigned len, int& res) const = 0;

    /// Query the content of the cursor, as a double. Returns false if the value is unset
    virtual bool enqd(const char* key, unsigned len, double& res) const = 0;

    /// Query the content of the cursor, as a string. Returns false if the value is unset
    virtual bool enqs(const char* key, unsigned len, std::string& res) const = 0;

    /// Query the content of the cursor, as a formatted string. Returns false if the value is unset
    virtual bool enqf(const char* key, unsigned len, std::string& res) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorStation> downcast(std::unique_ptr<dballe::CursorStation> c)
    {
        return std::unique_ptr<CursorStation>(dynamic_cast<CursorStation*>(c.release()));
    }
};

/// Cursor iterating over station data values
struct CursorStationData : public dballe::CursorStationData
{
    /// Query the content of the cursor, as an int. Returns false if the value is unset
    virtual bool enqi(const char* key, unsigned len, int& res) const = 0;

    /// Query the content of the cursor, as a double. Returns false if the value is unset
    virtual bool enqd(const char* key, unsigned len, double& res) const = 0;

    /// Query the content of the cursor, as a string. Returns false if the value is unset
    virtual bool enqs(const char* key, unsigned len, std::string& res) const = 0;

    /// Query the content of the cursor, as a formatted string. Returns false if the value is unset
    virtual bool enqf(const char* key, unsigned len, std::string& res) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorStationData> downcast(std::unique_ptr<dballe::CursorStationData> c)
    {
        return std::unique_ptr<CursorStationData>(dynamic_cast<CursorStationData*>(c.release()));
    }
};

/// Cursor iterating over data values
struct CursorData : public dballe::CursorData
{
    /// Query the content of the cursor, as an int. Returns false if the value is unset
    virtual bool enqi(const char* key, unsigned len, int& res) const = 0;

    /// Query the content of the cursor, as a double. Returns false if the value is unset
    virtual bool enqd(const char* key, unsigned len, double& res) const = 0;

    /// Query the content of the cursor, as a string. Returns false if the value is unset
    virtual bool enqs(const char* key, unsigned len, std::string& res) const = 0;

    /// Query the content of the cursor, as a formatted string. Returns false if the value is unset
    virtual bool enqf(const char* key, unsigned len, std::string& res) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorData> downcast(std::unique_ptr<dballe::CursorData> c)
    {
        return std::unique_ptr<CursorData>(dynamic_cast<CursorData*>(c.release()));
    }
};

/// Cursor iterating over summary entries
struct CursorSummary : public dballe::CursorSummary
{
    /// Query the content of the cursor, as an int. Returns false if the value is unset
    virtual bool enqi(const char* key, unsigned len, int& res) const = 0;

    /// Query the content of the cursor, as a double. Returns false if the value is unset
    virtual bool enqd(const char* key, unsigned len, double& res) const = 0;

    /// Query the content of the cursor, as a string. Returns false if the value is unset
    virtual bool enqs(const char* key, unsigned len, std::string& res) const = 0;

    /// Query the content of the cursor, as a formatted string. Returns false if the value is unset
    virtual bool enqf(const char* key, unsigned len, std::string& res) const = 0;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorSummary> downcast(std::unique_ptr<dballe::CursorSummary> c)
    {
        return std::unique_ptr<CursorSummary>(dynamic_cast<CursorSummary*>(c.release()));
    }
};

/// Cursor iterating over messages
struct CursorMessage : public dballe::CursorMessage
{
    /// Query the content of the cursor, as an int. Returns false if the value is unset
    virtual bool enqi(const char* key, unsigned len, int& res) const;

    /// Query the content of the cursor, as a double. Returns false if the value is unset
    virtual bool enqd(const char* key, unsigned len, double& res) const;

    /// Query the content of the cursor, as a string. Returns false if the value is unset
    virtual bool enqs(const char* key, unsigned len, std::string& res) const;

    /// Query the content of the cursor, as a formatted string. Returns false if the value is unset
    virtual bool enqf(const char* key, unsigned len, std::string& res) const;

    /// Downcast a unique_ptr pointer
    inline static std::unique_ptr<CursorMessage> downcast(std::unique_ptr<dballe::CursorMessage> c)
    {
        return std::unique_ptr<CursorMessage>(dynamic_cast<CursorMessage*>(c.release()));
    }
};

}
}
#endif
