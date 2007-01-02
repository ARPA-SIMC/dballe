#include <dballe++/db.h>

using namespace std;

#include <dballe/db/test-utils-db.h>

#include <sys/types.h>
#include <pwd.h>

#include <map>
#include <set>

namespace tut {
using namespace tut_dballe;

struct db_shar {
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

// vim:set ts=4 sw=4:
