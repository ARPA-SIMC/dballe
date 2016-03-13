#include "tests.h"
#include "record.h"
#include "matcher.h"
#include <wreport/utils/string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fnmatch.h>

using namespace wreport;
using namespace dballe::tests;
using namespace std;

namespace dballe {
namespace tests {

static std::string tag;

const static Varcode generator_varcodes[] = {
	WR_VAR(0,  1,   1),
	WR_VAR(0,  1,   2),
	WR_VAR(0,  1,   8),
	WR_VAR(0,  1,  11),
	WR_VAR(0,  1,  12),
	WR_VAR(0,  1,  13),
	WR_VAR(0,  2,   1),
	WR_VAR(0,  2,   2),
	WR_VAR(0,  2,   5),
	WR_VAR(0,  2,  11),
	WR_VAR(0,  2,  12),
	WR_VAR(0,  2,  61),
	WR_VAR(0,  2,  62),
	WR_VAR(0,  2,  63),
	WR_VAR(0,  2,  70),
	WR_VAR(0,  4,   1),
	WR_VAR(0,  4,   2),
	WR_VAR(0,  4,   3),
	WR_VAR(0,  4,   4),
	WR_VAR(0,  4,   5),
	WR_VAR(0,  5,   1),
	WR_VAR(0,  6,   1),
	WR_VAR(0,  7,  30),
	WR_VAR(0,  7,   2),
	WR_VAR(0,  7,  31),
	WR_VAR(0,  8,   1),
	WR_VAR(0,  8,   4),
	WR_VAR(0,  8,  21),
	WR_VAR(0, 10,   3),
	WR_VAR(0, 10,   4),
	WR_VAR(0, 10,  51),
	WR_VAR(0, 10,  61),
	WR_VAR(0, 10,  63),
	WR_VAR(0, 10, 197),
	WR_VAR(0, 11,   1),
	WR_VAR(0, 11,   2),
	WR_VAR(0, 11,   3),
	WR_VAR(0, 11,   4),
};

#if 0
generator::~generator()
{
	for (std::vector<dba_record>::iterator i = reused_pseudoana_fixed.begin();
			i != reused_pseudoana_fixed.end(); i++)
		dba_record_delete(*i);
	for (std::vector<dba_record>::iterator i = reused_pseudoana_mobile.begin();
			i != reused_pseudoana_mobile.end(); i++)
		dba_record_delete(*i);
	for (std::vector<dba_record>::iterator i = reused_context.begin();
			i != reused_context.end(); i++)
		dba_record_delete(*i);
}

dba_err generator::fill_pseudoana(dba_record rec, bool mobile)
{
	dba_record ana;
	if ((mobile && reused_pseudoana_mobile.empty()) ||
		(!mobile && reused_pseudoana_fixed.empty()) ||
		rnd(0.3))
	{
		DBA_RUN_OR_RETURN(dba_record_create(&ana));

		/* Pseudoana */
		DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_LAT, rnd(-90, 90)));
		DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_LON, rnd(-180, 180)));
		if (mobile)
		{
			DBA_RUN_OR_RETURN(dba_record_key_setc(ana, DBA_KEY_IDENT, rnd(10).c_str()));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_MOBILE, 1));
			reused_pseudoana_mobile.push_back(ana);
		} else {
			//DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_BLOCK, rnd(0, 99)));
			//DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_STATION, rnd(0, 999)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_MOBILE, 0));
			reused_pseudoana_fixed.push_back(ana);
		}
	} else {
		if (mobile)
			ana = reused_pseudoana_mobile[rnd(0, reused_pseudoana_mobile.size() - 1)];
		else
			ana = reused_pseudoana_fixed[rnd(0, reused_pseudoana_fixed.size() - 1)];
	}
	DBA_RUN_OR_RETURN(dba_record_add(rec, ana));
	return dba_error_ok();
}

dba_err generator::fill_context(dba_record rec)
{
	dba_record ctx;
	if (reused_context.empty() || rnd(0.7))
	{
		DBA_RUN_OR_RETURN(dba_record_create(&ctx));

		/* Context */
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_YEAR, rnd(2002, 2005)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_MONTH, rnd(1, 12)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_DAY, rnd(1, 28)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_HOUR, rnd(0, 23)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_MIN, rnd(0, 59)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_LEVELTYPE1, rnd(0, 300)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L1, rnd(0, 100000)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_LEVELTYPE2, rnd(0, 300)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L2, rnd(0, 100000)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_PINDICATOR, rnd(0, 300)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_P1, rnd(0, 100000)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_P2, rnd(0, 100000)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_REP_COD, rnd(105, 149)));

		reused_context.push_back(ctx);
	} else {
		ctx = reused_context[rnd(0, reused_context.size() - 1)];
	}
	DBA_RUN_OR_RETURN(dba_record_add(rec, ctx));
	return dba_error_ok();
}

dba_err generator::fill_record(dba_record rec)
{
	DBA_RUN_OR_RETURN(fill_pseudoana(rec, rnd(0.8)));
	DBA_RUN_OR_RETURN(fill_context(rec));
	DBA_RUN_OR_RETURN(dba_record_var_setc(rec, generator_varcodes[rnd(0, sizeof(generator_varcodes) / sizeof(dba_varcode))], "1"));
	return dba_error_ok();
}

#endif

#if 0
dba_err read_file(dba_encoding type, const std::string& name, dba_raw_consumer& cons)
{
	dba_err err = DBA_OK;
	dba_file file = open_test_data(name.c_str(), type);
	dba_rawmsg raw = 0;
	int found;

	DBA_RUN_OR_GOTO(cleanup, dba_rawmsg_create(&raw));

	DBA_RUN_OR_GOTO(cleanup, dba_file_read(file, raw, &found));
	while (found)
	{
		DBA_RUN_OR_GOTO(cleanup, cons.consume(raw));
		DBA_RUN_OR_GOTO(cleanup, dba_file_read(file, raw, &found));
	}

cleanup:
	if (file) dba_file_delete(file);
	if (raw) dba_rawmsg_delete(raw);
	return err == DBA_OK ? dba_error_ok() : err;
}
#endif

std::string datafile(const std::string& fname)
{
    // Skip appending the test data path for pathnames starting with ./
    if (fname[0] == '.') return fname;
	const char* testdatadirenv = getenv("DBA_TESTDATA");
	std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
	return testdatadir + "/" + fname;
}

unique_ptr<File> open_test_data(const char* filename, File::Encoding type)
{
    return unique_ptr<File>(File::create(type, datafile(filename), "r"));
}

BinaryMessage read_rawmsg(const char* filename, File::Encoding type)
{
    unique_ptr<File> f = wcallchecked(open_test_data(filename, type));
    BinaryMessage res = f->read();
    wassert(actual(res).istrue());
    return res;
}

#if 0
    void vars_equal(const Record& expected) { return TestRecordVarsEqual(this->actual, expected); }
void TestRecordValEqual::check() const
{
    const wreport::Var* evar = expected.get(name);
    const wreport::Var* avar = actual.get(name);

    if (with_missing_int && evar && evar->enq(MISSING_INT) == MISSING_INT)
        evar = NULL;
    if (with_missing_int && avar && avar->enq(MISSING_INT) == MISSING_INT)
        avar = NULL;

    if (!evar && !avar) return;
    if (!evar || !avar || evar->code() != avar->code() || !evar->value_equals(*avar))
    {
        std::stringstream ss;
        ss << "records differ on " << name << ": ";
        if (!evar)
            ss << "it is expected unset";
        else
            ss << evar->format() << " is expected";
        ss << ", but actual ";
        if (!avar)
            ss << "is unset";
        else
            ss << "has " << avar->format();
        throw TestFailed(ss.str());
    }
}
#endif

void ActualRecord::vars_equal(const Values& expected) const
{
    WREPORT_TEST_INFO(locinfo);
    const auto& act = core::Record::downcast(_actual);
    locinfo() << "Expected: " /* << expected.to_string() */ << " actual: " << act.to_string();

    const vector<Var*>& vars1 = act.vars();
    vector<Var*> vars2;
    for (const auto& i : expected)
        vars2.push_back(i.second.var);

    if (vars1.size() != vars2.size())
    {
        std::stringstream ss;
        ss << "records have a different number of variables. Expected is " << vars2.size() << " and actual has " << vars1.size();
        throw TestFailed(ss.str());
    }

    for (unsigned i = 0; i < vars1.size(); ++i)
    {
        if (*vars1[i] != *vars2[i])
        {
            std::stringstream ss;
            ss << "records variables differ at position " << i << ": expected is "
               << varcode_format(vars2[i]->code()) << "=" << vars2[i]->format("")
               << " and actual has "
               << varcode_format(vars1[i]->code()) << "=" << vars1[i]->format("");
            throw TestFailed(ss.str());
        }
    }
}

void set_record_from_string(Record& rec, const std::string& s)
{
    auto& r = core::Record::downcast(rec);
    str::Split split(s, ", ");
    for (const auto& i: split)
        r.set_from_string(i.c_str());
}

unique_ptr<Record> record_from_string(const std::string& s)
{
    auto res = Record::create();
    set_record_from_string(*res, s);
    return move(res);
}

unique_ptr<Query> query_from_string(const std::string& s)
{
    core::Query* q;
    unique_ptr<Query> res(q = new core::Query);
    q->set_from_test_string(s);
    return res;
}

core::Query core_query_from_string(const std::string& s)
{
    core::Query q;
    q.set_from_test_string(s);
    return q;
}

void ActualMatcherResult::operator==(int expected) const
{
    if (expected == _actual) return;
    std::stringstream ss;
    ss << "actual match result is " << matcher::result_format((matcher::Result)_actual) << " but it should be " << matcher::result_format((matcher::Result)expected);
    throw TestFailed(ss.str());
}

void ActualMatcherResult::operator!=(int expected) const
{
    if (expected != _actual) return;
    std::stringstream ss;
    ss << "actual match result is " << matcher::result_format((matcher::Result)_actual) << " but it should not be";
    throw TestFailed(ss.str());
}

}
}
