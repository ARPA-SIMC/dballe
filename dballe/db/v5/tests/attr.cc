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
#include "db/odbc/internals.h"
#include "db/v5/db.h"
#include "db/v5/attr.h"
#include "db/v5/data.h"
#include "db/v5/context.h"
#include "db/v5/station.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::db::v5;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct dbv5_attr_shar : public dballe::tests::db_test
{
    Attr* at;

    dbv5_attr_shar() : dballe::tests::db_test(db::V5)
    {
        if (!has_db()) return;
        at = &v5().attr();

        Station& st = v5().station();
        Context& co = v5().context();
        Data& da = v5().data();

        // Insert a mobile station
        wassert(actual(st.obtain_id(4500000, 1100000, "ciao")) == 1);

        // Insert a fixed station
        wassert(actual(st.obtain_id(4600000, 1200000)) == 2);

		// Insert a context
		co.id_station = 1;
		co.id_report = 1;
		co.date = make_sql_timestamp(2001, 2, 3, 4, 5, 6);
		co.ltype1 = 1;
		co.l1 = 2;
		co.ltype2 = 0;
		co.l2 = 3;
		co.pind = 4;
		co.p1 = 5;
		co.p2 = 6;
		ensure_equals(co.insert(), 1);

		// Insert another context
		co.id_station = 2;
		co.id_report = 2;
		co.date = make_sql_timestamp(2002, 3, 4, 5, 6, 7);
		co.ltype1 = 2;
		co.l1 = 3;
		co.ltype2 = 1;
		co.l2 = 4;
		co.pind = 5;
		co.p1 = 6;
		co.p2 = 7;
		ensure_equals(co.insert(), 2);

		// Insert a datum
		da.id_context = 1;
		da.id_var = WR_VAR(0, 1, 2);
		da.set_value("123");
        da.insert_or_fail();

		// Insert another datum
		da.id_context = 2;
		da.id_var = WR_VAR(0, 1, 2);
		da.set_value("234");
        da.insert_or_fail();
	}
};
TESTGRP(dbv5_attr);

/* Test dba_db_data_set* */
template<> template<>
void to::test<1>()
{
	use_db();

	// Test dba_db_attr_set
	at->set(var(WR_VAR(0, 1, 2), 123));
	ensure_equals(at->value, string("123"));
	ensure_equals(at->value_ind, 3);
	
	// Test dba_db_attr_set_value
    at->set_value("32");
	ensure_equals(at->value, string("32"));
	ensure_equals(at->value_ind, 2);
}


/* Insert some values and try to read them again */
template<> template<>
void to::test<2>()
{
	use_db();

	// Insert a datum
	at->id_context = 1;
	at->id_var = WR_VAR(0, 1, 2);
	at->type = WR_VAR(0, 33, 7);
	at->set_value("50");
    at->insert();

	// Insert another datum
	at->id_context = 2;
	at->id_var = WR_VAR(0, 1, 2);
	at->type = WR_VAR(0, 33, 7);
	at->set_value("75");
    at->insert();

    // Reinsert a datum: it should work
	at->id_context = 1;
	at->id_var = WR_VAR(0, 1, 2);
	at->type = WR_VAR(0, 33, 7);
	at->set_value("50");
    at->insert();

    // Reinsert the other datum: it should work
	at->id_context = 2;
	at->id_var = WR_VAR(0, 1, 2);
	at->type = WR_VAR(0, 33, 7);
	at->set_value("75");
    at->insert();

	// Load the attributes for the first variable
    {
        Var var(varinfo(WR_VAR(0, 1, 2)));
        at->id_context = 1;
        ensure(var.next_attr() == 0);
        at->load(var);
        ensure(var.next_attr() != 0);
        const Var* attr = var.next_attr();
        ensure_equals(string(attr->value()), "50");
        ensure(attr->next_attr() == NULL);
    }

	// Load the attributes for the second variable
    {
        Var var(varinfo(WR_VAR(0, 1, 2)));
        at->id_context = 2;
        ensure(var.next_attr() == 0);
        at->load(var);
        ensure(var.next_attr() != 0);
        const Var* attr = var.next_attr();
        ensure_equals(string(attr->value()), "75");
        ensure(attr->next_attr() == NULL);
    }

#if 0
	// Get the ID of the first data
	co->id = 0;
	co->id_ana = 1;
	co->id_report = 1;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2001, 2, 3, 4, 5, 6);
	co->ltype = 1;
	co->l1 = 2;
	co->l2 = 3;
	co->pind = 4;
	co->p1 = 5;
	co->p2 = 6;
	CHECKED(dba_db_data_get_id(co, &id));
	ensure_equals(id, 1);

	// Get the ID of the second data
	co->id = 0;
	co->id_ana = 2;
	co->id_report = 2;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2002, 3, 4, 5, 6, 7);
	co->ltype = 2;
	co->l1 = 3;
	co->l2 = 4;
	co->pind = 5;
	co->p1 = 6;
	co->p2 = 7;
	CHECKED(dba_db_data_get_id(co, &id));
	ensure_equals(id, 2);

	// Get info on the first data
	CHECKED(dba_db_data_get_data(co, 1));
	ensure_equals(co->id_ana, 1);
	ensure_equals(co->id_report, 1);
	ensure_equals(co->date, string("2001-02-03 04:05:06"));
	ensure_equals(co->date_ind, 19);
	ensure_equals(co->ltype, 1);
	ensure_equals(co->l1, 2);
	ensure_equals(co->l2, 3);
	ensure_equals(co->pind, 4);
	ensure_equals(co->p1, 5);
	ensure_equals(co->p2, 6);

	// Get info on the second data
	CHECKED(dba_db_data_get_data(co, 2));
	ensure_equals(co->id_ana, 2);
	ensure_equals(co->id_report, 2);
	ensure_equals(co->date, string("2002-03-04 05:06:07"));
	ensure_equals(co->date_ind, 19);
	ensure_equals(co->ltype, 2);
	ensure_equals(co->l1, 3);
	ensure_equals(co->l2, 4);
	ensure_equals(co->pind, 5);
	ensure_equals(co->p1, 6);
	ensure_equals(co->p2, 7);
#endif

#if 0
	// Update the second data
	co->id = 2;
	co->id_ana = 2;
	co->id_report = 2;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2003, 4, 5, 6, 7, 8);
	co->ltype = 3;
	co->l1 = 4;
	co->l2 = 5;
	co->pind = 6;
	co->p1 = 7;
	co->p2 = 8;
	CHECKED(dba_db_data_update(co));

	// Get info on the first station: it should be unchanged
	CHECKED(dba_db_data_get_data(pa, 1));
	ensure_equals(pa->lat, 4500000);
	ensure_equals(pa->lon, 1100000);
	ensure_equals(pa->ident, string("ciao"));
	ensure_equals(pa->ident_ind, 4);

	// Get info on the second station: it should be updated
	CHECKED(dba_db_data_get_data(pa, 2));
	ensure_equals(pa->lat, 4700000);
	ensure_equals(pa->lon, 1300000);
	ensure_equals(pa->ident[0], 0);
#endif
}

}

/* vim:set ts=4 sw=4: */
