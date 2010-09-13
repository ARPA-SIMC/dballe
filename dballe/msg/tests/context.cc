/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-msg.h>
#include <dballe/msg/context.h>
#include <memory>

using namespace dballe;
using namespace std;

namespace tut {

struct context_shar
{
    tests::TestMsgEnv testenv;

	context_shar()
	{
	}

	~context_shar()
	{
	}
};
TESTGRP(context);

// Ensure that the datum vector inside the context is in strict ascending order
void _ensure_context_is_sorted(const wibble::tests::Location& loc, const msg::Context& ctx)
{
	if (ctx.data.size() < 2)
		return;
	for (int i = 0; i < ctx.data.size() - 1; ++i)
		inner_ensure(ctx.data[i]->code() < ctx.data[i + 1]->code());
}
#define ensure_context_is_sorted(x) _ensure_context_is_sorted(wibble::tests::Location(__FILE__, __LINE__, "context is sorted in " #x), (x))


/* Test msg::Context */
template<> template<>
void to::test<1>()
{
    auto_ptr<msg::Context> c1(new msg::Context(9, 8, 7, 6, 1, 2, 3));
    auto_ptr<msg::Context> c2(new msg::Context(9, 8, 7, 6, 1, 3, 2));

	ensure_equals(c1->data.size(), 0);
	ensure_equals(c1->ltype1, 9);
	ensure_equals(c1->l1, 8);
	ensure_equals(c1->ltype2, 7);
	ensure_equals(c1->l2, 6);
	ensure_equals(c1->pind, 1);
	ensure_equals(c1->p1, 2);
	ensure_equals(c1->p2, 3);
	ensure_equals(c2->data.size(), 0);
	ensure_equals(c2->ltype1, 9);
	ensure_equals(c2->l1, 8);
	ensure_equals(c2->ltype2, 7);
	ensure_equals(c2->l2, 6);
	ensure_equals(c2->pind, 1);
	ensure_equals(c2->p1, 3);
	ensure_equals(c2->p2, 2);

    c1->set(Var(DBA_VAR(0, 1, 1)));
    c2->set(Var(DBA_VAR(0, 1, 1)));

	ensure(c1->compare(*c2) < 0);
	ensure(c2->compare(*c1) > 0);
	ensure_equals(c1->compare(*c1), 0);
	ensure_equals(c2->compare(*c2), 0);

	ensure(c1->compare(9, 8, 7, 6, 1, 2, 4) < 0);
	ensure(c1->compare(9, 8, 7, 6, 1, 2, 2) > 0);
	ensure(c1->compare(9, 8, 7, 6, 1, 3, 3) < 0);
	ensure(c1->compare(9, 8, 7, 6, 1, 1, 3) > 0);
	ensure(c1->compare(9, 8, 7, 6, 2, 2, 3) < 0);
	ensure(c1->compare(9, 8, 7, 6, 0, 2, 3) > 0);
	ensure(c1->compare(9, 8, 7, 7, 1, 2, 3) < 0);
	ensure(c1->compare(9, 8, 7, 5, 1, 2, 3) > 0);
	ensure_equals(c1->compare(9, 8, 7, 6, 1, 2, 3), 0);
}

/* Test Context external ordering */
template<> template<>
void to::test<2>()
{
    auto_ptr<msg::Context> c1(new msg::Context(1, 2, 3, 4, 1, 2, 3));
    auto_ptr<msg::Context> c2(new msg::Context(2, 1, 4, 3, 1, 2, 3));

	ensure_equals(c1->data.size(), 0);
	ensure_equals(c1->ltype1, 1);
	ensure_equals(c1->l1, 2);
	ensure_equals(c1->ltype2, 3);
	ensure_equals(c1->l2, 4);
	ensure_equals(c2->data.size(), 0);
	ensure_equals(c2->ltype1, 2);
	ensure_equals(c2->l1, 1);
	ensure_equals(c2->ltype2, 4);
	ensure_equals(c2->l2, 3);

	ensure(c1->compare(*c2) < 0);
	ensure(c2->compare(*c1) > 0);
	ensure_equals(c1->compare(*c1), 0);
	ensure_equals(c2->compare(*c2), 0);

	ensure(c1->compare(1, 2, 4, 4, 1, 2, 3) < 0);
	ensure(c1->compare(1, 2, 2, 4, 1, 2, 3) > 0);
	ensure(c1->compare(1, 3, 3, 4, 1, 2, 3) < 0);
	ensure(c1->compare(1, 1, 3, 4, 1, 2, 3) > 0);
	ensure(c1->compare(2, 2, 3, 4, 1, 2, 3) < 0);
	ensure(c1->compare(0, 2, 3, 4, 1, 2, 3) > 0);
	ensure_equals(c1->compare(1, 2, 3, 4, 1, 2, 3), 0);
}

/* Test msg::Context internal ordering */
template<> template<>
void to::test<3>()
{
	auto_ptr<msg::Context> c(new msg::Context(1, 2, 3, 4, 1, 2, 3));

	c->set(Var(DBA_VAR(0, 1, 1)));
	ensure_equals(c->data.size(), 1);
	c->set(Var(DBA_VAR(0, 1, 7)));
	ensure_equals(c->data.size(), 2);
	c->set(Var(DBA_VAR(0, 1, 2)));
	ensure_equals(c->data.size(), 3);
	// Variables with same code must get substituded and not added
	c->set(Var(DBA_VAR(0, 1, 1)));
	ensure_equals(c->data.size(), 3);

	ensure_context_is_sorted(*c);

	ensure(c->find(DBA_VAR(0, 1, 1)) != NULL);
	ensure_varcode_equals(c->find(DBA_VAR(0, 1, 1))->code(), DBA_VAR(0, 1, 1));

	ensure(c->find(DBA_VAR(0, 1, 2)) != NULL);
	ensure_varcode_equals(c->find(DBA_VAR(0, 1, 2))->code(), DBA_VAR(0, 1, 2));

	ensure(c->find(DBA_VAR(0, 1, 7)) != NULL);
	ensure_varcode_equals(c->find(DBA_VAR(0, 1, 7))->code(), DBA_VAR(0, 1, 7));

	ensure_equals(c->find(DBA_VAR(0, 1, 8)), (const Var*)0);
}

}

/* vim:set ts=4 sw=4: */
