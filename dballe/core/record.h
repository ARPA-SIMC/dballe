#ifndef DBALLE_CORE_RECORD_H
#define DBALLE_CORE_RECORD_H

/** @file
 * @ingroup core
 * Implement a storage object for a group of related observation data
 */

#include <dballe/record.h>
#include <dballe/core/defs.h>
#include <dballe/core/var.h>
#include <dballe/core/matcher.h>
#include <vector>

namespace dballe {
namespace core {

/**
 * Keyword used to quickly access context and query information from a record.
 */
enum _dba_keyword {
	DBA_KEY_ERROR		= -1,
	DBA_KEY_PRIORITY	=  0,
	DBA_KEY_PRIOMAX		=  1,
	DBA_KEY_PRIOMIN		=  2,
	DBA_KEY_REP_MEMO	=  3,
	DBA_KEY_ANA_ID		=  4,
	DBA_KEY_MOBILE		=  5,
	DBA_KEY_IDENT		=  6,
	DBA_KEY_LAT			=  7,
	DBA_KEY_LON			=  8,
	DBA_KEY_LATMAX		=  9,
	DBA_KEY_LATMIN		= 10,
	DBA_KEY_LONMAX		= 11,
	DBA_KEY_LONMIN		= 12,
	DBA_KEY_YEAR		= 13,
	DBA_KEY_MONTH		= 14,
	DBA_KEY_DAY			= 15,
	DBA_KEY_HOUR		= 16,
	DBA_KEY_MIN			= 17,
	DBA_KEY_SEC			= 18,
	DBA_KEY_YEARMAX		= 19,
	DBA_KEY_YEARMIN		= 20,
	DBA_KEY_MONTHMAX	= 21,
	DBA_KEY_MONTHMIN	= 22,
	DBA_KEY_DAYMAX		= 23,
	DBA_KEY_DAYMIN		= 24,
	DBA_KEY_HOURMAX		= 25,
	DBA_KEY_HOURMIN		= 26,
	DBA_KEY_MINUMAX		= 27,
	DBA_KEY_MINUMIN		= 28,
	DBA_KEY_SECMAX		= 29,
	DBA_KEY_SECMIN		= 30,
	DBA_KEY_LEVELTYPE1	= 31,
	DBA_KEY_L1			= 32,
	DBA_KEY_LEVELTYPE2	= 33,
	DBA_KEY_L2			= 34,
	DBA_KEY_PINDICATOR	= 35,
	DBA_KEY_P1			= 36,
	DBA_KEY_P2			= 37,
	DBA_KEY_VAR			= 38,
	DBA_KEY_VARLIST		= 39,
	DBA_KEY_CONTEXT_ID	= 40,
	DBA_KEY_QUERY		= 41,
	DBA_KEY_ANA_FILTER	= 42,
	DBA_KEY_DATA_FILTER	= 43,
	DBA_KEY_ATTR_FILTER	= 44,
	DBA_KEY_LIMIT		= 45,
	DBA_KEY_VAR_RELATED	= 46,
	DBA_KEY_COUNT		= 47,
};
/** @copydoc ::_dba_keyword */
typedef enum _dba_keyword dba_keyword;

std::ostream& operator<<(std::ostream& o, dba_keyword k);

/** DB-All.E record.
 *
 * A Record is a container for one observation of meteorological values, that
 * includes anagraphical informations, physical location of the observation in
 * time and space, and all the observed variables.
 */
class Record : public dballe::Record
{
protected:
	/* The storage for the core keyword data */
	wreport::Var* keydata[DBA_KEY_COUNT];

    // The variables, sorted by varcode
    std::vector<wreport::Var*> m_vars;

	/// Find an item by wreport::Varcode, returning -1 if not found
	int find_item(wreport::Varcode code) const throw ();

	/// Find an item by wreport::Varcode, raising an exception if not found
	wreport::Var& get_item(wreport::Varcode code);

	/// Find an item by wreport::Varcode, raising an exception if not found
	const wreport::Var& get_item(wreport::Varcode code) const;

	/// Remove an item by wreport::Varcode
	void remove_item(wreport::Varcode code);

    /// Return true if the given parameter is set in this record
    bool contains(dba_keyword parameter) const throw();

    /// Return true if the given variable is set in this record
    bool contains(wreport::Varcode parameter) const throw();

    /**
     * Look at the value of a parameter
     *
     * @return
     *   A const pointer to the internal variable, or NULL if the variable has not
     *   been found.
     */
    const wreport::Var* key_peek(dba_keyword parameter) const throw ();

    /**
     * Look at the value of a variable
     *
     * @return
     *   A const pointer to the internal variable, or NULL if the variable has not
     *   been found.
     */
    const wreport::Var* var_peek(wreport::Varcode code) const throw ();

    /**
     * Remove a parameter from the record.
     *
     * @param parameter
     *   The parameter to remove.
     */
    void key_unset(dba_keyword parameter);

    /**
     * Remove a parameter from the record.
     *
     * @param code
     *   The variable to remove.  See @ref vartable.h
     */
    void var_unset(wreport::Varcode code);

    /// Return the Var for a key, creating it if it is missing
    wreport::Var& obtain(const char* key);

    /// Return the Var for a key, creating it if it is missing
    wreport::Var& obtain(dba_keyword key);

    /// Return the Var for a variable, creating it if it is missing
    wreport::Var& obtain(wreport::Varcode code);

public:
	Record();
	Record(const Record& rec);
	~Record();

    std::unique_ptr<dballe::Record> clone() const override;

	Record& operator=(const Record& rec);

	bool operator==(const Record& rec) const;
	bool operator!=(const Record& rec) const { return !operator==(rec); }

	/// Remove all data from the record
	void clear();

	/// Remove all variables from the record, leaving the keywords intact
	void clear_vars();

    void seti(const char* key, int val) override;
    void setd(const char* key, double val) override;
    void setc(const char* key, const char* val) override;
    void sets(const char* key, const std::string& val) override;
    void setf(const char* key, const char* val) override;
    void set_datetime(const Datetime& dt) override;
    void set_level(const Level& lev) override;
    void set_trange(const Trange& tr) override;
    void set_var(const wreport::Var& var) override;
    void set_var_acquire(std::unique_ptr<wreport::Var>&& var) override;
    void unset(const char* name) override;
    const wreport::Var* get(const char* key) const override;
    void add(const dballe::Record& source) override;
    bool contains(const dballe::Record& subset) const override;
    void print(FILE* out) const override;

    /**
     * Return a reference to query downcasted as core::Query.
     *
     * Throws an exception if query is not a core::Query.
     */
    static const Record& downcast(const dballe::Record& query);

    /**
     * Return a reference to query downcasted as core::Query.
     *
     * Throws an exception if query is not a core::Query.
     */
    static Record& downcast(dballe::Record& query);

    /// Return true if some level attribute is set in this record
    bool contains_level() const throw();

    /// Return true if some time range attribute is set in this record
    bool contains_trange() const throw();

    /// Return true if some datetime information is set in this record
    bool contains_datetime() const throw();

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

    Level get_level() const;
    Trange get_trange() const;
    Datetime get_datetime() const;
    Datetime get_datetimemin() const;
    Datetime get_datetimemax() const;
    void setmin(const Datetime& dt);
    void setmax(const Datetime& dt);
    void unset_datetime();
    void unset_datetimemin();
    void unset_datetimemax();
    void set_coords(const Coords& c);
    void set_latrange(const LatRange& lr);
    void set_lonrange(const LonRange& lr);

	/**
	 * Set the date, level and timerange values to match the anagraphical context.
	 */
	void set_ana_context();

    /// Check if this record is set to the ana context
    bool is_ana_context() const;

    /**
     * Iterate all keys in the record, calling f on them.
     *
     * Iteration stops if f returns false.
     *
     * The function returns true if it reached the end of the iteration, or
     * false if it stopped because f returned false.
     */
    bool iter_keys(std::function<bool(dba_keyword, const wreport::Var&)> f) const;

    /// Return the varcode-sorted vector with the variables
    const std::vector<wreport::Var*>& vars() const;

	/**
	 * Parse the date extremes set in Record.
	 *
	 * This function will examine the values yearmin, monthmin, daymin, hourmin,
	 * minumin, secmin, yearmax, monthmax, daymax, hourmax, minumax, secmax, year,
	 * month, day, hour, min and sec, and will compute the two datetime extremes
	 * that bound the interval they represent.
	 *
	 * @retval minvalues
	 *   An array of 6 integers that will be filled with the minimum year, month,
	 *   day, hour, minute and seconds.
	 * @retval maxvalues
	 *   An array of 6 integers that will be filled with the maximum year, month,
	 *   day, hour, minute and seconds.
	 */
	void parse_date_extremes(int* minvalues, int* maxvalues) const;

    // Same as parse_date_extremes(int*, int*) but it fills Datetime objects
    void parse_date_extremes(Datetime& dtmin, Datetime& dtmax) const;

    /**
     * Look at the raw value of a keyword in the record, without raising errors.
     *
     * @param parameter
     *   The keyword to get the value for.
     * @return
     *   The raw string value, or NULL if the keyword has no value.
     */
    const char* key_peek_value(dba_keyword parameter) const throw ();

	/**
	 * Set a value in the record according to an assignment encoded in a string.
	 *
	 * String can use keywords, aliases and varcodes.  Examples: ana_id=3,
	 * name=Bologna, B12012=32.4
	 *
     * In case of numeric parameter, a hyphen ("-") means MISSING_INT (e.g.,
     * `leveltype2=-`).
     *
	 * @param rec
	 *   The record where the value is to be set.
	 * @param str
	 *   The string containing the assignment.
	 * @return
	 *   The error indicator for the function.
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
};

//typedef Record Query;

#if 0
/**
 * Print the difference between two records to an output stream.
 * If there is no difference, it does not print anything.
 *
 * @param rec1
 *   The first record to compare
 * @param rec2
 *   The second record to compare
 * @retval diffs
 *   Incremented by 1 if the variables differ
 * @param out
 *   The output stream to use for printing
 */
void dba_record_diff(dba_record rec1, dba_record rec2, int* diffs, FILE* out);
#endif

struct MatchedRecord : public Matched
{
    const Record& r;

    MatchedRecord(const Record& r);
    ~MatchedRecord();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_date(const Datetime& min, const Datetime& max) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};

}
}
#endif
