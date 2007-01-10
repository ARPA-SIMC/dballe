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

#include <test-utils-db.h>
#include <dballe/db/db.h>
#include <dballe/db/pseudoana.h>
#include <dballe/db/context.h>

namespace tut {
using namespace tut_dballe;

struct context_shar
{
	// DB handle
	dba_db db;
	dba_db_context co;

	context_shar() : db(NULL)
	{
		CHECKED(create_dba_db(&db));
		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_db_need_context(db));
		co = db->context;
		gen_ensure(co != NULL);

		CHECKED(dba_db_need_pseudoana(db));
		dba_db_pseudoana pa = db->pseudoana;
		gen_ensure(pa != NULL);

		// Insert a fixed station
		int id;
		pa->lat = 4500000;
		pa->lon = 1100000;
		dba_db_pseudoana_set_ident(pa, "ciao");
		CHECKED(dba_db_pseudoana_insert(pa, &id));
		gen_ensure_equals(id, 1);

		// Insert a mobile station
		pa->lat = 4600000;
		pa->lon = 1200000;
		dba_db_pseudoana_set_ident(pa, NULL);
		CHECKED(dba_db_pseudoana_insert(pa, &id));
		gen_ensure_equals(id, 2);
	}

	~context_shar()
	{
		if (db != NULL) dba_db_delete(db);
	}
};
TESTGRP(context);

/* Insert some values and try to read them again */
template<> template<>
void to::test<1>()
{
	// Insert a context
	int id;
	co->id_ana = 1;
	co->id_report = 1;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2001, 2, 3, 4, 5, 6);
	co->ltype = 1;
	co->l1 = 2;
	co->l2 = 3;
	co->pind = 4;
	co->p1 = 5;
	co->p2 = 6;
	CHECKED(dba_db_context_insert(co, &id));
	gen_ensure_equals(id, 1);

	// Insert another context
	co->id_ana = 2;
	co->id_report = 2;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2002, 3, 4, 5, 6, 7);
	co->ltype = 2;
	co->l1 = 3;
	co->l2 = 4;
	co->pind = 5;
	co->p1 = 6;
	co->p2 = 7;
	CHECKED(dba_db_context_insert(co, &id));
	gen_ensure_equals(id, 2);

	// Get the ID of the first context
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
	CHECKED(dba_db_context_get_id(co, &id));
	gen_ensure_equals(id, 1);

	// Get the ID of the second context
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
	CHECKED(dba_db_context_get_id(co, &id));
	gen_ensure_equals(id, 2);

	// Get info on the first context
	CHECKED(dba_db_context_get_data(co, 1));
	gen_ensure_equals(co->id_ana, 1);
	gen_ensure_equals(co->id_report, 1);
	gen_ensure_equals(co->date, string("2001-02-03 04:05:06"));
	gen_ensure_equals(co->date_ind, 19);
	gen_ensure_equals(co->ltype, 1);
	gen_ensure_equals(co->l1, 2);
	gen_ensure_equals(co->l2, 3);
	gen_ensure_equals(co->pind, 4);
	gen_ensure_equals(co->p1, 5);
	gen_ensure_equals(co->p2, 6);

	// Get info on the second context
	CHECKED(dba_db_context_get_data(co, 2));
	gen_ensure_equals(co->id_ana, 2);
	gen_ensure_equals(co->id_report, 2);
	gen_ensure_equals(co->date, string("2002-03-04 05:06:07"));
	gen_ensure_equals(co->date_ind, 19);
	gen_ensure_equals(co->ltype, 2);
	gen_ensure_equals(co->l1, 3);
	gen_ensure_equals(co->l2, 4);
	gen_ensure_equals(co->pind, 5);
	gen_ensure_equals(co->p1, 6);
	gen_ensure_equals(co->p2, 7);

#if 0
	// Update the second context
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
	CHECKED(dba_db_context_update(co));

	// Get info on the first station: it should be unchanged
	CHECKED(dba_db_context_get_data(pa, 1));
	gen_ensure_equals(pa->lat, 4500000);
	gen_ensure_equals(pa->lon, 1100000);
	gen_ensure_equals(pa->ident, string("ciao"));
	gen_ensure_equals(pa->ident_ind, 4);

	// Get info on the second station: it should be updated
	CHECKED(dba_db_context_get_data(pa, 2));
	gen_ensure_equals(pa->lat, 4700000);
	gen_ensure_equals(pa->lon, 1300000);
	gen_ensure_equals(pa->ident[0], 0);
#endif

	// Get a pseudoana context entry for the first station
	co->id_ana = 1;
	co->id_report = -1;
	CHECKED(dba_db_context_obtain_ana(co, &id));
	gen_ensure_equals(id, 3);

	// Get a pseudoana context entry for the second station
	co->id_ana = 2;
	co->id_report = -1;
	CHECKED(dba_db_context_obtain_ana(co, &id));
	gen_ensure_equals(id, 4);

	// Get a pseudoana context entry for the first station again, to see that only one is created
	co->id_ana = 1;
	co->id_report = -1;
	CHECKED(dba_db_context_obtain_ana(co, &id));
	gen_ensure_equals(id, 3);

	// Get a pseudoana context entry for the second station again, to see that only one is created
	co->id_ana = 2;
	co->id_report = -1;
	CHECKED(dba_db_context_obtain_ana(co, &id));
	gen_ensure_equals(id, 4);
}

}

/* vim:set ts=4 sw=4: */
