#ifndef DBALLE_CORE_RECORD_H
#define DBALLE_CORE_RECORD_H

/** @file
 * @ingroup core
 * Implement a storage object for a group of related observation data
 */

#include <dballe/record.h>
#include <dballe/core/defs.h>
#include <dballe/core/fwd.h>
#include <dballe/var.h>
#include <dballe/core/matcher.h>
#include <vector>
#include <set>
#include <functional>

namespace dballe {
namespace core {

/**
 * DB-All.E record.
 *
 * A Record is a container for one observation of meteorological values, that
 * includes anagraphical informations, physical location of the observation in
 * time and space, and all the observed variables.
 */
class Record : public dballe::Record
{
public:
    int priomin = MISSING_INT;
    int priomax = MISSING_INT;
    int mobile = MISSING_INT;
    DBStation station;
    LatRange latrange;
    LonRange lonrange;
    DatetimeRange datetime;
    Level level;
    Trange trange;
    wreport::Varcode var = 0;
    std::set<wreport::Varcode> varlist;
    std::string query;
    std::string ana_filter;
    std::string data_filter;
    std::string attr_filter;
    int count = MISSING_INT;

protected:
    // The variables, sorted by varcode
    std::vector<wreport::Var*> m_vars;

    /// Find an item by wreport::Varcode, returning -1 if not found
    int find_item(wreport::Varcode code) const noexcept;

	/// Find an item by wreport::Varcode, raising an exception if not found
	wreport::Var& get_item(wreport::Varcode code);

	/// Find an item by wreport::Varcode, raising an exception if not found
	const wreport::Var& get_item(wreport::Varcode code) const;

	/// Remove an item by wreport::Varcode
	void remove_item(wreport::Varcode code);

    /**
     * Look at the value of a variable
     *
     * @return
     *   A const pointer to the internal variable, or NULL if the variable has not
     *   been found.
     */
    const wreport::Var* var_peek(wreport::Varcode code) const throw ();

    /**
     * Remove a variable from the record.
     *
     * @param code
     *   The variable to remove
     */
    void unset_var(wreport::Varcode code);

    /// Return the Var for a variable, creating it if it is missing
    wreport::Var& obtain(wreport::Varcode code);

    void foreach_key_ref(std::function<void(const char*, const wreport::Var&)> dest) const;
    void foreach_key_copy(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const;

    int _enqi(const char* key, unsigned len, bool& found) const;
    double _enqd(const char* key, unsigned len, bool& found, bool& missing) const;
    std::string _enqs(const char* key, unsigned len, bool& found, bool& missing) const;

    bool _seti(const char* key, unsigned len, int val);
    bool _setd(const char* key, unsigned len, double val);
    bool _setc(const char* key, unsigned len, const char* val);
    bool _sets(const char* key, unsigned len, const std::string& val);
    bool _setf(const char* key, unsigned len, const char* val);

    bool _unset(const char* key, unsigned len);
    bool _isset(const char* key, unsigned len, bool& res) const;

    bool equals(const Record& rec) const;

public:
    Record();
    Record(const Record& rec);
    Record(Record&& rec);
    ~Record();

    std::unique_ptr<dballe::Record> clone() const override;

    Record& operator=(const Record& rec);
    Record& operator=(Record&& rec);

    bool operator==(const dballe::Record& rec) const override;
    bool operator!=(const dballe::Record& rec) const override;

    void clear() override;
    void clear_vars() override;
    void seti(const char* key, int val) override;
    void setd(const char* key, double val) override;
    void setc(const char* key, const char* val) override;
    void sets(const char* key, const std::string& val) override;
    void setf(const char* key, const char* val) override;
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
    void unset(const char* name) override;
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
    static const Record& downcast(const dballe::Record& query);

    /**
     * Return a reference to record downcasted as core::Record.
     *
     * Throws an exception if record is not a core::Record.
     */
    static Record& downcast(dballe::Record& query);

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

    int enqi(const char* key, int def) const override;
    double enqd(const char* key, double def) const override;
    bool enqdb(const char* key, double& res) const override;
    std::string enqs(const char* key, const std::string& def) const override;
    bool enqsb(const char* key, std::string& res) const override;

    bool isset(const char* key) const;

    Coords get_coords() const override;
    Ident get_ident() const override;
    Level get_level() const override;
    Trange get_trange() const override;
    Datetime get_datetime() const override;
    DatetimeRange get_datetimerange() const override;
    Station get_station() const override;
    DBStation get_dbstation() const override;
    const wreport::Var* get_var(wreport::Varcode code) const override;

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

    /// Return the varcode-sorted vector with the variables
    const std::vector<wreport::Var*>& vars() const;

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
};


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

}
}
#endif
