/*
 * core/test-utils-core - Test utility functions for the core module
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "test-utils-core.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace tests {

static std::string tag;


/*
static void _ensureRecordHas(const char* file, int line, dba_record rec, const char* key, int val)
{
	int v;
	INNER_CHECKED(dba_enqi(rec, key, &v));
	ensure_equals(v, val);
}
static void _ensureRecordHas(const char* file, int line, dba_record rec, const char* key, double val)
{
	double v;
	INNER_CHECKED(dba_enqd(rec, key, &v));
	ensure_equals(v, val);
}
static void _ensureRecordHas(const char* file, int line, dba_record rec, const char* key, const char* val)
{
	const char* v;
	INNER_CHECKED(dba_enqc(rec, key, &v));
	gen_ensure(strcmp(v, val) == 0);
}
#define ensureRecordHas(...) _ensureRecordHas(__FILE__, __LINE__, __VA_ARGS__)
*/

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
	WR_VAR(0,  7,   1),
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
	const char* testdatadirenv = getenv("DBA_TESTDATA");
	std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
	return testdatadir + "/" + fname;
}

auto_ptr<File> _open_test_data(const wibble::tests::Location& loc, const char* filename, Encoding type)
{
	try {
		return auto_ptr<File>(File::create(type, datafile(filename), "r"));
	} catch (wreport::error& e) {
		throw tut::failure(loc.msg(e.what()));
	}
}

auto_ptr<Rawmsg> _read_rawmsg(const wibble::tests::Location& loc, const char* filename, Encoding type)
{
	try {
		auto_ptr<File> f = _open_test_data(loc, filename, type);
		auto_ptr<Rawmsg> res(new Rawmsg);

		inner_ensure(f->read(*res));

		return res;
	} catch (wreport::error& e) {
		throw tut::failure(loc.msg(e.what()));
	}
}

}
}

// vim:set ts=4 sw=4:
