/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "core/test-utils-core.h"
#include "core/record.h"

using namespace wibble::tests;
using namespace dballe;
using namespace std;

namespace tut {

struct record_shar
{
	Record rec;

	record_shar()
	{
	}

	~record_shar()
	{
	}
};
TESTGRP(record);


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
	ensure(dba_item_get(chain, param, &item) == DBA_ERROR);
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
		int found; \
		int val; \
		CHECKED(dba_record_##keyvar##_enqi(rec, param, &val, &found)); \
		ensure_equals(found, 1); \
		ensure_equals(val, value); \
	} while (0)

#define fail_unless_double_is(keyvar, param, value) do { \
		int found; \
		double val; \
		CHECKED(dba_record_##keyvar##_enqd(rec, param, &val, &found)); \
		ensure_equals(found, 1); \
		ensure_equals(val, value); \
	} while (0)

#define fail_unless_char_is(keyvar, param, value) do { \
		const char* val; \
		CHECKED(dba_record_##keyvar##_enqc(rec, param, &val)); \
		ensure(val != NULL); \
		ensure_equals(string(val), string(value)); \
	} while (0)


// Check handling of keyword and variable information
template<> template<>
void to::test<1>()
{
	// Keyword name resolution
	ensure(Record::keyword_byname("cippo") == DBA_KEY_ERROR);
	ensure(Record::keyword_byname("zzzip") == DBA_KEY_ERROR);

	ensure(Record::keyword_byname("ana_id") == DBA_KEY_ANA_ID);
	ensure(Record::keyword_byname_len("ana_idi", 6) == DBA_KEY_ANA_ID);
	ensure_equals(string(Record::keyword_info(DBA_KEY_ANA_ID)->desc), "Station database ID");

	ensure(Record::keyword_byname("yearmin") == DBA_KEY_YEARMIN);
	ensure_equals(string(Record::keyword_info(DBA_KEY_YEARMIN)->desc), "Year or minimum year queried");

	ensure(Record::keyword_byname("lat") == DBA_KEY_LAT);
	ensure_equals(string(Record::keyword_info(DBA_KEY_LAT)->desc), "Latitude");

	ensure(Record::keyword_byname("lon") == DBA_KEY_LON);
	ensure_equals(string(Record::keyword_info(DBA_KEY_LON)->desc), "Longitude");
}

// Test get and set methods
template<> template<>
void to::test<2>()
{
	// Check that things don't exist at the beginning
	ensure(rec.key_peek(DBA_KEY_ANA_ID) == NULL);
	ensure(rec.key_peek(DBA_KEY_LAT) == NULL);
	ensure(rec.var_peek(WR_VAR(0, 20, 1)) == NULL);
	ensure(rec.var_peek(WR_VAR(0, 20, 3)) == NULL);

	// Set various things
	rec.key(DBA_KEY_ANA_ID).seti(-10);
	rec.key(DBA_KEY_LAT).seti(1234567);
	rec.key(DBA_KEY_LON).setd(76.54321);
	rec.key(DBA_KEY_YEARMIN).setc("1976");
	rec.var(WR_VAR(0, 20, 1)).setc("456");
	rec.var(WR_VAR(0, 20, 3)).setc("456");

	// Check that they now exist
	ensure(rec.key_peek(DBA_KEY_ANA_ID) != NULL);
	ensure(rec.key_peek(DBA_KEY_LAT) != NULL);
	ensure(rec.var_peek(WR_VAR(0, 20, 1)) != NULL);
	ensure(rec.var_peek(WR_VAR(0, 20, 3)) != NULL);

	// Check that they have the right value
	ensure_equals(rec.key_peek(DBA_KEY_ANA_ID)->enqi(), -10);
	ensure_equals(rec.key_peek(DBA_KEY_ANA_ID)->enqd(), -10.0);
	ensure_equals(rec.key_peek(DBA_KEY_LON)->enqi(), 7654321);
	ensure_equals(rec.key_peek(DBA_KEY_LON)->enqd(), 76.54321);
	ensure_equals(string(rec.key_peek(DBA_KEY_LON)->enqc()), "7654321");
	ensure_equals(rec.key_peek(DBA_KEY_LAT)->enqd(), 12.34567);
	ensure_equals(rec.key_peek(DBA_KEY_YEARMIN)->enqd(), 1976.0);
	ensure_equals(rec.var_peek(WR_VAR(0, 20, 1))->enqd(), 4560.0);
	ensure_equals(rec.var_peek(WR_VAR(0, 20, 3))->enqd(), 456);

	// See if unset works for keywords
	rec.key_unset(DBA_KEY_LAT);
	ensure(rec.key_peek(DBA_KEY_LAT) == NULL);

	// See if unset works for variables
	rec.var_unset(WR_VAR(0, 20, 1));
	ensure(rec.var_peek(WR_VAR(0, 20, 1)) == NULL);

	/* fprintf(stderr, "IVAL: %d\n", ival); */
	/* fprintf(stderr, "DVAL: %f\n", fval); */
	/*
	{
		int i = 7654321;
		double f = (double)i / 100000;
		fprintf(stderr, "I: %d, F: %f\n", i, f);
	}
	*/

	// See if clear clears
	rec.clear();
	ensure(rec.key_peek(DBA_KEY_LAT) == NULL);
	ensure(rec.var_peek(WR_VAR(0, 20, 3)) == NULL);

	rec.clear();
	ensure(rec.key_peek(DBA_KEY_LAT) == NULL);
	ensure(rec.var_peek(WR_VAR(0, 20, 3)) == NULL);
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

		ensure(item != chain);
		
		fail_unless_chain_is(chain, 1, chain);
		fail_unless_chain_is(chain, 2, item);

		dba_item_remove(&chain, 1);
		ensure(item == chain);
		
		fail_unless_chain_hasnt(chain, 1);
		fail_unless_chain_is(chain, 2, item);

		dba_item_remove(&chain, 2);

		ensure(chain == NULL);

		dba_item_delete(chain);
	}
#endif

#if 0
	{
		/* Hash table handling */
		dba_record rec;
		dba_item item = NULL;

		CHECKED(dba_record_create(&rec));
		ensure(rec != NULL);

		fail_unless_hasnt(rec, 1);
		CHECKED(dba_record_obtain_item(rec, 1, &item));
		CHECKED(dba_var_createc(info1, &(item->var), "1"));
		ensure(item != NULL);
		fail_unless_has(rec, 1);

		fail_unless_hasnt(rec, 2);
		CHECKED(dba_record_obtain_item(rec, 2, &item));
		CHECKED(dba_var_createc(info2, &(item->var), "2"));
		ensure(item != NULL);
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

// This used to cause a segfault
template<> template<>
void to::test<4>()
{
	Record rec;
	rec.key(DBA_KEY_IDENT).setc("nosort");

	Record rec1;
	rec = rec;
	rec.key(DBA_KEY_IDENT).setc("nosort");
}

// Test dba_record_equals
template<> template<>
void to::test<5>()
{
	Record rec;
	rec.key(DBA_KEY_ANA_ID).seti(-10);
	rec.key(DBA_KEY_LAT).seti(1234567);
	rec.key(DBA_KEY_LON).setd(76.54321);
	rec.key(DBA_KEY_YEARMIN).setc("1976");
	rec.var(WR_VAR(0, 20, 1)).setc("456");
	rec.var(WR_VAR(0, 20, 3)).setc("456");

	Record rec1;
	rec1 = rec;
	ensure(rec == rec1);
	ensure(rec1 == rec);
	ensure(!(rec != rec1));
	ensure(!(rec1 != rec));
	rec1.key(DBA_KEY_YEARMIN).seti(1975);
	ensure(rec != rec1);
	ensure(rec1 != rec);
	ensure(!(rec == rec1));
	ensure(!(rec1 == rec));

	rec1 = rec;
	ensure(rec == rec1);
	ensure(rec1 == rec);
	rec1.key_unset(DBA_KEY_YEARMIN);
	ensure(rec != rec1);
	ensure(rec1 != rec);

	rec1 = rec;
	ensure(rec == rec1);
	ensure(rec1 == rec);
	rec1.var(WR_VAR(0, 20, 1)).setc("45");
	ensure(rec != rec1);
	ensure(rec1 != rec);

	rec1 = rec;
	ensure(rec == rec1);
	ensure(rec1 == rec);
	rec1.var_unset(WR_VAR(0, 20, 1));
	ensure(rec != rec1);
	ensure(rec1 != rec);
}

// Test keyword metadata
template<> template<>
void to::test<6>()
{
	wreport::Varinfo info = Record::keyword_info(DBA_KEY_REP_MEMO);
	ensure(info->is_string());
}

// Test getting/setting Level and Trange structures
template<> template<>
void to::test<7>()
{
    Record rec;

    ensure_equals(rec.get_level(), Level());
    ensure_equals(rec.get_trange(), Trange());

    rec.set("leveltype1", 1);
    rec.set("l1", 0);
    rec.set("leveltype2", 2);
    rec.set("l2", 3);

    ensure_equals(rec.get_level(), Level(1, 0, 2, 3));
    ensure_equals(rec.get_trange(), Trange());

    rec.set("pindicator", 11);
    rec.set("p1", 22);
    rec.set("p2", 33);

    ensure_equals(rec.get_level(), Level(1, 0, 2, 3));
    ensure_equals(rec.get_trange(), Trange(11, 22, 33));

    rec.set(Level(9, 8));
    rec.set(Trange(7, 6));

    ensure_equals(rec.get("leveltype1", 0), 9);
    ensure_equals(rec.get("l1", 0), 8);
    ensure_equals(rec.get("leveltype2", 0), 0);
    ensure_equals(rec.get("l2", 0), 0);
    ensure_equals(rec.get("pindicator", 0), 7);
    ensure_equals(rec.get("p1", 0), 6);
    ensure_equals(rec.get("p2", 0), 0);

    ensure_equals(rec.get_level(), Level(9, 8));
    ensure_equals(rec.get_trange(), Trange(7, 6));
}

// Test getting/setting datetimes
template<> template<>
void to::test<8>()
{
    Record rec;
    rec.set_datetime(2013, 11, 1, 12, 0, 0);
    wassert(actual(rec.get_datetime()) == Datetime(2013, 11, 1, 12));

    rec.set(Datetime(2012, 5, 15, 17, 30, 30));
    int dt[6];
    rec.get_datetime(dt);
    wassert(actual(dt[0]) == 2012);
    wassert(actual(dt[1]) == 5);
    wassert(actual(dt[2]) == 15);
    wassert(actual(dt[3]) == 17);
    wassert(actual(dt[4]) == 30);
    wassert(actual(dt[5]) == 30);
}


// Test iteration
template<> template<>
void to::test<9>()
{
    Record rec;
    rec.set(DBA_KEY_PRIORITY, 1);
    rec.set_datetime(2013, 11, 1, 12, 0, 0);
    rec.set(DBA_KEY_VAR_RELATED, "B12123");

    unsigned count = 0;
    bool res = rec.iter_keys([&count](dba_keyword k, const wreport::Var& v) {
        ++count;
        return true;
    });

    wassert(actual(res).istrue());
    wassert(actual(count) == 8);
}

#if 0
// Test set_from_string
template<> template<>
void to::test<6>()
{
	int found;
	int ival;
	double dval;

	dba_record_clear(rec);
	CHECKED(dba_record_set_from_string(rec, "ana_id=10"));
	CHECKED(dba_record_set_from_string(rec, "lat=-10"));
	CHECKED(dba_record_set_from_string(rec, "lon=-10.45"));
	CHECKED(dba_record_set_from_string(rec, "B20001=4560"));
	CHECKED(dba_record_set_from_string(rec, "height=654"));

	CHECKED(dba_record_contains_key(rec, DBA_KEY_ANA_ID, &ival));
	ensure_equals(ival, 1);
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LAT, &ival));
	ensure_equals(ival, 1);
	CHECKED(dba_record_contains_key(rec, DBA_KEY_LON, &ival));
	ensure_equals(ival, 1);
	CHECKED(dba_record_contains_var(rec, WR_VAR(0, 20, 1), &ival));
	ensure_equals(ival, 1);
	CHECKED(dba_record_contains_var(rec, WR_VAR(0, 7, 1), &ival));
	ensure_equals(ival, 1);

	CHECKED(dba_record_key_enqi(rec, DBA_KEY_ANA_ID, &ival, &found));
	ensure_equals(found, 1);
	ensure_equals(ival, 10);
	CHECKED(dba_record_key_enqd(rec, DBA_KEY_LAT, &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, -10.0);
	CHECKED(dba_record_key_enqd(rec, DBA_KEY_LON, &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, -10.45);

	CHECKED(dba_record_var_enqd(rec, WR_VAR(0, 20, 1), &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, 4560.0);
	CHECKED(dba_record_var_enqd(rec, WR_VAR(0, 7, 1), &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, 654.0);
}

// Test get and set methods using strings
template<> template<>
void to::test<7>()
{
	int ival;
	double dval;
	const char* cval;
	int found;
	dba_record_clear(rec);

	// Keywords

	CHECKED(dba_record_key_seti(rec, DBA_KEY_ANA_ID, 3));
	CHECKED(dba_record_enqi(rec, "ana_id", &ival, &found));
	ensure_equals(found, 1);
	ensure_equals(ival, 3);

	CHECKED(dba_record_key_setd(rec, DBA_KEY_LAT, 15.2));
	CHECKED(dba_record_enqd(rec, "lat", &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, 15.2);

	CHECKED(dba_record_key_setc(rec, DBA_KEY_IDENT, "ciao"));
	CHECKED(dba_record_enqc(rec, "ident", &cval));
	ensure(cval != NULL);
	ensure_equals(string(cval), "ciao");

	// Values

	CHECKED(dba_record_var_seti(rec, WR_VAR(0, 1, 1), 5));
	CHECKED(dba_record_enqi(rec, "block", &ival, &found));
	ensure_equals(found, 1);
	ensure_equals(ival, 5);
	CHECKED(dba_record_enqi(rec, "B01001", &ival, &found));
	ensure_equals(found, 1);
	ensure_equals(ival, 5);

	CHECKED(dba_record_var_setd(rec, WR_VAR(0, 12, 101), 12.5));
	CHECKED(dba_record_enqd(rec, "t", &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, 12.5);
	CHECKED(dba_record_enqd(rec, "B12101", &dval, &found));
	ensure_equals(found, 1);
	ensure_equals(dval, 12.5);

	CHECKED(dba_record_var_setc(rec, WR_VAR(0, 1, 19), "oaic"));
	CHECKED(dba_record_enqc(rec, "name", &cval));
	ensure(cval != NULL);
	ensure_equals(string(cval), "oaic");
	CHECKED(dba_record_enqc(rec, "B01019", &cval));
	ensure(cval != NULL);
	ensure_equals(string(cval), "oaic");
}

// Test getters and setters of various elements
template<> template<>
void to::test<8>()
{
	// Start from cases that failed in the past only
	CHECKED(dba_record_key_seti(rec, DBA_KEY_P2, 2));
}
#endif

}

/* vim:set ts=4 sw=4: */
