/*
 * msg/context - Hold variables with the same physical context
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

#include <dballe/msg/context.h>
#include <dballe/msg/vars.h>

#include <stdlib.h>
#include <string.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

Context::Context(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
	: ltype1(ltype1), l1(l1), ltype2(ltype2), l2(l2), pind(pind), p1(p1), p2(p2)
{
}

Context::Context(const Context& c)
	: ltype1(c.ltype1), l1(c.l1), ltype2(c.ltype2), l2(c.l2), pind(c.pind), p1(c.p1), p2(c.p2)
{
	// Reserve space for the new vars
	data.reserve(c.data.size());
	
	// Copy the variables
	for (vector<Var*>::const_iterator i = c.data.begin();
			i != c.data.end(); ++i)
        data.push_back(new Var(**i));
}

Context::~Context()
{
	for (vector<Var*>::iterator i = data.begin();
			i != data.end(); ++i)
		delete *i;
}

Context& Context::operator=(const Context& src)
{
    // Manage a = a
    if (this == &src) return *this;

	ltype1 = src.ltype1;
	l1 = src.l1;
	ltype2 = src.ltype2;
	l2 = src.l2;
	pind = src.pind;
	p1 = src.p1;
	p2 = src.p2;

	// Delete existing vars
	for (vector<Var*>::iterator i = data.begin();
			i != data.end(); ++i)
		delete *i;
	data.clear();

	// Reserve space for the new vars
	data.reserve(src.data.size());
	
	// Copy the variables
	for (vector<Var*>::const_iterator i = src.data.begin();
			i != src.data.end(); ++i)
        data.push_back(new Var(**i));
}

int Context::compare(const Context& ctx) const
{
	int res;
	if ((res = ltype1 - ctx.ltype1)) return res;
	if ((res = l1 - ctx.l1)) return res;
	if ((res = ltype2 - ctx.ltype2)) return res;
	if ((res = l2 - ctx.l2)) return res;
	if ((res = pind - ctx.pind)) return res;
	if ((res = p1 - ctx.p1)) return res;
	return p2 - ctx.p2;
}

int Context::compare(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2) const
{
	int res;
	if ((res = this->ltype1 - ltype1)) return res;
	if ((res = this->l1 - l1)) return res;
	if ((res = this->ltype2 - ltype2)) return res;
	if ((res = this->l2 - l2)) return res;
	if ((res = this->pind - pind)) return res;
	if ((res = this->p1 - p1)) return res;
	return this->p2 - p2;
}

void Context::set(const Var& var)
{
    set(auto_ptr<Var>(new Var(var)));
}

void Context::set(auto_ptr<Var> var)
{
	Varcode code = var->code();
	int idx = find_index(code);

	if (idx != -1)
	{
		/* Replace the variable */
        delete data[idx];
	}
	else
	{
		/* Add the value */

		/* Enlarge the buffer */
        data.resize(data.size() + 1);

		/* Insertionsort.  Crude, but our datasets should be too small for an
		 * RB-Tree to be worth */
		for (idx = data.size() - 1; idx > 0; --idx)
			if (data[idx - 1]->code() > code)
				data[idx] = data[idx - 1];
			else
				break;
	}
    data[idx] = var.release();
}

int Context::find_index(Varcode code) const
{
	/* Binary search */
	int low = 0, high = data.size() - 1;
	while (low <= high)
	{
		int middle = low + (high - low)/2;
		int cmp = (int)code - (int)data[middle]->code();
		if (cmp < 0)
			high = middle - 1;
		else if (cmp > 0)
			low = middle + 1;
		else
			return middle;
	}

	return -1;
}

const Var* Context::find(Varcode code) const
{
	int idx = find_index(code);
	return (idx == -1) ? NULL : data[idx];
}

const Var* Context::find_by_id(int id) const
{
	return find(shortcutTable[id].code);
}

void Context::print(FILE* out) const
{
	fprintf(out, "Level %d,%d, %d,%d  tr %d,%d,%d ", ltype1, l1, ltype2, l2, pind, p1, p2);

	if (data.size() > 0)
	{
		fprintf(out, " %d vars:\n", data.size());
		for (vector<Var*>::const_iterator i = data.begin(); i != data.end(); ++i)
            (*i)->print(out);
	} else
		fprintf(out, "exists but is empty.\n");
}

static void var_summary(const Var& var, FILE* out)
{
	Varcode v = var.code();
	fprintf(out, "%d%02d%03d[%s]",
			WR_VAR_F(v), WR_VAR_X(v), WR_VAR_Y(v),
			var.info()->desc);
}

unsigned Context::diff(const Context& ctx, FILE* out) const
{
	if (ltype1 != ctx.ltype1 || l1 != ctx.l1
	 || ltype2 != ctx.ltype2 || l2 != ctx.l2
	 || pind != ctx.pind || p1 != ctx.p1 || p2 != ctx.p2)
	{
		fprintf(out, "the contexts are different (first is %d,%d, %d,%d, %d,%d,%d second is %d,%d, %d,%d, %d,%d,%d)\n",
				ltype1, l1, ltype2, l2, pind, p1, p2,
				ctx.ltype1, ctx.l1, ctx.ltype2, ctx.l2, ctx.pind, ctx.p1, ctx.p2);
		return 1;
	}
	
	int i1 = 0, i2 = 0;
    unsigned diffs = 0;
	while (i1 < data.size() || i2 < ctx.data.size())
	{
		if (i1 == data.size())
		{
			fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", ctx.ltype1, ctx.l1, ctx.ltype2, ctx.l2, ctx.pind, ctx.p1, ctx.p2);
            var_summary(*ctx.data[i2], out);
			fprintf(out, " exists only in the second message\n");
			++i2;
			++diffs;
		} else if (i2 == ctx.data.size()) {
			fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", ltype1, l1, ltype2, l2, pind, p1, p2);
            var_summary(*data[i1], out);
			fprintf(out, " exists only in the first message\n");
			++i1;
			++diffs;
		} else {
			int cmp = (int)data[i1]->code() - (int)data[i2]->code();
			if (cmp == 0)
			{
				diffs += data[i1]->diff(*data[i2], out);
				++i1;
				++i2;
			} else if (cmp < 0) {
				if (data[i1]->value() != NULL)
				{
					fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", ltype1, l1, ltype2, l2, pind, p1, p2);
                    var_summary(*data[i1], out);
					fprintf(out, " exists only in the first message\n");
					++diffs;
				}
				++i1;
			} else {
				if (ctx.data[i2]->value() != NULL)
				{
					fprintf(out, "Variable l(%d,%d, %d,%d, %d,%d,%d) ", ctx.ltype1, ctx.l1, ctx.ltype2, ctx.l2, ctx.pind, ctx.p1, ctx.p2);
                    var_summary(*ctx.data[i2], out);
					fprintf(out, " exists only in the second message\n");
					++diffs;
				}
				++i2;
			}
		}
	}
    return diffs;
}

const Var* Context::find_vsig() const
{
	// Check if we have the right context information
	if ((ltype1 != 100 && ltype1 != 102) || pind != 254 || p1 != 0 || p2 != 0)
		return NULL;

	// Look for VSS variable
	const Var* res = find(WR_VAR(0, 8, 1));
	if (res == NULL) return NULL;

	// Ensure it is not undefined
	if (res->value() == NULL) return NULL;

	// Finally return it
	return res;
}

}
}

/* vim:set ts=4 sw=4: */
