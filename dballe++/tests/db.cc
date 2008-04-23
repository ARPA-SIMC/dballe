#include <dballe++/db.h>
#include <dballe++/init.h>

using namespace std;

#include <dballe/db/test-utils-db.h>

#include <sys/types.h>
#include <pwd.h>

#include <map>
#include <set>

namespace tut {
using namespace tut_dballe;
using namespace dballe;

std::string getuname()
{
	struct passwd *pwd = getpwuid(getuid());
	const char* uname = pwd == NULL ? "test" : pwd->pw_name;
	return uname;
}

struct db_shar {
	DballeInit dballeInit;

	DB db;
	Record query;
	Record result;
	int context;

	db_shar() : db("test", getuname().c_str(), "") {
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
		data.keySet(DBA_KEY_LEVELTYPE1, 10);
		data.keySet(DBA_KEY_L1, 11);
		data.keySet(DBA_KEY_LEVELTYPE2, 15);
		data.keySet(DBA_KEY_L2, 22);
		data.keySet(DBA_KEY_PINDICATOR, 20);
		data.keySet(DBA_KEY_P1, 111);
		data.keySet(DBA_KEY_P2, 222);
		data.keySet(DBA_KEY_REP_COD, 1);
		data.varSet(DBA_VAR(0, 1, 11), "Hey Hey!!");
		data.varSet(DBA_VAR(0, 1, 12), 500);

		context = db.insert(data, false, true);

		data.clear();
		data.varSet(DBA_VAR(0, 33, 7), 50);
		data.varSet(DBA_VAR(0, 33, 36), 75);
		db.attrInsert(context, DBA_VAR(0, 1, 11), data);
	}
};

TESTGRP( db );

template<> template<>
void to::test<1>()
{
	Cursor cur = db.queryAna(query);
	gen_ensure_equals(cur.remaining(), 1);
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(cur.remaining(), 0);
	gen_ensure_equals(result.enqd(DBA_KEY_LAT), 12.34560);
	gen_ensure_equals(result.enqd(DBA_KEY_LON), 76.54320);
	gen_ensure_equals(result.contains(DBA_VAR(0, 1, 11)), false);
}
	
template<> template<>
void to::test<2>()
{
	map<dba_varcode, string> expected;
	expected[DBA_VAR(0, 1, 11)] = "Hey Hey!!";
	expected[DBA_VAR(0, 1, 12)] = "500";

	query.keySet(DBA_KEY_LATMIN, 10.0);
	Cursor cur = db.query(query);
	gen_ensure_equals(cur.remaining(), 2);
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(cur.remaining(), 1);
	gen_ensure(expected.find(cur.varcode()) != expected.end());
	gen_ensure_equals(result.enqs(cur.varcode()), expected[cur.varcode()]);
	expected.erase(cur.varcode());
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(cur.remaining(), 0);
	gen_ensure(expected.find(cur.varcode()) != expected.end());
	gen_ensure_equals(result.enqs(cur.varcode()), expected[cur.varcode()]);
	gen_ensure_equals(cur.next(result), false);
}

template<> template<>
void to::test<3>()
{
	Record data;
	int count = db.attrQuery(context, DBA_VAR(0, 1, 11), data);
	gen_ensure_equals(count, 2);

	vector<dba_varcode> vars;
	vars.push_back(DBA_VAR(0, 33, 36));
	count = db.attrQuery(context, DBA_VAR(0, 1, 11), vars, data);
	gen_ensure_equals(count, 1);
}

template<> template<>
void to::test<4>()
{
	query.clear();
	Cursor cur = db.queryLevels(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.enqi(DBA_KEY_LEVELTYPE1), 10);
	gen_ensure_equals(result.enqi(DBA_KEY_L1), 11);
	gen_ensure_equals(result.enqi(DBA_KEY_LEVELTYPE2), 15);
	gen_ensure_equals(result.enqi(DBA_KEY_L2), 22);
	gen_ensure_equals(cur.next(result), false);
}

template<> template<>
void to::test<5>()
{
	query.clear();
	Cursor cur = db.queryTimeRanges(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.enqi(DBA_KEY_PINDICATOR), 20);
	gen_ensure_equals(result.enqi(DBA_KEY_P1), 111);
	gen_ensure_equals(result.enqi(DBA_KEY_P2), 222);
	gen_ensure_equals(cur.next(result), false);
}

template<> template<>
void to::test<6>()
{
	query.clear();
	Cursor cur = db.queryLevelsAndTimeRanges(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.enqi(DBA_KEY_LEVELTYPE1), 10);
	gen_ensure_equals(result.enqi(DBA_KEY_L1), 11);
	gen_ensure_equals(result.enqi(DBA_KEY_LEVELTYPE2), 15);
	gen_ensure_equals(result.enqi(DBA_KEY_L2), 22);
	gen_ensure_equals(result.enqi(DBA_KEY_PINDICATOR), 20);
	gen_ensure_equals(result.enqi(DBA_KEY_P1), 111);
	gen_ensure_equals(result.enqi(DBA_KEY_P2), 222);
	gen_ensure_equals(cur.next(result), false);
}

template<> template<>
void to::test<7>()
{
	query.clear();
	Cursor cur = db.queryVariableTypes(query);
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
}

template<> template<>
void to::test<8>()
{
	query.clear();
	Cursor cur = db.queryIdents(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.contains(DBA_KEY_IDENT), false);
	gen_ensure_equals(cur.next(result), false);
}

template<> template<>
void to::test<9>()
{
	query.clear();
	Cursor cur = db.queryReports(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.enqi(DBA_KEY_REP_COD), 1);
	gen_ensure_equals(result.enqc(DBA_KEY_REP_MEMO), string("synop"));
	gen_ensure_equals(cur.next(result), false);
}

template<> template<>
void to::test<10>()
{
	query.clear();
	Cursor cur = db.queryDateTimes(query);
	gen_ensure_equals(cur.remaining(), 1);
	result.clear();
	gen_ensure_equals(cur.next(result), true);
	gen_ensure_equals(result.enqi(DBA_KEY_YEAR), 1945);
	gen_ensure_equals(result.enqi(DBA_KEY_MONTH), 4);
	gen_ensure_equals(result.enqi(DBA_KEY_DAY), 25);
	gen_ensure_equals(result.enqi(DBA_KEY_HOUR), 8);
	gen_ensure_equals(result.enqi(DBA_KEY_MIN), 0);
	gen_ensure_equals(result.enqi(DBA_KEY_SEC), 0);
	gen_ensure_equals(cur.next(result), false);
}

}

// vim:set ts=4 sw=4:
