#ifndef DBALLE_CURSOR_H
#define DBALLE_CURSOR_H

#include <dballe/fwd.h>
#include <dballe/values.h>
#include <memory>
#include <vector>
#include <wreport/var.h>

namespace dballe {

/**
 * Base class for cursors that iterate over DB query results
 */
class Cursor : public std::enable_shared_from_this<Cursor>
{
public:
    virtual ~Cursor();

    /**
     * Check if the cursor points to a valid value.
     *
     * @returns true if the cursor points to a valid accessible value, false if
     *          next() has not been called yet, or if at the end of iteration
     *          (i.e. next() returned false)
     */
    virtual bool has_value() const = 0;

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
     *   true if a new record has been read, false if there is no more data to
     * read
     */
    virtual bool next() = 0;

    /// Discard the results that have not been read yet
    virtual void discard() = 0;

    /**
     * Get the whole station data in a single call
     */
    virtual DBStation get_station() const = 0;
};

/// Cursor iterating over stations
class CursorStation : public Cursor
{
public:
    /**
     * Get the station data values
     */
    virtual DBValues get_values() const = 0;
};

/// Cursor iterating over station data values
class CursorStationData : public Cursor
{
public:
    /// Get the variable code
    virtual wreport::Varcode get_varcode() const = 0;

    /// Get the variable
    virtual wreport::Var get_var() const = 0;
};

/// Cursor iterating over data values
class CursorData : public Cursor
{
public:
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
};

/// Cursor iterating over summary entries
class CursorSummary : public Cursor
{
public:
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
};

/// Cursor iterating over messages
class CursorMessage : public Cursor
{
public:
    virtual std::shared_ptr<Message> get_message() const = 0;
};

} // namespace dballe
#endif
