/*
 * DB-ALLe - Archive for punctual meteorological data
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

#include <dballe/core/test-utils-core.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>
#if 0
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/file.h>
#include <dballe/msg/marshal.h>

#include <string>
#include <vector>
#include <iostream>
#endif

namespace dballe {
namespace tests {

std::auto_ptr<Msgs> _read_msgs(const wibble::tests::Location& loc, const char* filename, Encoding type, const dballe::msg::Importer::Options& opts=dballe::msg::Importer::Options());
#define read_msgs(filename, type) dballe::tests::_read_msgs(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define inner_read_msgs(filename, type) dballe::tests::_read_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define read_msgs_opts(filename, type, opts) dballe::tests::_read_msgs(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type), (opts))
#define inner_read_msgs_opts(filename, type, opts) dballe::tests::_read_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type), (opts))

void track_different_msgs(const Msg& msg1, const Msg& msg2, const std::string& prefix);
void track_different_msgs(const Msgs& msgs1, const Msgs& msgs2, const std::string& prefix);

#if 0

/* Random message generation functions */

class msg_generator : public generator
{
public:
	dba_err fill_message(dba_msg msg, bool mobile);
};


/* Message reading functions */

class msg_vector : public dba_raw_consumer, public std::vector<dba_msgs>
{
public:
	virtual ~msg_vector()
	{
		for (iterator i = begin(); i != end(); i++)
			dba_msgs_delete(*i);
	}
		
	virtual dba_err consume(dba_rawmsg raw)
	{
		dba_msgs msgs;

		DBA_RUN_OR_RETURN(dba_marshal_decode(raw, &msgs));
		push_back(msgs);

		return dba_error_ok();
	}
};
	
dba_var my_want_var(const char* file, int line, dba_msg msg, int id, const char* idname);
#define want_var(msg, id) my_want_var(__FILE__, __LINE__, (msg), (id), #id)

dba_var my_want_var_at(const char* file, int line, dba_msg msg, dba_varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2);
#define want_var_at(msg, code, ltype1, l1, ltype2, l2, pind, p1, p2) my_want_var_at(__FILE__, __LINE__, (msg), (code), (ltype1), (l1), (ltype2), (l2), (pind), (p1), (p2))


void my_ensure_msg_undef(const char* file, int line, dba_msg msg, int id, const char* idname);
#define gen_ensure_msg_undef(msg, id) my_ensure_msg_undef(__FILE__, __LINE__, (msg), (id), #id)
#define inner_ensure_msg_undef(msg, id) my_ensure_msg_undef(file, line, (msg), (id), #id)

template <typename T>
void my_ensure_msg_equals(const char* file, int line, dba_msg msg, int id, const char* idname, const T& value)
{
	dba_var var = my_want_var(file, line, msg, id, idname);
	inner_ensure_var_equals(var, value);
}
#define gen_ensure_msg_equals(msg, id, value) my_ensure_msg_equals(__FILE__, __LINE__, (msg), (id), #id, (value))
#define inner_ensure_msg_equals(msg, id, value) my_ensure_msg_equals(file, line, (msg), (id), #id, (value))
#endif

}
}

// vim:set ts=4 sw=4:
