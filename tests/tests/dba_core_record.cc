#include <tests/test-utils.h>
#include <dballe/core/dba_record.h>

/*
extern "C" {
	dba_err dba_record_keyword_selftest();
};
*/

namespace tut {
using namespace tut_dballe;

struct dba_core_record_shar
{
	dba_core_record_shar()
	{
	}

	~dba_core_record_shar()
	{
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

template<> template<>
void to::test<1>()
{
	dba_varinfo info1, info2;

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

	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 1, 1), &info1));
	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 1, 2), &info2));

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

	{
		/* Keyword info handling */
		dba_varinfo info;

		gen_ensure(dba_record_keyword_byname("cippo") == DBA_KEY_ERROR);
		gen_ensure(dba_record_keyword_byname("zzzip") == DBA_KEY_ERROR);

		gen_ensure_equals(dba_record_keyword_byname("ana_id"), DBA_KEY_ANA_ID);
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
	}

	{
		/* Record gets and sets */
		dba_err err;
		int ival;
		dba_record rec;

		CHECKED(dba_record_create(&rec));

		err = dba_record_key_enqi(rec, DBA_KEY_ANA_ID, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		err = dba_record_key_seti(rec, DBA_KEY_VAR, 1);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_TYPE);

		CHECKED(dba_record_key_seti(rec, DBA_KEY_ANA_ID, -10));
		CHECKED(dba_record_key_seti(rec, DBA_KEY_LAT, 1234567));
		CHECKED(dba_record_key_setd(rec, DBA_KEY_LON, 76.54321));
		CHECKED(dba_record_key_setc(rec, DBA_KEY_YEARMIN, "1976"));
		CHECKED(dba_record_var_setc(rec, DBA_VAR(0, 20, 13), "456"));

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

		fail_unless_int_is(var, DBA_VAR(0, 20, 13), 456);
		/*fail_unless_float_is(rec, "B02121", 45600000000.0)*/;
		fail_unless_double_is(var, DBA_VAR(0, 20, 13), 4560);
		fail_unless_char_is(var, DBA_VAR(0, 20, 13), "456");

		CHECKED(dba_record_key_unset(rec, DBA_KEY_LAT));

		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
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
		
		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		dba_record_clear(rec);

		err = dba_record_key_enqi(rec, DBA_KEY_LAT, &ival);
		gen_ensure(err == DBA_ERROR);
		gen_ensure(dba_error_get_code() == DBA_ERR_NOTFOUND);

		dba_record_delete(rec);
	}
}

}

/* vim:set ts=4 sw=4: */
