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

#include <extra/test-utils-bufrex.h>
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/file.h>
#include <dballe/msg/marshal.h>
#include <dballe/db/db.h>

#include <string>
#include <vector>
#include <iostream>

namespace tut_dballe {
using namespace std;
using namespace tut;

dba_msgs _read_test_msg(const char* file, int line, const char* filename, dba_encoding type);
#define read_test_msg(filename, type) _read_test_msg(__FILE__, __LINE__, filename, type)

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
	
void track_different_msgs(dba_msg msg1, dba_msg msg2, const std::string& prefix);
void track_different_msgs(dba_msgs msgs1, dba_msgs msgs2, const std::string& prefix);

dba_err create_dba_db(dba_db* db);

}

// vim:set ts=4 sw=4:
