#include "test-utils.h"

#include <../../dballe/bufrex/bufrex.h>
#include <../../dballe/bufrex/bufrex_raw.h>
#include <../../dballe/aof/decoder.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace tut_dballe {

static std::string tag;

bufrex_raw _read_test_msg_raw(const char* file, int line, const char* filename, dba_encoding type)
{
	dba_file input;
	dba_rawmsg rawmsg;
	int found;

	inner_ensure(type == BUFR || type == CREX);

	// Read the sample message
	INNER_CHECKED(dba_rawmsg_create(&rawmsg));
	INNER_CHECKED(dba_file_create(&input, type, filename, "r"));
	INNER_CHECKED(dba_file_read_raw(input, rawmsg, &found));
	inner_ensure_equals(found, 1);

	dba_file_delete(input);

	// Decode the sample message
	bufrex_raw bufrex;
	switch (type)
	{
		case BUFR: INNER_CHECKED(bufrex_raw_create(&bufrex, BUFREX_BUFR)); break;
		case CREX: INNER_CHECKED(bufrex_raw_create(&bufrex, BUFREX_CREX)); break;
		default: inner_ensure(false); break;
	}
	INNER_CHECKED(bufrex_raw_decode(bufrex, rawmsg));
	
	dba_rawmsg_delete(rawmsg);
	return bufrex;
}

dba_msg _read_test_msg(const char* file, int line, const char* filename, dba_encoding type)
{
	if (type == AOF)
	{
		dba_file input;
		dba_rawmsg rawmsg;
		int found;

		// Read the sample message
		INNER_CHECKED(dba_rawmsg_create(&rawmsg));
		INNER_CHECKED(dba_file_create(&input, type, filename, "r"));
		INNER_CHECKED(dba_file_read_raw(input, rawmsg, &found));
		inner_ensure_equals(found, 1);

		dba_file_delete(input);

		// Decode the sample message
		dba_msg msg;
		INNER_CHECKED(aof_decoder_decode(rawmsg, &msg));
		dba_rawmsg_delete(rawmsg);
		return msg;
	} else {
		bufrex_raw bufrex = _read_test_msg_raw(file, line, filename, type);

		// Parse the decoded message into a synop
		dba_msg msg;
		INNER_CHECKED(bufrex_raw_to_msg(bufrex, &msg));

		bufrex_raw_delete(bufrex);
		return msg;
	}
}

bufrex_raw _reencode_test(const char* file, int line, bufrex_raw msg)
{
	dba_rawmsg raw;
	INNER_CHECKED(bufrex_raw_encode(msg, &raw));

	bufrex_raw bufrex;
	INNER_CHECKED(bufrex_raw_create(&bufrex, msg->encoding_type));
	INNER_CHECKED(bufrex_raw_decode(bufrex, raw));

	dba_rawmsg_delete(raw);

	return bufrex;
}

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
		DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_REP_COD, rnd(0, 20)));

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

dba_err generator::fill_message(dba_msg msg, bool mobile)
{
	dba_record rec;
	DBA_RUN_OR_RETURN(dba_record_create(&rec));

	DBA_RUN_OR_RETURN(fill_pseudoana(rec, mobile));
	DBA_RUN_OR_RETURN(dba_msg_set_latitude_var(msg,		dba_record_key_peek(rec, DBA_KEY_LAT)));
	DBA_RUN_OR_RETURN(dba_msg_set_longitude_var(msg,	dba_record_key_peek(rec, DBA_KEY_LON)));
	/* DBA_RUN_OR_RETURN(dba_msg_set_name_var(msg,			dba_record_key_peek(rec, DBA_KEY_NAME))); */
	if (mobile)
	{
		DBA_RUN_OR_RETURN(dba_msg_set_ident_var(msg,	dba_record_key_peek(rec, DBA_KEY_IDENT)));
	} else {
		DBA_RUN_OR_RETURN(dba_msg_set_block_var(msg,	dba_record_key_peek(rec, DBA_KEY_BLOCK)));
		DBA_RUN_OR_RETURN(dba_msg_set_station_var(msg,	dba_record_key_peek(rec, DBA_KEY_STATION)));
	}

	DBA_RUN_OR_RETURN(fill_context(rec));
	DBA_RUN_OR_RETURN(dba_msg_set_year_var(msg,		dba_record_key_peek(rec, DBA_KEY_YEAR)));
	DBA_RUN_OR_RETURN(dba_msg_set_month_var(msg,	dba_record_key_peek(rec, DBA_KEY_MONTH)));
	DBA_RUN_OR_RETURN(dba_msg_set_day_var(msg,		dba_record_key_peek(rec, DBA_KEY_DAY)));
	DBA_RUN_OR_RETURN(dba_msg_set_hour_var(msg,		dba_record_key_peek(rec, DBA_KEY_HOUR)));
	DBA_RUN_OR_RETURN(dba_msg_set_minute_var(msg,	dba_record_key_peek(rec, DBA_KEY_MIN)));

	for (int i = 0; i < rnd(4, 20); i++)
	{
		DBA_RUN_OR_RETURN(fill_context(rec));

		int ltype, l1, l2, pind, p1, p2;
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE, &ltype));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L1, &l1));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L2, &l2));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_PINDICATOR, &pind));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P1, &p1));
		DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P2, &p2));

		dba_var var;
		DBA_RUN_OR_RETURN(dba_var_create_local(generator_varcodes[rnd(0, sizeof(generator_varcodes) / sizeof(dba_varcode))], &var));
		DBA_RUN_OR_RETURN(dba_var_setc(var, "11111"));
		DBA_RUN_OR_RETURN(dba_msg_set_nocopy(msg, var, ltype, l1, l2, pind, p1, p2));
	}
	return dba_error_ok();
}

dba_err read_file(dba_encoding type, const std::string& name, dba_raw_consumer& cons)
{
	dba_err err = DBA_OK;
	dba_file file = 0;
	dba_rawmsg raw = 0;
	int found;

	DBA_RUN_OR_GOTO(cleanup, dba_file_create(&file, type, name.c_str(), "r"));
	DBA_RUN_OR_GOTO(cleanup, dba_rawmsg_create(&raw));

	DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, raw, &found));
	while (found)
	{
		DBA_RUN_OR_GOTO(cleanup, cons.consume(raw));
		DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, raw, &found));
	}

cleanup:
	if (file) dba_file_delete(file);
	if (raw) dba_rawmsg_delete(raw);
	return err == DBA_OK ? dba_error_ok() : err;
}

void track_different_msgs(dba_msg msg1, dba_msg msg2, const std::string& prefix)
{
	string fname1 = "/tmp/test-" + prefix + "1.bufr";
	string fname2 = "/tmp/test-" + prefix + "2.bufr";
	FILE* out1 = fopen(fname1.c_str(), "w");
	FILE* out2 = fopen(fname2.c_str(), "w");
	dba_msg_print(msg1, out1);
	dba_msg_print(msg2, out2);
	fclose(out1);
	fclose(out2);
	cerr << "Wrote mismatching messages to " << fname1 << " and " << fname2 << endl;
}

dba_err create_dba_db(dba_db* db)
{
	const char* uname = getenv("DBA_USER");
	if (uname == NULL)
	{
		struct passwd *pwd = getpwuid(getuid());
		uname = pwd == NULL ? "test" : pwd->pw_name;
	}
	return dba_db_create("test", uname , "", db);
}

}
