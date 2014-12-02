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
#include "db/v6/internals.h"
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
        wassert(actual(st.obtain_id(4500000, 1100000, "ciao")) == 1);

        // Insert a fixed station
        wassert(actual(st.obtain_id(4600000, 1200000)) == 2);

        // Insert a lev_tr
        wassert(actual(lt.obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

        // Insert another lev_tr
        wassert(actual(lt.obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);

        // Insert a datum
        da.set_context(1, 1, 1);
        da.set_date(2001, 2, 3, 4, 5, 6);
        da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 123));

        // Insert another datum
        da.set_context(2, 2, 2);
        da.set_date(2002, 3, 4, 5, 6, 7);
        da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 234));
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
