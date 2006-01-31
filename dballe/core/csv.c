#include <dballe/core/csv.h>

/*
#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
*/

#include <stdio.h>
#include <string.h>

int dba_csv_read_next(FILE* in, char** cols, int col_max)
{
	char line[2000];
	char* tok;
	char* stringp;
	int i;

	if (fgets(line, 2000, in) == NULL)
		return 0;

	for (i = 0, stringp = line; i < col_max - 1 && (tok = strsep(&stringp, ",")) != NULL; i++)
		cols[i] = strdup(tok);

	return i;
}

#if 0
#ifdef HAVE_CHECK
#include <check.h>

static void print_dba_error()
{
	const char* details = dba_error_get_details();
	fprintf(stderr, "Error %d (%s) while %s",
				dba_error_get_code(),
				dba_error_get_message(),
				dba_error_get_context());
	if (details == NULL)
		fputc('\n', stderr);
	else
		fprintf(stderr, ".  Details:\n%s\n", details);
}

#define CHECKED(...) \
	do{ \
		dba_err err = __VA_ARGS__; \
		if (err) print_dba_error(); \
		fail_unless(err == DBA_OK); \
	} while (0)

void test_dba_csv()
{
	{
		/* Test dba_item handling */

		dba_item chain = NULL, item = NULL;

		CHECKED(dba_item_obtain(&chain, "cippo", &item));

		fail_unless_chain_is(chain, "cippo", chain);
		fail_unless_chain_hasnt(chain, "lippo");

		CHECKED(dba_item_obtain(&chain, "lippo", &item));

		fail_unless(item != chain);
		
		fail_unless_chain_is(chain, "cippo", chain);
		fail_unless_chain_is(chain, "lippo", item);

		dba_item_remove(&chain, "cippo");
		fail_unless(item == chain);
		
		fail_unless_chain_hasnt(chain, "cippo");
		fail_unless_chain_is(chain, "lippo", item);

		dba_item_remove(&chain, "lippo");

		fail_unless(chain == NULL);

		dba_item_delete(chain);
	}

	{
		/* Hash table handling */
		dba_record rec;
		dba_item item = NULL;

		CHECKED(dba_record_create(&rec));
		fail_unless(rec != NULL);

		fail_unless_hasnt(rec, "cippo");
		CHECKED(dba_record_obtain_item(rec, "cippo", &item));
		fail_unless(item != NULL);
		fail_unless_has(rec, "cippo");
		item->value = strdup("cippo");

		fail_unless_hasnt(rec, "lippo");
		CHECKED(dba_record_obtain_item(rec, "lippo", &item));
		fail_unless(item != NULL);
		fail_unless_has(rec, "cippo");
		fail_unless_has(rec, "lippo");
		item->value = strdup("lippo");

		fail_unless_is_it(rec, "cippo");
		fail_unless_is_it(rec, "lippo");

		dba_record_remove_item(rec, "cippo");
		fail_unless_hasnt(rec, "cippo");
		fail_unless_has(rec, "lippo");

		dba_record_remove_item(rec, "cippo");
		fail_unless_hasnt(rec, "cippo");
		fail_unless_has(rec, "lippo");

		dba_record_remove_item(rec, "pippo");
		fail_unless_hasnt(rec, "cippo");
		fail_unless_has(rec, "lippo");

		dba_record_remove_item(rec, "lippo");
		fail_unless_hasnt(rec, "cippo");
		fail_unless_hasnt(rec, "lippo");

		dba_record_remove_item(rec, "pippo");
		fail_unless_hasnt(rec, "cippo");
		fail_unless_hasnt(rec, "lippo");

		dba_record_delete(rec);
	}

	{
		/* Keyword info handling */
		dba_varinfo* info;
		int index;

		fail_unless(dba_record_keyword("cippo", &index) == NULL);
		fail_unless(dba_record_keyword("zzzip", &index) == NULL);

		fail_unless((info = dba_record_keyword("idstaz_select", &index)) != NULL);
		fail_unless(strcmp(info->desc, "Unique station identifier") == 0);

		fail_unless((info = dba_record_keyword("yearmin", &index)) != NULL);
		fail_unless(strcmp(info->desc, "Year or minimum year queried") == 0);

		fail_unless((info = dba_record_keyword("lat", &index)) != NULL);
		fail_unless(strcmp(info->desc, "Latitude") == 0);

		fail_unless((info = dba_record_keyword("lon", &index)) != NULL);
		fail_unless(strcmp(info->desc, "Longitude") == 0);
	}

	{
		/* Record gets and sets */
		dba_err err;
		int ival;
		dba_record rec;

		CHECKED(dba_record_create(&rec));

		err = dba_enqi(rec, "idstaz_select", &ival);
		fail_unless(err == DBA_ERROR);
		fail_unless(dba_error_get_code() == DBA_ERR_TYPE);

		err = dba_enqi(rec, "lat", &ival);
		fail_unless(err == DBA_ERROR);
		fail_unless(dba_error_get_code() == DBA_ERR_NOTFOUND);

		err = dba_seti(rec, "idstaz_select", 1);
		fail_unless(err == DBA_ERROR);
		fail_unless(dba_error_get_code() == DBA_ERR_TYPE);

		CHECKED(dba_seti(rec, "ana_id", -10));
		CHECKED(dba_seti(rec, "lat", 1234567));
		CHECKED(dba_setd(rec, "lon", 76.54321));
		CHECKED(dba_setc(rec, "yearmin", "1976"));
		CHECKED(dba_setc(rec, "B02121", "456"));

		fail_unless_int_is(rec, "ana_id", -10);
		fail_unless_real_is(rec, "ana_id", -10.0);
		fail_unless_int_is(rec, "lon", 7654321);
		/*fail_unless_float_is(rec, "lon", 76.54321);*/
		fail_unless_real_is(rec, "lon", 76.54321);
		fail_unless_char_is(rec, "lon", "7654321");

		fail_unless_int_is(rec, "lat", 1234567);
		/*fail_unless_float_is(rec, "lat", 12.34567);*/
		fail_unless_real_is(rec, "lat", 12.34567);
		fail_unless_char_is(rec, "lat", "1234567");

		fail_unless_int_is(rec, "yearmin", 1976);
		fail_unless_float_is(rec, "yearmin", 1976);
		fail_unless_real_is(rec, "yearmin", 1976);
		fail_unless_char_is(rec, "yearmin", "1976");

		fail_unless_int_is(rec, "B02121", 456);
		/*fail_unless_float_is(rec, "B02121", 45600000000.0)*/;
		fail_unless_real_is(rec, "B02121", 45600000000.0);
		fail_unless_char_is(rec, "B02121", "456");

		CHECKED(dba_unset(rec, "lat"));

		err = dba_enqi(rec, "lat", &ival);
		fail_unless(err == DBA_ERROR);
		fail_unless(dba_error_get_code() == DBA_ERR_NOTFOUND);

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
		
		err = dba_enqi(rec, "lat", &ival);
		fail_unless(err == DBA_ERROR);
		fail_unless(dba_error_get_code() == DBA_ERR_NOTFOUND);

		dba_record_clear(rec);

		err = dba_enqi(rec, "lat", &ival);
		fail_unless(err == DBA_ERROR);
		fail_unless(dba_error_get_code() == DBA_ERR_NOTFOUND);

		dba_record_delete(rec);
	}
}

#endif
#endif

/* vim:set ts=4 sw=4: */
