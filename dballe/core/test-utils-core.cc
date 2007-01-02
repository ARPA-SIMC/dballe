/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/core/file_internals.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace tut_dballe {

static std::string tag;

void test_tag(const std::string& ttag)
{
	tag = ttag;
}

void test_untag()
{
	tag = string();
}

DBAException::DBAException(const char* file, int line)
{
	std::stringstream ss;
	ss << file << ":" << line << ": ";
	if (!tag.empty())
		ss << "[" << tag << "] ";
	ss << "Error " << dba_error_get_code();
	ss << " (" << dba_error_get_message() << ") while " << dba_error_get_context();

	const char* details = dba_error_get_details();
	if (details == NULL)
		ss << endl;
	else
		ss << ".  Details:" << endl << details << endl;

	m_what = ss.str();
}

std::string __ensure_errmsg(std::string file, int line, std::string msg)
{
	std::stringstream ss;
	ss << file << ":" << line << ": ";
	if (!tag.empty())
		ss << "[" << tag << "] ";
	ss << "'" << msg << "'";
	return ss.str();
}

void _ensure_var_undef(const char* file, int line, dba_var var)
{
	inner_ensure_equals(dba_var_value(var), (const char*)0);
}
void _ensure_var_equals(const char* file, int line, dba_var var, int val)
{
	int v;
	INNER_CHECKED(dba_var_enqi(var, &v));
	inner_ensure_equals(v, val);
}
void _ensure_var_equals(const char* file, int line, dba_var var, double val)
{
	double v;
	INNER_CHECKED(dba_var_enqd(var, &v));
	inner_ensure_equals(v, val);
}
void _ensure_var_equals(const char* file, int line, dba_var var, const string& val)
{
	const char* v;
	INNER_CHECKED(dba_var_enqc(var, &v));
	inner_ensure_equals(string(v), val);
}

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

const static dba_varcode generator_varcodes[] = {
	DBA_VAR(0,  1,   1),
	DBA_VAR(0,  1,   2),
	DBA_VAR(0,  1,   8),
	DBA_VAR(0,  1,  11),
	DBA_VAR(0,  1,  12),
	DBA_VAR(0,  1,  13),
	DBA_VAR(0,  2,   1),
	DBA_VAR(0,  2,   2),
	DBA_VAR(0,  2,   5),
	DBA_VAR(0,  2,  11),
	DBA_VAR(0,  2,  12),
	DBA_VAR(0,  2,  61),
	DBA_VAR(0,  2,  62),
	DBA_VAR(0,  2,  63),
	DBA_VAR(0,  2,  70),
	DBA_VAR(0,  4,   1),
	DBA_VAR(0,  4,   2),
	DBA_VAR(0,  4,   3),
	DBA_VAR(0,  4,   4),
	DBA_VAR(0,  4,   5),
	DBA_VAR(0,  5,   1),
	DBA_VAR(0,  6,   1),
	DBA_VAR(0,  7,   1),
	DBA_VAR(0,  7,   2),
	DBA_VAR(0,  7,  31),
	DBA_VAR(0,  8,   1),
	DBA_VAR(0,  8,   4),
	DBA_VAR(0,  8,  21),
	DBA_VAR(0, 10,   3),
	DBA_VAR(0, 10,   4),
	DBA_VAR(0, 10,  51),
	DBA_VAR(0, 10,  61),
	DBA_VAR(0, 10,  63),
	DBA_VAR(0, 10, 197),
	DBA_VAR(0, 11,   1),
	DBA_VAR(0, 11,   2),
	DBA_VAR(0, 11,   3),
	DBA_VAR(0, 11,   4),
};

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
			DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_BLOCK, rnd(0, 99)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_STATION, rnd(0, 999)));
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
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_LEVELTYPE, rnd(0, 300)));
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L1, rnd(0, 100000)));
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
	DBA_RUN_OR_RETURN(dba_record_var_setc(rec, generator_varcodes[rnd(0, sizeof(generator_varcodes) / sizeof(dba_varcode))], "11111"));
	return dba_error_ok();
}

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

dba_file _open_test_data(const char* file, int line, const char* filename, dba_encoding type)
{
	const char* testdatadirenv = getenv("DBA_TESTDATA");
	std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
	dba_file res;

	INNER_CHECKED(dba_file_create(type, (testdatadir + "/" + filename).c_str(), "r", &res));

	return res;
}

dba_rawmsg _read_rawmsg(const char* file, int line, const char* filename, dba_encoding type)
{
	dba_file input = _open_test_data(file, line, filename, type);
	dba_rawmsg rawmsg;
	int found;

	// Read the sample message
	INNER_CHECKED(dba_rawmsg_create(&rawmsg));
	INNER_CHECKED(dba_file_read(input, rawmsg, &found));
	inner_ensure_equals(found, 1);

	dba_file_delete(input);

	return rawmsg;
}

static dba_err slurp_file_read(dba_file file, dba_rawmsg msg, int* found)
{
	FILE* in = file->fd;

	/* Reset bufr_message data in case this message has been used before */
	dba_rawmsg_reset(msg);
	msg->offset = ftell(in);

	/* Read the entire file contents */
	while (!feof(in))
	{
		unsigned char c;
		if (fread(&c, 1, 1, in) == 1)
		{
			if (msg->len >= msg->alloclen)
				DBA_RUN_OR_RETURN(dba_rawmsg_expand_buffer(msg));
			msg->buf[msg->len++] = c;
		}
	}
	
	msg->encoding = BUFR;
	msg->file = file;
	*found = msg->len != 0;
	return dba_error_ok();
}
static void slurp_file_delete(dba_file file)
{
	free(file);
}
static dba_err slurp_file_create(dba_encoding type, FILE* fd, const char* mode, dba_file* file)
{
	*file = (dba_file)calloc(1, sizeof(struct _dba_file));
	if (*file == NULL)
		return dba_error_alloc("allocating new _dba_file");
	(*file)->fun_delete = slurp_file_delete;
	(*file)->fun_read = slurp_file_read;
	(*file)->fun_write = dba_file_default_write_impl;
	return dba_error_ok();
}

DbaFileSlurpOnly::DbaFileSlurpOnly()
{
	oldBufr = (void*)dba_file_aof_create;
	oldCrex = (void*)dba_file_bufr_create;
	oldAof = (void*)dba_file_crex_create;
	dba_file_aof_create = slurp_file_create;
	dba_file_bufr_create = slurp_file_create;
	dba_file_crex_create = slurp_file_create;
}
DbaFileSlurpOnly::~DbaFileSlurpOnly()
{
	dba_file_aof_create = (dba_file_create_fun)oldBufr;
	dba_file_bufr_create = (dba_file_create_fun)oldCrex;
	dba_file_crex_create = (dba_file_create_fun)oldAof;
}

}

// vim:set ts=4 sw=4:
