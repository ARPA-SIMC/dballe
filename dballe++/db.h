#ifndef DBALLE_CPP_DB_H
#define DBALLE_CPP_DB_H

#include <dballe/db/db.h>
#include <dballe/core/file.h>

#include <dballe++/record.h>
#include <vector>

namespace dballe {

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

	DB(const std::string& dsn, const std::string& user, const std::string& password)
	{
		checked(dba_db_create(dsn.c_str(), user.c_str(), password.c_str(), &m_db));
	}
	~DB()
	{
		dba_db_delete(m_db);
	}

	void reset(const std::string& repinfo_file = std::string())
	{
		if (repinfo_file == std::string())
			checked(dba_db_reset(m_db, NULL));
		else
			checked(dba_db_reset(m_db, repinfo_file.c_str()));
	}

	Cursor queryAna(Record& query)
	{
		int count;
		dba_db_cursor cur;
		checked(dba_db_ana_query(m_db, query.rec(), &cur, &count));
		return Cursor(cur);
	}

	Cursor query(Record& query)
	{
		int count;
		dba_db_cursor cur;
		checked(dba_db_query(m_db, query.rec(), &cur, &count));
		return Cursor(cur);
	}

	Cursor queryAnaSummary(Record& query);
	Cursor queryLevels(Record& query);
	Cursor queryTimeRanges(Record& query);
	Cursor queryLevelsAndTimeRanges(Record& query);
	Cursor queryVariableTypes(Record& query);
	Cursor queryIdents(Record& query);
	Cursor queryReports(Record& query);
	Cursor queryDateTimes(Record& query);

	int attrQuery(int context, dba_varcode var, Record& res)
	{
		int count;
		checked(dba_db_qc_query(m_db, context, var, NULL, 0, res.rec(), &count));
		return count;
	}

	int attrQueryCurrent(const Cursor& cur, Record& res)
	{
		return attrQuery(cur.contextID(), cur.varcode(), res);
	}

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

	void remove(Record& query)
	{
		checked(dba_db_remove(m_db, query.rec()));
	}

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

	void exportResults(Record& query, dba_encoding encoding, const std::string& file);

	void exportResultsAsGeneric(Record& query, dba_encoding encoding, const std::string& file);

	const dba_db db() const
	{
		return m_db;
	}
	dba_db db()
	{
		return m_db;
	}
};

}

#endif
