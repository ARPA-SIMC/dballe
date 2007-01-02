#ifndef DBALLE_CPP_DB_H
#define DBALLE_CPP_DB_H

#include <dballe/db/db.h>
#include <dballe/core/file.h>

#include <dballe++/record.h>
#include <vector>

namespace dballe {

/** Iterate through the results of a database query */
class Cursor
{
	dba_db_cursor m_cur;

public:
	/// auto_ptr-like copy semantics
	Cursor(const Cursor& cur)
	{
		m_cur = cur.m_cur;
		Cursor* nc_cur = const_cast<Cursor*>(&cur);
		nc_cur->m_cur = 0;
	}
	/// auto_ptr-like copy semantics
	Cursor& operator=(const Cursor& cur);

	/// Wraps an existing dba_db_cursor, taking charge of memory allocation
	Cursor(dba_db_cursor cur) : m_cur(cur) {}
	~Cursor();

	/// Get the number of remaining results to be fetched
	int remaining() const;

	/// Get the varcode of the last data fetched, when applicable
	dba_varcode varcode() const;

	/// Get the context id of the last data fetched, when applicable
	int contextID() const;

	/**
	 * Fetch the next result into a record.
	 *
	 * @returns false when there is no more data to read.
	 */
	bool next(Record& rec);
};


/** Wrap a dba_db */
class DB
{
	dba_db m_db;

private:
	// Forbid copying
	DB(const DB& db);
	DB& operator=(const DB& var);

public:
	/// Wraps an existing dba_db, taking charge of memory allocation
	DB(dba_db db) : m_db(db) {}

	/// Create a dba_db connecting to a database with the given parameters
	DB(const std::string& dsn, const std::string& user, const std::string& password)
	{
		checked(dba_db_create(dsn.c_str(), user.c_str(), password.c_str(), &m_db));
	}
	~DB()
	{
		dba_db_delete(m_db);
	}

	/**
	 * Wipe the database and reinitialize it, taking the initial repinfo
	 * values from the given file, or from the default repinfo.csv
	 */
	void reset(const std::string& repinfo_file = std::string())
	{
		if (repinfo_file == std::string())
			checked(dba_db_reset(m_db, NULL));
		else
			checked(dba_db_reset(m_db, repinfo_file.c_str()));
	}

	/**
	 * Query pseudoana information, retrieving the extra pseudoana info for
	 * every station
	 */
	Cursor queryAna(Record& query)
	{
		int count;
		dba_db_cursor cur;
		checked(dba_db_ana_query(m_db, query.rec(), &cur, &count));
		return Cursor(cur);
	}

	/**
	 * Query data values
	 */
	Cursor query(Record& query)
	{
		int count;
		dba_db_cursor cur;
		checked(dba_db_query(m_db, query.rec(), &cur, &count));
		return Cursor(cur);
	}

	/**
	 * Query pseudoana information, without retrieving the extra pseudoana
	 * info.
	 */
	Cursor queryAnaSummary(Record& query);

	/// Query the list of levels present in the database
	Cursor queryLevels(Record& query);

	/// Query the list of time ranges for which there is data in the
	/// database
	Cursor queryTimeRanges(Record& query);

	/// Query the list of levels and time ranges for which there is data in
	/// the database
	Cursor queryLevelsAndTimeRanges(Record& query);

	/// Query the list of variable types present in the database
	Cursor queryVariableTypes(Record& query);

	/// Query the list of movable station identifiers present in the
	/// database
	Cursor queryIdents(Record& query);

	/// Query the report information for which there is data in the
	/// database
	Cursor queryReports(Record& query);

	/// Query the list of date and times for which there is data in the
	/// database
	Cursor queryDateTimes(Record& query);

	/// Query the attributes for the given variable in the given context
	int attrQuery(int context, dba_varcode var, Record& res)
	{
		int count;
		checked(dba_db_qc_query(m_db, context, var, NULL, 0, res.rec(), &count));
		return count;
	}

	/// Query the attributes for the variable currently referenced by the
	/// given cursor
	int attrQueryCurrent(const Cursor& cur, Record& res)
	{
		return attrQuery(cur.contextID(), cur.varcode(), res);
	}

	/**
	 * Insert values from a record into the database
	 *
	 * @param rec
	 *   The record with the values to insert.
	 * @param canReplace
	 *   True if existing values can be replaced.
	 * @param addToPseudoana
	 *   True if a nonexisting pseudoana entry can be created.
	 * @retval anaid
	 *   If a pseudoana entry has been created, this is its database ID.
	 * @return
	 *   The context id of the values that have been inserted.
	 */
	int insert(Record& rec, bool canReplace, bool addToPseudoana, int *anaid = NULL)
	{
		int context;
		checked(dba_db_insert(m_db, rec.rec(), canReplace, addToPseudoana,
			anaid, &context));
		return context;
	}

	void attrInsert(int context, dba_varcode var, Record& data)
	{
		checked(dba_db_qc_insert(m_db, context, var, data.rec()));
	}
	void attrInsertCurrent(const Cursor& cur, Record& data)
	{
		attrInsert(cur.contextID(), cur.varcode(), data);
	}
	void attrInsertNew(int context, dba_varcode var, Record& data)
	{
		checked(dba_db_qc_insert_new(m_db, context, var, data.rec()));
	}
	void attrInsertNewCurrent(const Cursor& cur, Record& data)
	{
		attrInsertNew(cur.contextID(), cur.varcode(), data);
	}

	/// Remove from the database all the values that match the given query
	void remove(Record& query)
	{
		checked(dba_db_remove(m_db, query.rec()));
	}

	/// Remove all the pseudoana and context entries which have no data
	void removeOrphans()
	{
		checked(dba_db_remove_orphans(m_db));
	}

	void attrRemove(int context, dba_varcode var, dba_varcode attr)
	{
		dba_varcode codes[1];
		codes[0] = attr;
		checked(dba_db_qc_remove(m_db, context, var, codes, 1));
	}
	void attrRemoveAll(int context, dba_varcode var)
	{
		checked(dba_db_qc_remove(m_db, context, var, NULL, 0));
	}
	void attrRemoveList(int context, dba_varcode var, const std::vector<dba_varcode>& attrs)
	{
		dba_varcode codes[attrs.size()];
		std::copy(attrs.begin(), attrs.end(), &codes[0]);
		checked(dba_db_qc_remove(m_db, context, var, codes, attrs.size()));
	}
	/*
	void attrRemoveCurrent(const Cursor& cur, const std::vector<dba_varcode>& attrs = std::vector<dba_varcode>())
	{
		attrRemove(cur.contextID(), cur.varcode(), attrs);
	}
	*/

	/// Export the results of a query to a file, using the given encoding
	void exportResults(Record& query, dba_encoding encoding, const std::string& file);

	/// Export the results of a query to a file, using the given encoding
	/// and a generic BUFR and CREX template
	void exportResultsAsGeneric(Record& query, dba_encoding encoding, const std::string& file);

	/// Access the underlying dba_db
	const dba_db db() const
	{
		return m_db;
	}
	/// Access the underlying dba_db
	dba_db db()
	{
		return m_db;
	}
};

}

#endif
