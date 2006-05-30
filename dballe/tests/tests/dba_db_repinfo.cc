#include <tests/test-utils.h>
#include <dballe/init.h>
#include <dballe/db/querybuf.h>
#include <dballe/db/dba_db.h>
#include <dballe/db/internals.h>
#include <dballe/db/repinfo.h>


namespace tut {
using namespace tut_dballe;

struct dba_db_repinfo_shar
{
	// DB handle
	dba_db db;
	dba_db_repinfo ri;

	dba_db_repinfo_shar() : db(NULL)
	{
		CHECKED(dba_init());
		CHECKED(create_dba_db(&db));
		ri = db->repinfo;
	}

	~dba_db_repinfo_shar()
	{
		if (db != NULL) dba_db_delete(db);
		dba_shutdown();
	}
};
TESTGRP(dba_db_repinfo);

/* Test simple queries */
template<> template<>
void to::test<1>()
{
	int id, i;

	CHECKED(dba_db_repinfo_get_id(ri, "synop", &id));
	gen_ensure_equals(id, 1);

	CHECKED(dba_db_repinfo_has_id(ri, 1, &i));
	gen_ensure_equals((bool)i, true);

	CHECKED(dba_db_repinfo_has_id(ri, 199, &i));
	gen_ensure_equals((bool)i, false);
}

/* Test update */
template<> template<>
void to::test<2>()
{
	int added, deleted, updated;

	CHECKED(dba_db_repinfo_update(ri, NULL, &added, &deleted, &updated));

	gen_ensure_equals(added, 0);
	gen_ensure_equals(deleted, 0);
	gen_ensure_equals(updated, 62);
}

}

/* vim:set ts=4 sw=4: */
