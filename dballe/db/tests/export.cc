/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006,2007  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-db.h>
#include <dballe/db/export.h>

namespace tut {
using namespace tut_dballe;

struct db_export_shar
{
	// DB handle
	dba_db db;

	db_export_shar() : db(NULL)
	{
		CHECKED(create_dba_db(&db));
		CHECKED(dba_db_reset(db, NULL));

		dba_record rec;
		CHECKED(dba_record_create(&rec));

		// Insert some data
		CHECKED(dba_record_key_setd(rec, DBA_KEY_LAT, 12.34560));
		CHECKED(dba_record_key_setd(rec, DBA_KEY_LON, 76.54321));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));

		CHECKED(dba_record_key_seti(rec, DBA_KEY_YEAR, 1945));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_MONTH, 4));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_DAY, 25));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_HOUR, 8));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_MIN, 0));

		CHECKED(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE, 1));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_L1, 2));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_L2, 3));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_PINDICATOR, 4));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_P1, 5));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_P2, 6));

		CHECKED(dba_record_key_seti(rec, DBA_KEY_REP_COD, 1));

		CHECKED(dba_record_var_seti(rec, DBA_VAR(0, 1, 12), 500));

		CHECKED(dba_db_insert(db, rec, 0, 1, NULL, NULL));

		CHECKED(dba_record_key_seti(rec, DBA_KEY_DAY, 26));
		CHECKED(dba_record_var_seti(rec, DBA_VAR(0, 1, 12), 400));

		CHECKED(dba_db_insert(db, rec, 0, 1, NULL, NULL));

		CHECKED(dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
		CHECKED(dba_record_key_setc(rec, DBA_KEY_IDENT, "ciao"));
		CHECKED(dba_record_var_seti(rec, DBA_VAR(0, 1, 12), 300));

		CHECKED(dba_db_insert(db, rec, 0, 1, NULL, NULL));

		dba_record_delete(rec);
	}

	~db_export_shar()
	{
		if (db != NULL) dba_db_delete(db);
		test_untag();
	}
};
TESTGRP(db_export);

static dba_err msg_collector(dba_msgs msgs, void* data)
{
//	cerr << "MSG COLLECTOR";
	vector<dba_msg>* vec = static_cast<vector<dba_msg>*>(data);
	for (int i = 0; i < msgs->len; ++i)
	{
//		cerr << " got " << i << "/" << msgs->len << ":" << (int)msgs->msgs[i];
		(*vec).push_back(msgs->msgs[i]);
		// Detach the message from the msgs
		msgs->msgs[i] = NULL;
	}
//	cerr << endl;
	dba_msgs_delete(msgs);
	return dba_error_ok();
}

// Put some data in the database and check that it gets exported properly
template<> template<>
void to::test<1>()
{
	// Query back the data
	dba_record query;
	CHECKED(dba_record_create(&query));

	vector<dba_msg> msgs;
	CHECKED(dba_db_export(db, query, msg_collector, &msgs));

	gen_ensure_equals(msgs.size(), 3u);

	gen_ensure_equals(msgs[0]->type, MSG_SYNOP);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_LATITUDE, 12.34560);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_LONGITUDE, 76.54321);
	gen_ensure_msg_undef(msgs[0], DBA_MSG_IDENT);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_YEAR, 1945);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_MONTH, 4);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_DAY, 25);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_HOUR, 8);
	gen_ensure_msg_equals(msgs[0], DBA_MSG_MINUTE, 0);
	dba_var var = want_var_at(msgs[0], DBA_VAR(0, 1, 12), 1, 2, 3, 4, 5, 6);
	gen_ensure_var_equals(var, 500);

	gen_ensure_msg_equals(msgs[1], DBA_MSG_LATITUDE, 12.34560);
	gen_ensure_msg_equals(msgs[1], DBA_MSG_LONGITUDE, 76.54321);
	gen_ensure_msg_undef(msgs[1], DBA_MSG_IDENT);
	gen_ensure_msg_equals(msgs[1], DBA_MSG_YEAR, 1945);
	gen_ensure_msg_equals(msgs[1], DBA_MSG_MONTH, 4);
	gen_ensure_msg_equals(msgs[1], DBA_MSG_DAY, 26);
	gen_ensure_msg_equals(msgs[1], DBA_MSG_HOUR, 8);
	gen_ensure_msg_equals(msgs[1], DBA_MSG_MINUTE, 0);
	var = want_var_at(msgs[1], DBA_VAR(0, 1, 12), 1, 2, 3, 4, 5, 6);
	gen_ensure_var_equals(var, 400);

	gen_ensure_msg_equals(msgs[2], DBA_MSG_LATITUDE, 12.34560);
	gen_ensure_msg_equals(msgs[2], DBA_MSG_LONGITUDE, 76.54321);
	gen_ensure_msg_equals(msgs[2], DBA_MSG_IDENT, "ciao");
	gen_ensure_msg_equals(msgs[2], DBA_MSG_YEAR, 1945);
	gen_ensure_msg_equals(msgs[2], DBA_MSG_MONTH, 4);
	gen_ensure_msg_equals(msgs[2], DBA_MSG_DAY, 26);
	gen_ensure_msg_equals(msgs[2], DBA_MSG_HOUR, 8);
	gen_ensure_msg_equals(msgs[2], DBA_MSG_MINUTE, 0);
	var = want_var_at(msgs[2], DBA_VAR(0, 1, 12), 1, 2, 3, 4, 5, 6);
	gen_ensure_var_equals(var, 300);

	dba_record_delete(query);
}

}

// vim:set ts=4 sw=4:
