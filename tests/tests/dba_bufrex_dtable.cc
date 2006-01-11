#include <tests/test-utils.h>
#include <dballe/bufrex/bufrex_dtable.h>

namespace tut {
using namespace tut_dballe;

struct dba_bufrex_dtable_shar
{
	dba_bufrex_dtable_shar()
	{
	}

	~dba_bufrex_dtable_shar()
	{
	}
};
TESTGRP(dba_bufrex_dtable);

// Test basic queries
template<> template<>
void to::test<1>()
{
	const char* oldenv = getenv("DBA_TABLES");
	setenv("DBA_TABLES", ".", 1);

	bufrex_dtable table;
	bufrex_opcode chain;
	bufrex_opcode cur;

	CHECKED(bufrex_dtable_create("test-crex-d-table", &table));

	/* Try querying a nonexisting item */
	gen_ensure_equals(bufrex_dtable_query(table, DBA_VAR(3, 0, 9), &chain), DBA_ERROR);
	chain = NULL;

	/* Now query an existing item */
	CHECKED(bufrex_dtable_query(table, DBA_VAR(3, 35, 6), &chain));
	gen_ensure(chain != NULL);
	cur = chain;

	/*fprintf(stderr, "VAL: %d %02d %03d\n", DBA_VAR_F(cur->val), DBA_VAR_X(cur->val), DBA_VAR_Y(cur->val));*/
	gen_ensure_equals(cur->val, DBA_VAR(0, 8, 21));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 4, 4));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 8, 21));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 4, 4));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 35, 0));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 1, 3));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(0, 35, 11));

	bufrex_opcode_delete(&chain);
	gen_ensure_equals(chain, (bufrex_opcode)0);

	/* Then query the last item */
	CHECKED(bufrex_dtable_query(table, DBA_VAR(3, 35, 10), &chain));
	gen_ensure(chain != NULL);
	cur = chain;

	gen_ensure_equals(cur->val, DBA_VAR(3, 35, 2));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(3, 35, 3));
	cur = cur->next; gen_ensure(cur != NULL);
	gen_ensure_equals(cur->val, DBA_VAR(3, 35, 7));

	bufrex_opcode_delete(&chain);
	gen_ensure_equals(chain, (bufrex_opcode)0);

	/* Cleanup after the tests */
	setenv("DBA_TABLES", oldenv, 1);
}

}

/* vim:set ts=4 sw=4: */
