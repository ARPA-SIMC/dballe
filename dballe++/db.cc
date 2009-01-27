#include <config.h>

#include <dballe++/db.h>

#include <dballe/db/cursor.h>
#include <dballe/db/internals.h>
#include <dballe/db/export.h>
#include <dballe/msg/file.h>

#include <stdlib.h>

namespace dballe {

Cursor::~Cursor()
{
	if (m_cur)
		dba_db_cursor_delete(m_cur);
}

Cursor& Cursor::operator=(const Cursor& cur)
{
	// Correctly handle the self-assignment cur = cur
	if (this == &cur)
		return *this;

	if (m_cur)
		dba_db_cursor_delete(m_cur);
	m_cur = cur.m_cur;
	Cursor* nc_cur = const_cast<Cursor*>(&cur);
	nc_cur->m_cur = 0;
	return *this;
}

int Cursor::remaining() const
{
	return m_cur->count;
}

dba_varcode Cursor::varcode() const
{
	return m_cur->out_idvar;
}

int Cursor::contextID() const
{
	return m_cur->out_context_id;
}

bool Cursor::next(Record& rec)
{
	int has_data;
	checked(dba_db_cursor_next(m_cur, &has_data));
	if (has_data)
	{
		dba_record_clear(rec.rec());
		checked(dba_db_cursor_to_record(m_cur, rec.rec()));
		return true;
	} else
		return false;
}

int Cursor::attributes(Record& res)
{
    int count;
    checked(dba_db_qc_query(m_cur->db, m_cur->out_context_id, m_cur->out_idvar, NULL, 0, res.rec(), &count));
    return count;
}

int Cursor::attributes(const std::vector<dba_varcode>& wanted, Record& res)
{
    int count;
#ifdef HAVE_VECTOR_DATA
    checked(dba_db_qc_query(m_cur->db, m_cur->out_context_id, m_cur->out_idvar, wanted.data(), wanted.size(), res.rec(), &count));
#else
	// Work-around for when ::data() is not available on vector
	dba_varcode buf[wanted.size()];
	for (size_t i = 0; i < wanted.size(); ++i)
		buf[i] = wanted[i];
    checked(dba_db_qc_query(m_cur->db, m_cur->out_context_id, m_cur->out_idvar, buf, wanted.size(), res.rec(), &count));
#endif
    return count;
}

Cursor DB::queryAnaSummary(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_ANA_ID | DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

Cursor DB::queryLevels(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_LEVEL,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

Cursor DB::queryTimeRanges(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_TIMERANGE,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

Cursor DB::queryLevelsAndTimeRanges(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_LEVEL | DBA_DB_WANT_TIMERANGE,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

Cursor DB::queryVariableTypes(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_VAR_NAME,
				DBA_DB_MODIFIER_DISTINCT | DBA_DB_MODIFIER_NOANAEXTRA ));

	return Cursor(cur);
}

Cursor DB::queryIdents(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_IDENT,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

Cursor DB::queryReports(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_REPCOD,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

Cursor DB::queryDateTimes(const Record& query)
{
	dba_db_cursor cur;

	/* Allocate a new cursor */
	checked(dba_db_cursor_create(m_db, &cur));

	/* Perform the query, limited to level values */
	checked(dba_db_cursor_query(cur, query.rec(),
				DBA_DB_WANT_DATETIME,
				DBA_DB_MODIFIER_DISTINCT));

	return Cursor(cur);
}

int DB::attrQuery(int context, dba_varcode var, const std::vector<dba_varcode>& wanted, Record& res) const
{
	int count;
#ifdef HAVE_VECTOR_DATA
	checked(dba_db_qc_query(m_db, context, var, wanted.data(), wanted.size(), res.rec(), &count));
#else
	// Work-around for when ::data() is not available on vector
	dba_varcode buf[wanted.size()];
	for (size_t i = 0; i < wanted.size(); ++i)
		buf[i] = wanted[i];
	checked(dba_db_qc_query(m_db, context, var, buf, wanted.size(), res.rec(), &count));
#endif
	return count;
}

static dba_err msg_writer(dba_msgs msgs, void* data)
{
	dba_file file = (dba_file)data;
	return dba_file_write_msgs(file, msgs, 0, 0, 0);
}

static dba_err msg_generic_writer(dba_msgs msgs, void* data)
{
	dba_file file = (dba_file)data;
	for (int i = 0; i < msgs->len; ++i)
		msgs->msgs[i]->type = MSG_GENERIC;
	return dba_file_write_msgs(file, msgs, 0, 0, 0);
}

void DB::exportResults(Record& query, dba_encoding encoding, const std::string& file)
{
	dba_file out = NULL;
	checked(dba_file_create(encoding, file.c_str(), "wb", &out));
	try {
		checked(dba_db_export(m_db, query.rec(), msg_writer, out));
	} catch (...) {
		// Cleanup in case of mess
		dba_file_delete(out);
		throw;
	}
	dba_file_delete(out);
}

void DB::exportResultsAsGeneric(Record& query, dba_encoding encoding, const std::string& file)
{
	dba_file out = NULL;
	checked(dba_file_create(encoding, file.c_str(), "wb", &out));
	try {
		checked(dba_db_export(m_db, query.rec(), msg_generic_writer, out));
	} catch (...) {
		// Cleanup in case of mess
		dba_file_delete(out);
		throw;
	}
	dba_file_delete(out);
}

static dba_db test_db()
{
	dba_db db = NULL;
	const char* dsn = getenv("DBA_TEST_DSN");
	const char* user = getenv("DBA_TEST_USER");
	const char* pass = getenv("DBA_TEST_PASS");
	if (dsn == NULL || dsn[0] == 0) return db;
	if (user == NULL) user = "";
	if (pass == NULL) pass = "";

	checked(dba_db_create(dsn, user, pass, &db));
	return db;
}

TestDB::TestDB() : DB(test_db())
{
}

}

// vim:set ts=4 sw=4:
