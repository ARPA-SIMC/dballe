#ifndef DBALLE_CORE_DATA_H
#define DBALLE_CORE_DATA_H

#include <dballe/data.h>
#include <dballe/core/values.h>
#if 0
#include <dballe/core/defs.h>
#include <dballe/core/fwd.h>
#include <dballe/var.h>
#include <dballe/core/matcher.h>
#include <vector>
#include <set>
#include <functional>
#endif

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
    Values values;

#if 0
protected:
    /// Find an item by wreport::Varcode, returning -1 if not found
    int find_item(wreport::Varcode code) const noexcept;

	/// Find an item by wreport::Varcode, raising an exception if not found
	wreport::Var& get_item(wreport::Varcode code);

	/// Find an item by wreport::Varcode, raising an exception if not found
	const wreport::Var& get_item(wreport::Varcode code) const;

	/// Remove an item by wreport::Varcode
	void remove_item(wreport::Varcode code);

    void foreach_key_ref(std::function<void(const char*, const wreport::Var&)> dest) const;
    void foreach_key_copy(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const;

    bool equals(const Record& rec) const;
#endif

public:
    Data() = default;
    Data(const Data& rec) = default;
    Data(Data&& rec) = default;
    ~Data();

#if 0
    std::unique_ptr<dballe::Data> clone() const override;
#endif

    Data& operator=(const Data& rec) = default;
    Data& operator=(Data&& rec) = default;

    bool operator==(const dballe::Data& rec) const override;
    bool operator!=(const dballe::Data& rec) const override;

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
#if 0
    void add(const dballe::Record& source) override;
#endif
#if 0
    bool contains(const dballe::Record& subset) const override;
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
	/**
	 * Set the record to contain only those fields that change source1 into source2.
	 *
	 * If a field has been deleted from source1 to source2, it will not be copied
	 * in dest.
	 *
	 * @param source1
	 *   The original record to compute the changes from.
	 * @param source2
	 *   The new record that has changes over source1.
	 */
	void set_to_difference(const Record& source1, const Record& source2);
#endif

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

#if 0
    /**
     * Iterate all keys in the record, calling f on them.
     *
     * Iteration stops if f returns false.
     *
     * The function returns true if it reached the end of the iteration, or
     * false if it stopped because f returned false.
     */
    bool iter_keys(std::function<bool(dba_keyword, const wreport::Var&)> f) const;
#endif

#if 0
    /**
     * Set a value in the record according to an assignment encoded in a string.
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
     * Set a record from a ", "-separated string of assignments.
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

    /**
     * Copy the contents to a query
     */
    void to_query(core::Query& q) const;
#endif

#if 0
    /**
     * Generate a sequence of key names and const Var& for all the
     * contents of the record
     */
    void foreach_key(std::function<void(const char*, const wreport::Var&)> dest) const { foreach_key_ref(dest); }

    /**
     * Generate a sequence of key names and unique_ptr<Var> for all the
     * contents of the record
     */
    void foreach_key(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const { foreach_key_copy(dest); }
#endif

#if 0
	/**
	 * Return the name of a dba_keyword
	 *
	 * @return
	 *   The keyword name, or NULL if keyword is not a valid keyword
	 */
	static const char* keyword_name(dba_keyword keyword);

	/**
	 * Return informations about a keyword
	 *
	 * @return
	 *   The wreport::Varinfo structure with the informations.
	 */
	static wreport::Varinfo keyword_info(dba_keyword keyword);

	/**
	 * Get the dba_keyword corresponding to the given name
	 *
	 * @returns
	 *   The corresponding dba_keyword, or DBA_KEY_ERROR if tag does not match a
	 *   valid keyword.
	 */
	static dba_keyword keyword_byname(const char* tag);

	/**
	 * Get the dba_keyword corresponding to the given name
	 *
	 * @param tag
	 *   The name to query.
	 * @param len
	 *   The length of the name in tag.
	 * @returns
	 *   The corresponding dba_keyword, or DBA_KEY_ERROR if tag does not match a
	 *   valid keyword.
	 */
	static dba_keyword keyword_byname_len(const char* tag, int len);
#endif

#if 0
    // TODO: remove these:

    /// Return the Var for a variable, creating it if it is missing
    wreport::Var& obtain(wreport::Varcode code);
    /// Return the varcode-sorted vector with the variables
    const std::vector<wreport::Var*>& vars() const;
    /**
     * Remove a variable from the record.
     *
     * @param code
     *   The variable to remove
     */
    void unset_var(wreport::Varcode code);
#endif
};

#if 0
struct MatchedRecord : public Matched
{
    const Record& r;

    MatchedRecord(const Record& r);
    ~MatchedRecord();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_datetime(const DatetimeRange& range) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};
#endif

}
}
#endif

