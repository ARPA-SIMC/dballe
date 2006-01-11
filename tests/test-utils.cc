#include <tests/test-utils.h>

#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/aof/aof_decoder.h>

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

}
