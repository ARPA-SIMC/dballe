/*
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "db/test-utils-db.h"
#include "db/internals.h"
#include "db/v6/db.h"
#include "db/v6/attr.h"
#include "db/v6/data.h"
#include "db/v6/lev_tr.h"
#include "db/v5/station.h"
#include "db/internals.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct dbv6_attr_shar : public dballe::tests::db_test
{
    db::v6::Attr* at;

    dbv6_attr_shar() : dballe::tests::db_test(db::V6)
    {
        if (!has_db()) return;
        at = &v6().attr();

        db::v5::Station& st = v6().station();
        db::v6::LevTr& lt = v6().lev_tr();
        db::v6::Data& da = v6().data();

		// Insert a mobile station
		st.lat = 4500000;
		st.lon = 1100000;
		st.set_ident("ciao");
        ensure_equals(st.insert(), 1);

		// Insert a fixed station
		st.lat = 4600000;
		st.lon = 1200000;
		st.set_ident(NULL);
		ensure_equals(st.insert(), 2);

        // Insert a lev_tr
        lt.ltype1 = 1;
        lt.l1 = 2;
        lt.ltype2 = 0;
        lt.l2 = 3;
        lt.pind = 4;
        lt.p1 = 5;
        lt.p2 = 6;
        ensure_equals(lt.insert(), 1);

        // Insert another lev_tr
        lt.ltype1 = 2;
        lt.l1 = 3;
        lt.ltype2 = 1;
        lt.l2 = 4;
        lt.pind = 5;
        lt.p1 = 6;
        lt.p2 = 7;
        ensure_equals(lt.insert(), 2);

        // Insert a datum
        da.id_station = 1;
        da.id_report = 1;
        da.set_id_lev_tr(1);
        da.date = make_sql_timestamp(2001, 2, 3, 4, 5, 6);
        da.id_var = WR_VAR(0, 1, 2);
        da.set_value("123");
        da.insert_or_fail();

        // Insert another datum
        da.id_station = 2;
        da.id_report = 2;
        da.set_id_lev_tr(2);
        da.date = make_sql_timestamp(2002, 3, 4, 5, 6, 7);
        da.id_var = WR_VAR(0, 1, 2);
        da.set_value("234");
        da.insert_or_fail();
    }
};
TESTGRP(dbv6_attr);

/* Test dba_db_data_set* */
template<> template<>
void to::test<1>()
{
#if 0
    // Currently these are inaccessible internals
	use_db();

	// Test dba_db_attr_set
	at->set(var(WR_VAR(0, 1, 2), 123));
	ensure_equals(at->value, string("123"));
	ensure_equals(at->value_ind, 3);
	
	// Test dba_db_attr_set_value
    at->set_value("32");
	ensure_equals(at->value, string("32"));
	ensure_equals(at->value_ind, 2);
#endif
}


/* Insert some values and try to read them again */
template<> template<>
void to::test<2>()
{
    use_db();

    // Insert a datum
    at->write(1, Var(varinfo(WR_VAR(0, 33, 7)), 50));

    // Insert another datum
    at->write(2, Var(varinfo(WR_VAR(0, 33, 7)), 75));

    // Reinsert a datum: it should work
    at->write(1, Var(varinfo(WR_VAR(0, 33, 7)), 50));

    // Reinsert the other datum: it should work
    at->write(2, Var(varinfo(WR_VAR(0, 33, 7)), 75));

    // Load the attributes for the first variable
    {
        Var var(varinfo(WR_VAR(0, 1, 2)));
        wassert(actual(var.next_attr()).isfalse());
        at->read(1, var);
        wassert(actual(var.next_attr()).istrue());
        const Var* attr = var.next_attr();
        wassert(actual(attr->value()) == "50");
        wassert(actual(attr->next_attr()).isfalse());
    }

    // Load the attributes for the second variable
    {
        Var var(varinfo(WR_VAR(0, 1, 2)));
        wassert(actual(var.next_attr()).isfalse());
        at->read(2, var);
        wassert(actual(var.next_attr()).istrue());
        const Var* attr = var.next_attr();
        wassert(actual(attr->value()) == "75");
        wassert(actual(attr->next_attr()).isfalse());
    }
}

}

/* vim:set ts=4 sw=4: */
