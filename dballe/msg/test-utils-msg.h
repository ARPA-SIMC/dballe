/*
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

typedef wibble::tests::Location Location;

std::auto_ptr<Msgs> _read_msgs(const Location& loc, const char* filename, Encoding type, const dballe::msg::Importer::Options& opts=dballe::msg::Importer::Options());
#define read_msgs(filename, type) dballe::tests::_read_msgs(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define inner_read_msgs(filename, type) dballe::tests::_read_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define read_msgs_opts(filename, type, opts) dballe::tests::_read_msgs(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type), (opts))
#define inner_read_msgs_opts(filename, type, opts) dballe::tests::_read_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type), (opts))

std::auto_ptr<Msgs> _read_msgs_csv(const Location& loc, const char* filename);
#define read_msgs_csv(filename) dballe::tests::_read_msgs_csv(wibble::tests::Location(__FILE__, __LINE__, "load csv " #filename), (filename))
#define inner_read_msgs_csv(filename) dballe::tests::_read_msgs_csv(wibble::tests::Location(loc, __FILE__, __LINE__, "load csv " #filename), (filename))

void track_different_msgs(const Msg& msg1, const Msg& msg2, const std::string& prefix);
void track_different_msgs(const Msgs& msgs1, const Msgs& msgs2, const std::string& prefix);

extern const char* bufr_files[];
extern const char* crex_files[];
extern const char* aof_files[];

void _ensure_msg_undef(const Location& loc, const Msg& msg, int shortcut);
#define ensure_msg_undef(msg, id) dballe::tests::_ensure_msg_undef(wibble::tests::Location(__FILE__, __LINE__, #msg " has undefined " #id), (msg), (id))
#define inner_ensure_msg_undef(msg, id) dballe::tests::_ensure_msg_undef(wibble::tests::Location(loc, __FILE__, __LINE__, #msg " has undefined " #id), (msg), (id))

const wreport::Var& _want_var(const Location& loc, const Msg& msg, int shortcut);
const wreport::Var& _want_var(const Location& loc, const Msg& msg, wreport::Varcode code, const dballe::Level& lev, const dballe::Trange& tr);
#define want_var(msg, ...) dballe::tests::_want_var(wibble::tests::Location(__FILE__, __LINE__, #msg " needs to have var " #__VA_ARGS__), (msg), __VA_ARGS__)


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
