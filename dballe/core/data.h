#ifndef DBALLE_CORE_DATA_H
#define DBALLE_CORE_DATA_H

#include <dballe/data.h>
#include <dballe/values.h>

namespace dballe {
namespace core {

/**
 * Holds data for database I/O
 *
 * Data is a container for one observation of meteorological values, that
 * includes station informations, physical location of the observation in time
 * and space, and all the observed variables.
 */
class Data : public dballe::Data
{
public:
    DBStation station;
    Datetime datetime;
    Level level;
    Trange trange;
    DBValues values;

public:
    Data()                = default;
    Data(const Data& rec) = default;
    Data(Data&& rec)      = default;
    ~Data();

    Data& operator=(const Data& rec) = default;
    Data& operator=(Data&& rec)      = default;

    bool operator==(const dballe::Data& rec) const override;
    bool operator!=(const dballe::Data& rec) const override;

    /**
     * Check the data fields for consistency, and fill in missing values:
     *
     *  - month without year, day without month, and so on, cause errors
     *  - min and max datetimes are filled with the actual minimum and maximum
     *    values acceptable for that range (year=2017, for example, becomes
     *    min=2017-01-01 00:00:00, max=2017-12-31 23:59:59
     */
    void validate();

    void clear() override;
    void clear_vars() override;
    void clear_ids() override;
#if 0
    void set_datetime(const Datetime& dt) override;
    void set_datetimerange(const DatetimeRange& range) override;
    void set_coords(const Coords& c) override;
    void set_latrange(const LatRange& lr) override;
    void set_lonrange(const LonRange& lr) override;
    void set_level(const Level& lev) override;
    void set_trange(const Trange& tr) override;
    void set_var(const wreport::Var& var) override;
    void set_var_acquire(std::unique_ptr<wreport::Var>&& var) override;
    void set_station(const Station& s) override;
    void set_dbstation(const DBStation& s) override;
#endif
    void print(FILE* out) const override;

    /**
     * Return a reference to record downcasted as core::Record.
     *
     * Throws an exception if record is not a core::Record.
     */
    static const Data& downcast(const dballe::Data& data);

    /**
     * Return a reference to record downcasted as core::Record.
     *
     * Throws an exception if record is not a core::Record.
     */
    static Data& downcast(dballe::Data& data);

#if 0
    Coords get_coords() const override;
    Ident get_ident() const override;
    Level get_level() const override;
    Trange get_trange() const override;
    Datetime get_datetime() const override;
    DatetimeRange get_datetimerange() const override;
    Station get_station() const override;
    DBStation get_dbstation() const override;
    const wreport::Var* get_var(wreport::Varcode code) const override;
#endif

    /**
     * Set a value according to an assignment encoded in a string.
     *
     * String can use keywords, aliases and varcodes.  Examples: ana_id=3,
     * name=Bologna, B12012=32.4
     *
     * In case of numeric parameter, a hyphen ("-") means MISSING_INT (e.g.,
     * `leveltype2=-`).
     *
     * @param str
     *   The string containing the assignment.
     */
    void set_from_string(const char* str);

    /**
     * Set the Data from a ", "-separated string of assignments.
     *
     * The implementation is not efficient and the method is not safe for any
     * input, since ", " could appear in a station identifier. It is however
     * useful to quickly create test queries for unit testing.
     */
    void set_from_test_string(const std::string& s);

    /**
     * Encode in a one-liner of comma-separated assignments
     */
    std::string to_string() const;

protected:
    void setf(const char* key, unsigned len, const char* val);
};

} // namespace core
} // namespace dballe
#endif
