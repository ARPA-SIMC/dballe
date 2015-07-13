/*
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

#include "msg/tests.h"
#include "msg/context.h"
#include <memory>

using namespace wreport;
using namespace dballe;
using namespace std;

namespace tut {

struct context_shar
{
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
    Level lev(9, 8, 7, 6);
    unique_ptr<msg::Context> c1(new msg::Context(lev, Trange(1, 2, 3)));
    unique_ptr<msg::Context> c2(new msg::Context(lev, Trange(1, 3, 2)));

	ensure_equals(c1->data.size(), 0);
	ensure_equals(c1->level, lev);
	ensure_equals(c1->trange, Trange(1, 2, 3));
	ensure_equals(c2->data.size(), 0);
	ensure_equals(c2->level, lev);
	ensure_equals(c2->trange, Trange(1, 3, 2));

    c1->set(var(WR_VAR(0, 1, 1)));
    c2->set(var(WR_VAR(0, 1, 1)));

	ensure(c1->compare(*c2) < 0);
	ensure(c2->compare(*c1) > 0);
	ensure_equals(c1->compare(*c1), 0);
	ensure_equals(c2->compare(*c2), 0);

	ensure(c1->compare(lev, Trange(1, 2, 4)) < 0);
	ensure(c1->compare(lev, Trange(1, 2, 2)) > 0);
	ensure(c1->compare(lev, Trange(1, 3, 3)) < 0);
	ensure(c1->compare(lev, Trange(1, 1, 3)) > 0);
	ensure(c1->compare(lev, Trange(2, 2, 3)) < 0);
	ensure(c1->compare(lev, Trange(0, 2, 3)) > 0);
	ensure(c1->compare(Level(9, 8, 7, 7), Trange(1, 2, 3)) < 0);
	ensure(c1->compare(Level(9, 8, 7, 5), Trange(1, 2, 3)) > 0);
	ensure_equals(c1->compare(lev, Trange(1, 2, 3)), 0);
}

/* Test Context external ordering */
template<> template<>
void to::test<2>()
{
    Trange tr(1, 2, 3);
    unique_ptr<msg::Context> c1(new msg::Context(Level(1, 2, 3, 4), tr));
    unique_ptr<msg::Context> c2(new msg::Context(Level(2, 1, 4, 3), tr));

	ensure_equals(c1->data.size(), 0);
	ensure_equals(c1->level, Level(1, 2, 3, 4));
	ensure_equals(c2->data.size(), 0);
	ensure_equals(c2->level, Level(2, 1, 4, 3));

	ensure(c1->compare(*c2) < 0);
	ensure(c2->compare(*c1) > 0);
	ensure_equals(c1->compare(*c1), 0);
	ensure_equals(c2->compare(*c2), 0);

	ensure(c1->compare(Level(1, 2, 4, 4), tr) < 0);
	ensure(c1->compare(Level(1, 2, 2, 4), tr) > 0);
	ensure(c1->compare(Level(1, 3, 3, 4), tr) < 0);
	ensure(c1->compare(Level(1, 1, 3, 4), tr) > 0);
	ensure(c1->compare(Level(2, 2, 3, 4), tr) < 0);
	ensure(c1->compare(Level(0, 2, 3, 4), tr) > 0);
	ensure_equals(c1->compare(Level(1, 2, 3, 4), tr), 0);
}

/* Test msg::Context internal ordering */
template<> template<>
void to::test<3>()
{
	unique_ptr<msg::Context> c(new msg::Context(Level(1, 2, 3, 4), Trange::instant()));

	c->set(var(WR_VAR(0, 1, 1)));
	ensure_equals(c->data.size(), 1);
	c->set(var(WR_VAR(0, 1, 7)));
	ensure_equals(c->data.size(), 2);
	c->set(var(WR_VAR(0, 1, 2)));
	ensure_equals(c->data.size(), 3);
	// Variables with same code must get substituded and not added
	c->set(var(WR_VAR(0, 1, 1)));
	ensure_equals(c->data.size(), 3);

	ensure_context_is_sorted(*c);

	ensure(c->find(WR_VAR(0, 1, 1)) != NULL);
	ensure_varcode_equals(c->find(WR_VAR(0, 1, 1))->code(), WR_VAR(0, 1, 1));

	ensure(c->find(WR_VAR(0, 1, 2)) != NULL);
	ensure_varcode_equals(c->find(WR_VAR(0, 1, 2))->code(), WR_VAR(0, 1, 2));

	ensure(c->find(WR_VAR(0, 1, 7)) != NULL);
	ensure_varcode_equals(c->find(WR_VAR(0, 1, 7))->code(), WR_VAR(0, 1, 7));

	ensure_equals(c->find(WR_VAR(0, 1, 8)), (const Var*)0);
}

}

/* vim:set ts=4 sw=4: */
