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

#include <tests/test-utils.h>
#include <dballe/core/record.h>

/*
extern "C" {
	dba_err dba_record_keyword_selftest();
};
*/

namespace tut {
using namespace tut_dballe;

struct dba_core_record_shar
{
	dba_record rec;

	dba_core_record_shar()
	{
		CHECKED(dba_record_create(&rec));
	}

	~dba_core_record_shar()
	{
		dba_record_delete(rec);
	}
};
TESTGRP(dba_core_record);


/*
#define fail_unless_chain_is(chain, param, compare) \
{ \
	dba_item item; \
	dba_err err = dba_item_get(chain, param, &item); \
	fail_unless(err == DBA_OK); \
	fail_unless(item == compare); \
}

void fail_unless_chain_hasnt(dba_item chain, dba_varcode param)
{
	dba_item item = NULL;
	gen_ensure(dba_item_get(chain, param, &item) == DBA_ERROR);
}

void fail_unless_is_it(dba_record rec, dba_varcode param)
{
	dba_item item = NULL;
	dba_err err = dba_record_get_item(rec, param, &item);
	if (err) print_dba_error();
	fail_unless(err == DBA_OK);
	fail_unless(item != NULL);
	fail_unless(dba_var_code(item->var) == param);
	fail_unless(atoi(dba_var_value(item->var)) == param);
}

void fail_unless_has(dba_record rec, dba_varcode param)
{
	dba_item item = NULL;
	dba_err err = dba_record_get_item(rec, param, &item);
	if (err) print_dba_error();
	fail_unless(err == DBA_OK);
	fail_unless(item != NULL);
	fail_unless(dba_var_code(item->var) == param);
}

void fail_unless_hasnt(dba_record rec, dba_varcode param)
{
	dba_item item = NULL;
	fail_unless(dba_record_get_item(rec, param, &item) == DBA_ERROR);
}
*/

#define fail_unless_int_is(keyvar, param, value) do { \
		int val; \
		CHECKED(dba_record_##keyvar##_enqi(rec, param, &val)); \
		gen_ensure_equals(val, value); \
	} while (0)

#define fail_unless_double_is(keyvar, param, value) do { \
		double val; \
		CHECKED(dba_record_##keyvar##_enqd(rec, param, &val)); \
		gen_ensure_equals(val, value); \
	} while (0)

#define fail_unless_char_is(keyvar, param, value) do { \
		const char* val; \
		CHECKED(dba_record_##keyvar##_enqc(rec, param, &val)); \
		gen_ensure_equals(string(val), string(value)); \
	} while (0)


// Check handling of keyword and variable information
template<> template<>
void to::test<1>()
{
	dba_varinfo info1, info2;

	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 1, 1), &info1));

	/* Keyword info handling */
	dba_varinfo info;

	gen_ensure(dba_record_keyword_byname("cippo") == DBA_KEY_ERROR);
	gen_ensure(dba_record_keyword_byname("zzzip") == DBA_KEY_ERROR);

	gen_ensure_equals(dba_record_keyword_byname("ana_id"), DBA_KEY_ANA_ID);
	CHECKED(dba_record_keyword_info(DBA_KEY_ANA_ID, &info));
	gen_ensure_equals(string(info->desc), string("Pseudoana database ID"));

	gen_ensure_equals(dba_record_keyword_byname_len("ana_idi", 6), DBA_KEY_ANA_ID);
	CHECKED(dba_record_keyword_info(DBA_KEY_ANA_ID, &info));
	gen_ensure_equals(string(info->desc), string("Pseudoana database ID"));

	gen_ensure_equals(dba_record_keyword_byname("yearmin"), DBA_KEY_YEARMIN);
	CHECKED(dba_record_keyword_info(DBA_KEY_YEARMIN, &info));
	gen_ensure_equals(string(info->desc), string("Year or minimum year queried"));

	gen_ensure_equals(dba_record_keyword_byname("lat"), DBA_KEY_LAT);
	CHECKED(dba_record_keyword_info(DBA_KEY_LAT, &info));
	gen_ensure_equals(string(info->desc), string("Latitude"));

	gen_ensure_equals(dba_record_keyword_byname("lon"), DBA_KEY_LON);
	CHECKED(dba_record_keyword_info(DBA_KEY_LON, &info));
	gen_ensure_equals(string(info->desc), string("Longitude"));
	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 1, 2), &info2));
}

// Test type consistency checks
template<> template<>
void to::test<2>()
{
	dba_err err;

	err = dba_record_key_seti(rec, DBA_KEY_VAR, 1);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_TYPE);

	err = dba_record_var_seti(rec, DBA_VAR(0, 1, 19), 1);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_TYPE);
}

// Test get and set methods
template<> template<>
void to::test<3>()
{
	/* Record gets and sets */
	dba_err err;
	int ival;

	// Check that things don't exist at the beginning
	CHECKED(dba_record_contains_key(rec, DBA_KEY_ANA_ID, &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_key_enqi(rec, DBA_KEY_ANA_ID, &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 1), &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_var_enqi(rec, DBA_VAR(0, 20, 1), &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 3), &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_var_enqi(rec, DBA_VAR(0, 20, 3), &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	// Set various things
	CHECKED(dba_record_key_seti(rec, DBA_KEY_ANA_ID, -10));
	CHECKED(dba_record_key_seti(rec, DBA_KEY_LAT, 1234567));
	CHECKED(dba_record_key_setd(rec, DBA_KEY_LON, 76.54321));
	CHECKED(dba_record_key_setc(rec, DBA_KEY_YEARMIN, "1976"));
	CHECKED(dba_record_var_setc(rec, DBA_VAR(0, 20, 1), "456"));
	CHECKED(dba_record_var_setc(rec, DBA_VAR(0, 20, 3), "456"));

	// Check that they now exist
	CHECKED(dba_record_contains_key(rec, DBA_KEY_ANA_ID, &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 1), &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 3), &ival));
	gen_ensure_equals(ival, 1);

	// Check that they have the right value
	fail_unless_int_is(key, DBA_KEY_ANA_ID, -10);
	fail_unless_double_is(key, DBA_KEY_ANA_ID, -10.0);
	fail_unless_int_is(key, DBA_KEY_LON, 7654321);
	fail_unless_double_is(key, DBA_KEY_LON, 76.54321);
	fail_unless_char_is(key, DBA_KEY_LON, "7654321");

	fail_unless_int_is(key, DBA_KEY_LAT, 1234567);
	fail_unless_double_is(key, DBA_KEY_LAT, 12.34567);
	fail_unless_char_is(key, DBA_KEY_LAT, "1234567");

	fail_unless_int_is(key, DBA_KEY_YEARMIN, 1976);
	fail_unless_double_is(key, DBA_KEY_YEARMIN, 1976);
	fail_unless_char_is(key, DBA_KEY_YEARMIN, "1976");

	fail_unless_int_is(var, DBA_VAR(0, 20, 1), 456);
	/*fail_unless_float_is(rec, "B02121", 45600000000.0)*/;
	fail_unless_double_is(var, DBA_VAR(0, 20, 1), 4560);
	fail_unless_char_is(var, DBA_VAR(0, 20, 1), "456");

	fail_unless_int_is(var, DBA_VAR(0, 20, 3), 456);
	/*fail_unless_float_is(rec, "B02121", 45600000000.0)*/;
	fail_unless_double_is(var, DBA_VAR(0, 20, 3), 456);
	fail_unless_char_is(var, DBA_VAR(0, 20, 3), "456");

	// See if unset works for keywords
	CHECKED(dba_record_key_unset(rec, DBA_KEY_LAT));
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	// See if unset works for variables
	CHECKED(dba_record_var_unset(rec, DBA_VAR(0, 20, 1)));
	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 1), &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_var_enqi(rec, DBA_VAR(0, 20, 1), &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	/* fprintf(stderr, "IVAL: %d\n", ival); */
	/* fprintf(stderr, "DVAL: %f\n", fval); */
	/*
	{
		int i = 7654321;
		double f = (double)i / 100000;
		fprintf(stderr, "I: %d, F: %f\n", i, f);
	}
	*/

	/* See if clear clears */
	dba_record_clear(rec);
	
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 3), &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_var_enqi(rec, DBA_VAR(0, 20, 3), &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	dba_record_clear(rec);

	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 3), &ival));
	gen_ensure_equals(ival, 0);
	err = dba_record_var_enqi(rec, DBA_VAR(0, 20, 3), &ival);
	gen_ensure(err == DBA_ERROR);
	gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);
}


//	CHECKED(dba_record_keyword_selftest());

#if 0
	{
		int i;
		/* Check that the elements in tag_defs are properly sorted */
		for (i = 0; i < (sizeof(tag_defs) / sizeof(struct tagdef)) - 1; i++)
			fail_unless(strcmp(tag_defs[i].tag, tag_defs[i + 1].tag) < 0);
	}
#endif

	/*CHECKED(dba_vartable_create("dballe", &vartable));*/

#if 0
	{
		/* Test dba_item handling */

		dba_item chain = NULL, item = NULL;

		CHECKED(dba_item_obtain(&chain, 1, &item));
		CHECKED(dba_var_createc(info1, &(item->var), "1"));

		fail_unless_chain_is(chain, 1, chain);
		fail_unless_chain_hasnt(chain, 2);

		CHECKED(dba_item_obtain(&chain, 2, &item));
		CHECKED(dba_var_createc(info2, &(item->var), "2"));

		gen_ensure(item != chain);
		
		fail_unless_chain_is(chain, 1, chain);
		fail_unless_chain_is(chain, 2, item);

		dba_item_remove(&chain, 1);
		gen_ensure(item == chain);
		
		fail_unless_chain_hasnt(chain, 1);
		fail_unless_chain_is(chain, 2, item);

		dba_item_remove(&chain, 2);

		gen_ensure(chain == NULL);

		dba_item_delete(chain);
	}
#endif

#if 0
	{
		/* Hash table handling */
		dba_record rec;
		dba_item item = NULL;

		CHECKED(dba_record_create(&rec));
		gen_ensure(rec != NULL);

		fail_unless_hasnt(rec, 1);
		CHECKED(dba_record_obtain_item(rec, 1, &item));
		CHECKED(dba_var_createc(info1, &(item->var), "1"));
		gen_ensure(item != NULL);
		fail_unless_has(rec, 1);

		fail_unless_hasnt(rec, 2);
		CHECKED(dba_record_obtain_item(rec, 2, &item));
		CHECKED(dba_var_createc(info2, &(item->var), "2"));
		gen_ensure(item != NULL);
		fail_unless_has(rec, 1);
		fail_unless_has(rec, 2);

		fail_unless_is_it(rec, 1);
		fail_unless_is_it(rec, 2);

		dba_record_remove_item(rec, 1);
		fail_unless_hasnt(rec, 1);
		fail_unless_has(rec, 2);

		dba_record_remove_item(rec, 1);
		fail_unless_hasnt(rec, 1);
		fail_unless_has(rec, 2);

		dba_record_remove_item(rec, 3);
		fail_unless_hasnt(rec, 1);
		fail_unless_has(rec, 2);

		dba_record_remove_item(rec, 2);
		fail_unless_hasnt(rec, 1);
		fail_unless_hasnt(rec, 2);

		dba_record_remove_item(rec, 3);
		fail_unless_hasnt(rec, 1);
		fail_unless_hasnt(rec, 2);

		dba_record_delete(rec);
	}
#endif

// This created a segfault
template<> template<>
void to::test<4>()
{
	dba_record rec;
	CHECKED(dba_record_create(&rec));
	CHECKED(dba_record_key_setc(rec, DBA_KEY_IDENT, "nosort"));

	dba_record rec1;
	CHECKED(dba_record_create(&rec1));
	CHECKED(dba_record_copy(rec1, rec));
	CHECKED(dba_record_key_setc(rec1, DBA_KEY_IDENT, "nosort"));

	dba_record_delete(rec);
	dba_record_delete(rec1);
}

// Test dba_record_equals
template<> template<>
void to::test<5>()
{
	dba_record rec;
	CHECKED(dba_record_create(&rec));
	CHECKED(dba_record_key_seti(rec, DBA_KEY_ANA_ID, -10));
	CHECKED(dba_record_key_seti(rec, DBA_KEY_LAT, 1234567));
	CHECKED(dba_record_key_setd(rec, DBA_KEY_LON, 76.54321));
	CHECKED(dba_record_key_setc(rec, DBA_KEY_YEARMIN, "1976"));
	CHECKED(dba_record_var_setc(rec, DBA_VAR(0, 20, 1), "456"));
	CHECKED(dba_record_var_setc(rec, DBA_VAR(0, 20, 3), "456"));

	dba_record rec1;
	CHECKED(dba_record_create(&rec1));
	CHECKED(dba_record_copy(rec1, rec));
	gen_ensure(dba_record_equals(rec, rec1));
	gen_ensure(dba_record_equals(rec1, rec));
	CHECKED(dba_record_key_setc(rec1, DBA_KEY_YEARMIN, "1975"));
	gen_ensure(!dba_record_equals(rec, rec1));
	gen_ensure(!dba_record_equals(rec1, rec));

	CHECKED(dba_record_copy(rec1, rec));
	gen_ensure(dba_record_equals(rec, rec1));
	gen_ensure(dba_record_equals(rec1, rec));
	CHECKED(dba_record_key_unset(rec1, DBA_KEY_YEARMIN));
	gen_ensure(!dba_record_equals(rec, rec1));
	gen_ensure(!dba_record_equals(rec1, rec));

	CHECKED(dba_record_copy(rec1, rec));
	gen_ensure(dba_record_equals(rec, rec1));
	gen_ensure(dba_record_equals(rec1, rec));
	CHECKED(dba_record_var_setc(rec1, DBA_VAR(0, 20, 1), "45"));
	gen_ensure(!dba_record_equals(rec, rec1));
	gen_ensure(!dba_record_equals(rec1, rec));

	CHECKED(dba_record_copy(rec1, rec));
	gen_ensure(dba_record_equals(rec, rec1));
	gen_ensure(dba_record_equals(rec1, rec));
	CHECKED(dba_record_var_unset(rec1, DBA_VAR(0, 20, 1)));
	gen_ensure(!dba_record_equals(rec, rec1));
	gen_ensure(!dba_record_equals(rec1, rec));
}

// Test dba_record_equals
template<> template<>
void to::test<6>()
{
	int ival;
	double dval;

	dba_record rec;
	CHECKED(dba_record_create(&rec));
	CHECKED(dba_record_set_from_string(rec, "ana_id=10"));
	CHECKED(dba_record_set_from_string(rec, "lat=-10"));
	CHECKED(dba_record_set_from_string(rec, "lon=-10.45"));
	CHECKED(dba_record_set_from_string(rec, "B20001=4560"));
	CHECKED(dba_record_set_from_string(rec, "height=654"));

	CHECKED(dba_record_contains_key(rec, DBA_KEY_ANA_ID, &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LON, &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 20, 1), &ival));
	gen_ensure_equals(ival, 1);
	CHECKED(dba_record_contains_var(rec, DBA_VAR(0, 7, 1), &ival));
	gen_ensure_equals(ival, 1);

	CHECKED(dba_record_key_enqi(rec, DBA_KEY_ANA_ID, &ival));
	gen_ensure_equals(ival, 10);
	CHECKED(dba_record_key_enqd(rec, DBA_KEY_LAT, &dval));
	gen_ensure_equals(dval, -10.0);
	CHECKED(dba_record_key_enqd(rec, DBA_KEY_LON, &dval));
	gen_ensure_equals(dval, -10.45);

	CHECKED(dba_record_var_enqd(rec, DBA_VAR(0, 20, 1), &dval));
	gen_ensure_equals(dval, 4560.0);
	CHECKED(dba_record_var_enqd(rec, DBA_VAR(0, 7, 1), &dval));
	gen_ensure_equals(dval, 654.0);
}

}

/* vim:set ts=4 sw=4: */
