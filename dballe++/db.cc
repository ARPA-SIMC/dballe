#include <dballe++/db.h>

#include <dballe/db/cursor.h>
#include <dballe/db/internals.h>
#include <dballe/db/export.h>
#include <dballe/msg/file.h>

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
		checked(dba_db_cursor_to_record(m_cur, rec.rec()));
		return true;
	} else
		return false;
}

Cursor DB::queryAnaSummary(Record& query)
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

Cursor DB::queryLevels(Record& query)
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

Cursor DB::queryTimeRanges(Record& query)
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

Cursor DB::queryLevelsAndTimeRanges(Record& query)
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

Cursor DB::queryVariableTypes(Record& query)
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

Cursor DB::queryIdents(Record& query)
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

Cursor DB::queryReports(Record& query)
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

Cursor DB::queryDateTimes(Record& query)
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

static dba_err msg_writer(dba_msgs msgs, void* data)
{
	dba_file file = (dba_file)data;
	return dba_file_write_msgs(file, msgs, 0, 0);
}

static dba_err msg_generic_writer(dba_msgs msgs, void* data)
{
	dba_file file = (dba_file)data;
	for (int i = 0; i < msgs->len; ++i)
		msgs->msgs[i]->type = MSG_GENERIC;
	return dba_file_write_msgs(file, msgs, 0, 0);
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

}

using namespace std;

#ifdef DBALLEPP_COMPILE_TESTSUITE

#include <tests/test-utils.h>
#include <dballe++/init.h>

#include <sys/types.h>
#include <pwd.h>

#include <map>
#include <set>

namespace tut {

struct db_shar {
	// Automatically initialize and shutdown the library
	dballe::LibInit lib;
};

TESTGRP( db );

using namespace dballe;

template<> template<>
void to::test<1>()
{
	struct passwd *pwd = getpwuid(getuid());
	const char* uname = pwd == NULL ? "test" : pwd->pw_name;

	DB db("test", uname, "");
	db.reset();
	
	Record data;
	data.keySet(DBA_KEY_LAT, 12.34560);
	data.keySet(DBA_KEY_LON, 76.54320);
	data.keySet(DBA_KEY_MOBILE, 0);
	data.keySet(DBA_KEY_YEAR, 1945);
	data.keySet(DBA_KEY_MONTH, 4);
	data.keySet(DBA_KEY_DAY, 25);
	data.keySet(DBA_KEY_HOUR, 8);
	data.keySet(DBA_KEY_MIN, 0);
	data.keySet(DBA_KEY_LEVELTYPE, 10);
	data.keySet(DBA_KEY_L1, 11);
	data.keySet(DBA_KEY_L2, 22);
	data.keySet(DBA_KEY_PINDICATOR, 20);
	data.keySet(DBA_KEY_P1, 111);
	data.keySet(DBA_KEY_P2, 222);
	data.keySet(DBA_KEY_REP_COD, 1);
	data.varSet(DBA_VAR(0, 1, 11), "Hey Hey!!");
	data.varSet(DBA_VAR(0, 1, 12), 500);

	int context = db.insert(data, false, true);

	data.clear();
	data.varSet(DBA_VAR(0, 33, 7), 50);
	data.varSet(DBA_VAR(0, 33, 36), 75);
	db.attrInsert(context, DBA_VAR(0, 1, 11), data);

	Record query;
	Cursor cur = db.queryAna(query);
	gen_ensure_equals(cur.remaining(), 1);
	Record result;
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(cur.remaining(), 0);
	gen_ensure_equals(result.keyEnqd(DBA_KEY_LAT), 12.34560);
	gen_ensure_equals(result.keyEnqd(DBA_KEY_LON), 76.54320);
	gen_ensure_equals(result.varContains(DBA_VAR(0, 1, 11)), false);

	
	map<dba_varcode, string> expected;
	expected[DBA_VAR(0, 1, 11)] = "Hey Hey!!";
	expected[DBA_VAR(0, 1, 12)] = "500";

	query.keySet(DBA_KEY_LATMIN, 10.0);
	cur = db.query(query);
	gen_ensure_equals(cur.remaining(), 2);
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(cur.remaining(), 1);
	gen_ensure(expected.find(cur.varcode()) != expected.end());
	gen_ensure_equals(result.varEnqs(cur.varcode()), expected[cur.varcode()]);
	expected.erase(cur.varcode());
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(cur.remaining(), 0);
	gen_ensure(expected.find(cur.varcode()) != expected.end());
	gen_ensure_equals(result.varEnqs(cur.varcode()), expected[cur.varcode()]);
	gen_ensure_equals(cur.next(result), false);

	int count = db.attrQuery(context, DBA_VAR(0, 1, 11), data);
	gen_ensure_equals(count, 2);

	query.clear();
	cur = db.queryLevels(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_LEVELTYPE), 10);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_L1), 11);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_L2), 22);
	gen_ensure_equals(cur.next(result), false);

	query.clear();
	cur = db.queryTimeRanges(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_PINDICATOR), 20);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_P1), 111);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_P2), 222);
	gen_ensure_equals(cur.next(result), false);

	query.clear();
	cur = db.queryLevelsAndTimeRanges(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_LEVELTYPE), 10);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_L1), 11);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_L2), 22);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_PINDICATOR), 20);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_P1), 111);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_P2), 222);
	gen_ensure_equals(cur.next(result), false);

	query.clear();
	cur = db.queryVariableTypes(query);
	gen_ensure_equals(cur.remaining(), 2);
	result.clear();
	std::set<dba_varcode> expectedVC;
	expectedVC.insert(DBA_VAR(0, 1, 11));
	expectedVC.insert(DBA_VAR(0, 1, 12));
	gen_ensure_equals(cur.next(result), true);
	gen_ensure(expectedVC.find(cur.varcode()) != expectedVC.end());
	expectedVC.erase(cur.varcode());
	gen_ensure_equals(cur.next(result), true);
	gen_ensure(expectedVC.find(cur.varcode()) != expectedVC.end());
	gen_ensure_equals(cur.next(result), false);

	query.clear();
	cur = db.queryIdents(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.keyContains(DBA_KEY_IDENT), false);
	gen_ensure_equals(cur.next(result), false);

	query.clear();
	cur = db.queryReports(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_REP_COD), 1);
	gen_ensure_equals(result.keyEnqc(DBA_KEY_REP_MEMO), string("synop"));
	gen_ensure_equals(cur.next(result), false);

	query.clear();
	cur = db.queryDateTimes(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_YEAR), 1945);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_MONTH), 4);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_DAY), 25);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_HOUR), 8);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_MIN), 0);
	gen_ensure_equals(result.keyEnqi(DBA_KEY_SEC), 0);
	gen_ensure_equals(cur.next(result), false);
}

}

#endif

// vim:set ts=4 sw=4:
