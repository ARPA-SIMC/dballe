#ifndef DBALLE_CURSOR_H
#define DBALLE_CURSOR_H

#include <dballe/fwd.h>
#include <dballe/record.h>
#include <wreport/var.h>
#include <memory>
#include <vector>

namespace dballe {

/**
 * Base class for cursors that iterate over DB query results
 */
struct Cursor
{
    virtual ~Cursor();

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    virtual int remaining() const = 0;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    virtual bool next() = 0;

    /// Discard the results that have not been read yet
    virtual void discard() = 0;

    /// Query the content of the cursor, as an int. Returns false if the value is unset
    virtual bool enqi(const char* key, unsigned len, int& res) const = 0;

    /// Query the content of the cursor, as a double. Returns false if the value is unset
    virtual bool enqd(const char* key, unsigned len, double& res) const = 0;

    /// Query the content of the cursor, as a string. Returns false if the value is unset
    virtual bool enqs(const char* key, unsigned len, std::string& res) const = 0;

    /// Query the content of the cursor, as a formatted string. Returns false if the value is unset
    virtual bool enqf(const char* key, unsigned len, std::string& res) const = 0;

    /**
     * Fill in a Data structure with the current contents of the cursor
     *
     * @param data
     *   The Data where to store the values
     */
    virtual void to_record(Record& record) = 0;

    /**
     * Get the whole station data in a single call
     */
    virtual DBStation get_station() const = 0;
};

/// Cursor iterating over stations
struct CursorStation : public Cursor
{
    /// Create a CursorStation iterating on no results
    static std::unique_ptr<CursorStation> make_empty();
};

/// Cursor iterating over station data values
struct CursorStationData : public Cursor
{
    /// Get the variable code
    virtual wreport::Varcode get_varcode() const = 0;

    /// Get the variable
    virtual wreport::Var get_var() const = 0;

    /// Create a CursorStationData iterating on no results
    static std::unique_ptr<CursorStationData> make_empty();
};

/// Cursor iterating over data values
struct CursorData : public Cursor
{
    /// Get the variable code
    virtual wreport::Varcode get_varcode() const = 0;

    /// Get the variable
    virtual wreport::Var get_var() const = 0;

    /// Get the level
    virtual Level get_level() const = 0;

    /// Get the time range
    virtual Trange get_trange() const = 0;

    /// Get the datetime
    virtual Datetime get_datetime() const = 0;

    /// Create a CursorData iterating on no results
    static std::unique_ptr<CursorData> make_empty();
};

/// Cursor iterating over summary entries
struct CursorSummary : public Cursor
{
    /// Get the level
    virtual Level get_level() const = 0;

    /// Get the time range
    virtual Trange get_trange() const = 0;

    /// Get the variable code
    virtual wreport::Varcode get_varcode() const = 0;

    /// Get the datetime range
    virtual DatetimeRange get_datetimerange() const = 0;

    /// Get the count of elements
    virtual size_t get_count() const = 0;

    /// Create a CursorSummary iterating on no results
    static std::unique_ptr<CursorSummary> make_empty();
};

}
#endif
