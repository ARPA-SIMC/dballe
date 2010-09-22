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
#include <sstream>
#include <stdlib.h>
#include <string.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

Context::Context(const Level& lev, const Trange& tr)
    : level(lev), trange(tr)
{
}

Context::Context(const Context& c)
    : level(c.level), trange(c.trange)
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

    level = src.level;
    trange = src.trange;

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
    if (res = level.compare(ctx.level)) return res;
    return trange.compare(ctx.trange);
}

int Context::compare(const Level& lev, const Trange& tr) const
{
    int res;
    if (res = level.compare(lev)) return res;
    return trange.compare(tr);
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

Var* Context::edit(Varcode code)
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
    stringstream header;
    header << "Level " << level << "  tr " << trange;
    fprintf(out, "%s ", header.str().c_str());

    if (data.size() > 0)
    {
        fprintf(out, " %d vars:\n", data.size());
        for (vector<Var*>::const_iterator i = data.begin(); i != data.end(); ++i)
            (*i)->print(out);
    } else
        fprintf(out, "exists but is empty.\n");
}

static void context_summary(const msg::Context& c, FILE* out)
{
    stringstream str;
    str << c.level << ", " << c.trange;
    fprintf(out, "c(%s)", str.str().c_str());
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
    if (level != ctx.level || trange != ctx.trange)
    {
        stringstream msg;
        msg << "the contexts are different (first is "
            << level << ", " << trange
            << " second is "
            << ctx.level << ", " << ctx.trange
            << ")" << endl;
        fputs(msg.str().c_str(), out);
        return 1;
    }

    size_t i1 = 0, i2 = 0;
    unsigned diffs = 0;
    while (i1 < data.size() || i2 < ctx.data.size())
    {
        if (i1 == data.size())
        {
            fputs("Variable ", out); context_summary(ctx, out);
            fputs(" ", out); var_summary(*ctx.data[i2], out);
            fprintf(out, " exists only in the second message\n");
            ++i2;
            ++diffs;
        } else if (i2 == ctx.data.size()) {
            fputs("Variable ", out); context_summary(*this, out);
            fputs(" ", out); var_summary(*data[i1], out);
            fprintf(out, " exists only in the first message\n");
            ++i1;
            ++diffs;
        } else {
            int cmp = (int)data[i1]->code() - (int)ctx.data[i2]->code();
            if (cmp == 0)
            {
                diffs += data[i1]->diff(*ctx.data[i2], out);
                ++i1;
                ++i2;
            } else if (cmp < 0) {
                if (data[i1]->value() != NULL)
                {
                    fputs("Variable ", out); context_summary(*this, out);
                    fputs(" ", out); var_summary(*data[i1], out);
                    fprintf(out, " exists only in the first message\n");
                    ++diffs;
                }
                ++i1;
            } else {
                if (ctx.data[i2]->value() != NULL)
                {
                    fputs("Variable ", out); context_summary(ctx, out);
                    fputs(" ", out); var_summary(*ctx.data[i2], out);
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
    if ((level.ltype1 != 100 && level.ltype1 != 102) || trange != Trange(254))
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
